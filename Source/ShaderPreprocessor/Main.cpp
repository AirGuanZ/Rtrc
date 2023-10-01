#include <iostream>

#include <cxxopts.hpp>

#include <Core/Filesystem/File.h>
#include <Core/Serialization/TextSerializer.h>
#include <Core/SourceWriter.h>
#include <ShaderCompiler/DXC/DXC.h>
#include <ShaderCompiler/Parser/ShaderParser.h>

struct CommandArguments
{
    std::vector<std::string> includeDirectory;
    std::string inputFilename;
    std::string outputFilename;
};

std::optional<CommandArguments> ParseCommandArguments(int argc, const char *argv[])
{
    cxxopts::Options options("Rtrc.ShaderPreprocessor");
    options.add_options()
        ("i,input",  "Input directory",   cxxopts::value<std::string>())
        ("o,output", "Output directory",  cxxopts::value<std::string>())
        ("r,root",   "Include directory", cxxopts::value<std::vector<std::string>>())
        ("h,help",   "Print usage");

    auto parseResult = options.parse(argc, argv);
    if(parseResult.count("help"))
    {
        std::cout << options.help() << std::endl;
        return std::nullopt;
    }

    CommandArguments ret;
    if(parseResult.count("input"))
    {
        ret.inputFilename = parseResult["input"].as<std::string>();
    }
    else
    {
        std::cout << options.help() << std::endl;
        return std::nullopt;
    }
    if(parseResult.count("output"))
    {
        ret.outputFilename = parseResult["output"].as<std::string>();
    }
    else
    {
        std::cout << options.help() << std::endl;
        return std::nullopt;
    }
    if(parseResult.count("root"))
    {
        ret.includeDirectory = parseResult["root"].as<std::vector<std::string>>();
    }
    else
    {
        std::cout << options.help() << std::endl;
        return std::nullopt;
    }
    return ret;
}

std::string ShaderNameToNamespaceName(const std::string &shaderName)
{
    std::string ret = "_" + shaderName;
    for(char &c : ret)
    {
        if(!std::isalnum(c) && c != '_')
        {
            c = '_';
        }
    }
    ret += "_";
    for(char c : shaderName)
    {
        ret += fmt::format("{:0>2x}", static_cast<unsigned int>(c));
    }
    return ret;
}

std::string ToString(Rtrc::RHI::ShaderStageFlags stages)
{
    return GetShaderStageFlagsName(stages);
}

std::string_view BindingNameToTypeName(Rtrc::RHI::BindingType type)
{
    constexpr std::string_view NAMES[] =
    {
        "Texture",
        "RWTexture",
        "Buffer",
        "StructuredBuffer",
        "RWBuffer",
        "RWStructuredBuffer",
        "ConstantBuffer",
        "SamplerState",
        "RaytracingAccelerationStructure"
    };
    assert(std::to_underlying(type) < Rtrc::GetArraySize<int>(NAMES));
    return NAMES[std::to_underlying(type)];
}

void GenerateBindingGroupDefinition(
    Rtrc::SourceWriter                 &sw,
    const Rtrc::SC::ParsedBindingGroup &group,
    std::string_view                    name)
{
    sw("rtrc_group({})", name).NewLine();
    sw("{").NewLine();
    ++sw;

    for(size_t i = 0; i < group.bindings.size(); ++i)
    {
        auto &binding = group.bindings[i];
        if(group.isRef[i])
        {
            sw("rtrc_ref({}, {});", binding.name, ToString(binding.stages)).NewLine();
            continue;
        }

        const std::string_view typeStr = BindingNameToTypeName(binding.type);

        std::string templateParamStr;
        if(binding.type == Rtrc::RHI::BindingType::ConstantBuffer && !binding.templateParam.empty())
        {
            templateParamStr = fmt::format("<{}>", binding.templateParam);
        }

        std::string arraySizeStr;
        if(binding.arraySize)
        {
            arraySizeStr = fmt::format("[{}]", *binding.arraySize);
        }

        std::string_view head = "rtrc_define";
        if(binding.bindless)
        {
            if(binding.variableArraySize)
            {
                head = "rtrc_bindless_variable_size";
            }
            else
            {
                head = "rtrc_bindless";
            }
        }

        sw(
            "{}({}{}{}, {}, {});",
            head, typeStr, templateParamStr, arraySizeStr, binding.name, ToString(binding.stages)).NewLine();
    }

    --sw;
    sw("};").NewLine();
}

void GenerateKeywordAndBindingGroupsInShader(
    Rtrc::SourceWriter           &sw,
    const Rtrc::SC::RawShader    &rawShader,
    const Rtrc::SC::ParsedShader &parsedShader)
{
    std::cout << "Preprocessing shader " << rawShader.shaderName << std::endl;
    const std::string namespaceName = ShaderNameToNamespaceName(parsedShader.name);

    sw("namespace Rtrc::ShaderInfoDetail::{}", namespaceName).NewLine();
    sw("{").NewLine();
    ++sw;

    std::set<std::string> allBindingGroupNames;
    std::map<Rtrc::SC::ParsedBindingGroup, std::string> bindingGroupNames;
    auto &bgsw = sw.GetSubWritter();
    bgsw("namespace BindingGroups", namespaceName).NewLine();
    bgsw("{").NewLine();
    ++bgsw;

    // Keywords

    for(auto &keyword : parsedShader.keywords)
    {
        if(keyword.Is<Rtrc::SC::BoolShaderKeyword>())
        {
            continue;
        }
        auto &enumKeyword = keyword.As<Rtrc::SC::EnumShaderKeyword>();
        sw("enum class {} : uint32_t", enumKeyword.name).NewLine();
        sw("{").NewLine();
        ++sw;
        for(const std::string &keywordValue : enumKeyword.values)
        {
            sw("{},", keywordValue).NewLine();
        }
        --sw;
        sw("};").NewLine();
    }

    if(!parsedShader.keywords.empty())
    {
        sw("template<");
        for(size_t i = 0; i < parsedShader.keywords.size(); ++i)
        {
            if(i > 0)
            {
                sw(", ");
            }
            parsedShader.keywords[i].Match(
                [&](const Rtrc::SC::BoolShaderKeyword &keyword)
                {
                    sw("bool kw{}", keyword.name);
                },
                [&](const Rtrc::SC::EnumShaderKeyword &keyword)
                {
                    sw("{} kw{}", keyword.name, keyword.name);
                });
        }
        sw(">").NewLine();
        sw("struct Variant").NewLine();
        sw("{").NewLine();
        sw("};").NewLine();
    }

    for(size_t variantIndex = 0; variantIndex < parsedShader.variants.size(); ++variantIndex)
    {
        auto &variant = parsedShader.variants[variantIndex];
        const auto keywordValues = ExtractKeywordValues(parsedShader.keywords, variantIndex);

        if(!parsedShader.keywords.empty())
        {
            sw("template<>").NewLine();
        }
        sw("struct Variant");
        if(!parsedShader.keywords.empty())
        {
            sw("<");
            for(size_t keywordIndex = 0; keywordIndex < keywordValues.size(); ++keywordIndex)
            {
                if(keywordIndex > 0)
                {
                    sw(", ");
                }
                const int value = keywordValues[keywordIndex].value;
                parsedShader.keywords[keywordIndex].Match(
                    [&](const Rtrc::SC::BoolShaderKeyword &)
                    {
                        sw(value ? "true" : "false");
                    },
                    [&](const Rtrc::SC::EnumShaderKeyword &keyword)
                    {
                        sw("{}::{}", keyword.name, keyword.values[value]);
                    });
            }
            sw(">");
        }
        sw.NewLine();
        sw("{").NewLine();
        ++sw;

        for(auto &bindingGroup : variant.bindingGroups)
        {
            std::string bindingGroupName;
            if(auto it = bindingGroupNames.find(bindingGroup); it != bindingGroupNames.end())
            {
                bindingGroupName = it->second;
            }
            else
            {
                int nameSuffix = 0;
                while(true)
                {
                    bindingGroupName = fmt::format("{}_{}", bindingGroup.name, nameSuffix);
                    if(!allBindingGroupNames.contains(bindingGroupName))
                    {
                        break;
                    }
                    ++nameSuffix;
                }

                GenerateBindingGroupDefinition(bgsw, bindingGroup, bindingGroupName);
                bindingGroupNames.insert({ bindingGroup, bindingGroupName });
                allBindingGroupNames.insert(bindingGroupName);
            }
            sw("using {} = BindingGroups::{};", bindingGroup.name, bindingGroupName).NewLine();
        }

        --sw;
        sw("};").NewLine(); // struct VariantTrait
    }

    --bgsw;
    bgsw("}").NewLine(); // namespace BindingGroups

    --sw;
    sw("}").NewLine(); // namespace Rtrc::ShaderInfoDetail::ShaderNamespaceName

    sw("namespace Rtrc::ShaderDatabase").NewLine();
    sw("{").NewLine();
    ++sw;

    sw("template<>").NewLine();
    sw("struct ShaderInfo<TemplateStringParameter<\"{}\">>", rawShader.shaderName).NewLine();
    sw("{").NewLine();
    ++sw;

    for(auto &keyword : parsedShader.keywords)
    {
        if(auto kw = keyword.AsIf<Rtrc::SC::EnumShaderKeyword>())
        {
            sw(
                "using {} = Rtrc::ShaderInfoDetail::{}::{};",
                kw->name, namespaceName, kw->name).NewLine();
        }
    }

    std::string templateArgsStr;
    if(!parsedShader.keywords.empty())
    {
        sw("template<");
        templateArgsStr += "<";
        for(size_t i = 0; i < parsedShader.keywords.size(); ++i)
        {
            if(i > 0)
            {
                sw(", ");
                templateArgsStr += ", ";
            }
            parsedShader.keywords[i].Match(
                [&](const Rtrc::SC::BoolShaderKeyword &keyword)
                {
                    sw("bool kw{}", keyword.name);
                    templateArgsStr += "kw" + keyword.name;
                },
                [&](const Rtrc::SC::EnumShaderKeyword &keyword)
                {
                    sw("{} kw{}", keyword.name, keyword.name);
                    templateArgsStr += keyword.name + "kw" + keyword.name;
                });
        }
        sw(">").NewLine();
        templateArgsStr += ">";
    }
    sw(
        "using Variant = Rtrc::ShaderInfoDetail::{}::Variant{};",
        namespaceName, templateArgsStr).NewLine();

    --sw;
    sw("};").NewLine();

    --sw;
    sw("}").NewLine();
}

void GenerateShaderRegistration(
    Rtrc::SourceWriter                       &sw,
    const Rtrc::SC::ParsedShader             &parsedShader,
    const Rtrc::SC::ShaderCompileEnvironment &envir)
{
    sw("template<>").NewLine();
    sw("void _rtrcRegisterShaderInfo<__COUNTER__>(ShaderDatabase &database)").NewLine();
    sw("{").NewLine();
    ++sw;

    Rtrc::TextSerializer parsedShaderSerializer;
    parsedShaderSerializer(parsedShader, "shader");
    std::string parsedShaderString = parsedShaderSerializer.ResolveResult();

    constexpr int CHARS_PER_LINE = 40;

    // Parsed shader

    sw("static const char SERIALIZED_SHADER[] = {").NewLine();
    ++sw;
    for(size_t i = 0; i <= parsedShaderString.size() /* '\0' is also included */; i += CHARS_PER_LINE)
    {
        const size_t end = std::min(i + CHARS_PER_LINE, parsedShaderString.size());
        for(size_t j = i; j < end; ++j)
        {
            sw("{}, ", static_cast<int>(parsedShaderString[j]));
        }
        sw.NewLine();
    }
    --sw;
    sw("};").NewLine();

    sw("TextDeserializer shaderDeser;").NewLine();
    sw("ParsedShader shader;").NewLine();
    sw("shaderDeser.SetSource(std::string(SERIALIZED_SHADER));").NewLine();
    sw("shaderDeser(shader, \"shader\");").NewLine();

    sw("database.AddShader(shader);").NewLine();
    
    --sw;
    sw("}").NewLine();
}

int main(int argc, const char *argv[])
{
    const auto args = ParseCommandArguments(argc, argv);
    if(!args)
    {
        return 0;
    }

    struct FileRecord
    {
        std::vector<const Rtrc::SC::RawShader*> shaders;
    };
    std::map<std::string, FileRecord> fileRecords;

    const Rtrc::SC::RawShaderDatabase rawShaderDatabase = Rtrc::SC::CreateRawShaderDatabase({ args->inputFilename });
    for(const Rtrc::SC::RawShader &rawShader : rawShaderDatabase.rawShaders)
    {
        fileRecords[rawShader.filename].shaders.push_back(&rawShader);
    }

    Rtrc::SC::DXC dxc;
    Rtrc::SC::ShaderCompileEnvironment envir;
    for(auto &inc : args->includeDirectory)
    {
        envir.includeDirs.push_back(std::filesystem::absolute(inc).lexically_normal().string());
    }

    for(const auto &[path, file] : fileRecords)
    {
        std::cout << "Preprocessing file " << path << std::endl;

        std::vector<Rtrc::SC::ParsedShader> parsedShaders;
        for(const Rtrc::SC::RawShader *rawShader : file.shaders)
        {
            parsedShaders.push_back(ParseShader(envir, &dxc, *rawShader));
        }

        Rtrc::SourceWriter sw;
        sw("#pragma once").NewLine(2);

        sw("#if ENABLE_SHADER_INFO").NewLine();
        sw("#include <Core/TemplateStringParameter.h>").NewLine();
        sw("#include <Graphics/Device/BindingGroupDSL.h>").NewLine();
        sw("#include <ShaderCompiler/Database/ShaderInfo.h>").NewLine();
        for(size_t i = 0; i < parsedShaders.size(); ++i)
        {
            GenerateKeywordAndBindingGroupsInShader(sw, *file.shaders[i], parsedShaders[i]);
        }
        sw("#endif // #if ENABLE_SHADER_INFO").NewLine();

        sw("#if ENABLE_SHADER_REGISTRATION").NewLine();
        for(size_t i = 0; i < parsedShaders.size(); ++i)
        {
            GenerateShaderRegistration(sw, parsedShaders[i], envir);
        }
        sw("#endif // #if ENABLE_SHADER_REGISTRATION").NewLine();

        const std::string content = sw.ResolveResult();
        const auto outputFile = absolute(std::filesystem::path(args->outputFilename));
        
        create_directories(outputFile.parent_path());
        Rtrc::File::WriteTextFile(outputFile.string(), content);
    }
}
