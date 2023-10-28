#include <fstream>
#include <iostream>

#include <cxxopts.hpp>

#include <Core/Filesystem/File.h>
#include <Core/Serialization/TextSerializer.h>
#include <Core/SourceWriter.h>
#include <Core/String.h>
#include <ShaderCommon/DXC/DXC.h>
#include <ShaderCommon/Parser/ShaderParser.h>

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
    Rtrc::SourceWriter             &sw,
    const Rtrc::ParsedBindingGroup &group,
    std::string_view                name)
{
    const std::string uniformStages = GetShaderStageFlagsName(group.stages);
    sw("rtrc_group({}, {})", name, uniformStages).NewLine();
    sw("{").NewLine();
    ++sw;

    for(size_t i = 0; i < group.bindings.size(); ++i)
    {
        auto &binding = group.bindings[i];

        const std::string_view typeStr = binding.rawTypeName;

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

    for(size_t i = 0; i < group.uniformPropertyDefinitions.size(); ++i)
    {
        auto &uniform = group.uniformPropertyDefinitions[i];
        sw("rtrc_uniform({}, {});", uniform.type, uniform.name).NewLine();
    }

    --sw;
    sw("};").NewLine();
}

void GenerateKeywordAndBindingGroupsInShader(
    Rtrc::SourceWriter       &sw,
    const Rtrc::RawShader    &rawShader,
    const Rtrc::ParsedShader &parsedShader)
{
    std::cout << "Preprocessing shader " << rawShader.shaderName << std::endl;
    const std::string namespaceName = ShaderNameToNamespaceName(parsedShader.name);

    sw("#ifndef RTRC_STATIC_SHADER_INFO_{}", namespaceName).NewLine();
    sw("#define RTRC_STATIC_SHADER_INFO_{}", namespaceName).NewLine();

    sw("namespace Rtrc::ShaderInfoDetail::{}", namespaceName).NewLine();
    sw("{").NewLine();
    ++sw;

    std::set<std::string> allBindingGroupNames;
    std::map<Rtrc::ParsedBindingGroup, std::string> bindingGroupNames;
    auto &bgsw = sw.GetSubWritter();
    bgsw("namespace BindingGroups", namespaceName).NewLine();
    bgsw("{").NewLine();
    ++bgsw;

    // Keywords

    for(auto &keyword : parsedShader.keywords)
    {
        if(keyword.Is<Rtrc::BoolShaderKeyword>())
        {
            continue;
        }
        auto &enumKeyword = keyword.As<Rtrc::EnumShaderKeyword>();
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
                [&](const Rtrc::BoolShaderKeyword &keyword)
                {
                    sw("bool kw{}", keyword.name);
                },
                [&](const Rtrc::EnumShaderKeyword &keyword)
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
                    [&](const Rtrc::BoolShaderKeyword &)
                    {
                        sw(value ? "true" : "false");
                    },
                    [&](const Rtrc::EnumShaderKeyword &keyword)
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

    sw("namespace Rtrc").NewLine();
    sw("{").NewLine();
    ++sw;

    sw("template<>").NewLine();
    sw("struct StaticShaderInfo<TemplateStringParameter(\"{}\")>", rawShader.shaderName).NewLine();
    sw("{").NewLine();
    ++sw;

    for(auto &keyword : parsedShader.keywords)
    {
        if(auto kw = keyword.AsIf<Rtrc::EnumShaderKeyword>())
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
                [&](const Rtrc::BoolShaderKeyword &keyword)
                {
                    sw("bool kw{}", keyword.name);
                    templateArgsStr += "kw" + keyword.name;
                },
                [&](const Rtrc::EnumShaderKeyword &keyword)
                {
                    sw("{} kw{}", keyword.name, keyword.name);
                    templateArgsStr += "kw" + keyword.name;
                });
        }
        sw(">").NewLine();
        templateArgsStr += ">";
    }
    sw(
        "using Variant = Rtrc::ShaderInfoDetail::{}::Variant{};",
        namespaceName, templateArgsStr).NewLine();

    if(parsedShader.keywords.empty())
    {
        assert(parsedShader.variants.size() == 1);
        for(auto &bindingGroup : parsedShader.variants[0].bindingGroups)
        {
            sw("using {} = Variant::{};", bindingGroup.name, bindingGroup.name).NewLine();
        }
    }

    --sw;
    sw("};").NewLine();

    --sw;
    sw("}").NewLine();

    sw("#endif // #ifndef RTRC_STATIC_SHADER_INFO_{}", namespaceName).NewLine();
}

void FillStringAsCharArray(Rtrc::SourceWriter &sw, std::string_view varName, std::string_view data)
{
    constexpr int CHARS_PER_LINE = 512;
    sw("static const char {}[] = {{", varName).NewLine();
    ++sw;
    for(size_t i = 0; i < data.size(); i += CHARS_PER_LINE)
    {
        const size_t end = std::min(i + CHARS_PER_LINE, data.size());
        for(size_t j = i; j < end; ++j)
        {
            sw("0x{:0>2x}, ", static_cast<int>(data[j]));
        }
        sw.NewLine();
    }
    if(!data.empty())
    {
        sw("0").NewLine();
    }
    --sw;
    sw("};").NewLine();
}

void GenerateShaderRegistration(Rtrc::SourceWriter &sw, const Rtrc::ParsedShader &parsedShader)
{
    sw("template<>").NewLine();
    sw("void _rtrcRegisterShaderInfo<__COUNTER__>(Rtrc::ShaderDatabase &database)").NewLine();
    sw("{").NewLine();
    ++sw;

    Rtrc::TextSerializer parsedShaderSerializer;
    parsedShaderSerializer(parsedShader, "shader");
    const std::string parsedShaderString = parsedShaderSerializer.ResolveResult();
    FillStringAsCharArray(sw, "SERIALIZED_SHADER", parsedShaderString);

    sw("Rtrc::TextDeserializer shaderDeser;").NewLine();
    sw("Rtrc::ParsedShader shader;").NewLine();
    sw("shaderDeser.SetSource(std::string(SERIALIZED_SHADER));").NewLine();
    sw("shaderDeser(shader, \"shader\");").NewLine();

    sw("database.AddShader(shader);").NewLine();
    
    --sw;
    sw("}").NewLine();
}

void GenerateMaterialRegistration(Rtrc::SourceWriter &sw, const std::vector<Rtrc::RawMaterialRecord> &materials)
{
    sw("template<>").NewLine();
    sw("void _rtrcRegisterMaterial<__COUNTER__>(Rtrc::MaterialManager &manager)").NewLine();
    sw("{").NewLine();
    ++sw;

    Rtrc::TextSerializer ser;
    ser(materials, "materials");
    const std::string materialsString = ser.ResolveResult();
    FillStringAsCharArray(sw, "SERIALIZED_MATERIALS", materialsString);

    sw("Rtrc::TextDeserializer deser;").NewLine();
    sw("std::vector<Rtrc::RawMaterialRecord> materials;").NewLine();
    sw("deser.SetSource(std::string(SERIALIZED_MATERIALS));").NewLine();
    sw("deser(materials, \"materials\");").NewLine();

    sw("for(auto &m : materials)").NewLine();
    sw("{").NewLine();
    ++sw;
    sw("manager.AddMaterial(m);").NewLine();
    --sw;
    sw("}").NewLine();

    --sw;
    sw("}").NewLine();
}

void WriteTxt(const std::string &filename, const std::string &content)
{
    auto parentDir = absolute(std::filesystem::path(filename)).parent_path();
    if(!exists(parentDir))
        create_directories(parentDir);

    {
        std::ifstream fin(filename, std::ifstream::in);
        if(fin)
        {
            const std::string oldContent
            {
                std::istreambuf_iterator<char>(fin),
                std::istreambuf_iterator<char>()
            };
            if(oldContent == content)
            {
                return;
            }
        }
    }

    std::ofstream fout(filename, std::ofstream::out | std::ofstream::trunc);
    if(fout)
    {
        fout << content;
    }
}

void Run(int argc, const char *argv[])
{
    const auto args = ParseCommandArguments(argc, argv);
    if(!args)
    {
        return;
    }

    Rtrc::SourceWriter sw;
    
    {
        const auto rawShaderDatabase = Rtrc::CreateRawShaderDatabase({ args->inputFilename });

        Rtrc::DXC dxc;
        Rtrc::ShaderCompileEnvironment envir;
        for(auto &inc : args->includeDirectory)
        {
            envir.includeDirs.push_back(std::filesystem::absolute(inc).lexically_normal().string());
        }

        std::vector<Rtrc::ParsedShader> parsedShaders;
        for(const Rtrc::RawShader &rawShader : rawShaderDatabase.rawShaders)
        {
            parsedShaders.push_back(ParseShader(envir, &dxc, rawShader));
        }

        sw("/*=============================================================================").NewLine();
        sw("    Generated by Rtrc shader preprocessor. Don't modify this file directly.").NewLine();
        sw("=============================================================================*/").NewLine();
        sw.NewLine();

        sw("#if !RTRC_REFLECTION_TOOL").NewLine();

        sw("#if !ENABLE_SHADER_REGISTRATION && !ENABLE_MATERIAL_REGISTRATION").NewLine();
        sw("#include <Core/TemplateStringParameter.h>").NewLine();
        sw("#include <Graphics/Device/BindingGroupDSL.h>").NewLine();
        sw("#include <Graphics/Shader/ShaderInfo.h>").NewLine();
        for(size_t i = 0; i < parsedShaders.size(); ++i)
        {
            GenerateKeywordAndBindingGroupsInShader(sw, rawShaderDatabase.rawShaders[i], parsedShaders[i]);
        }
        sw("#endif // #if !ENABLE_SHADER_REGISTRATION && !ENABLE_MATERIAL_REGISTRATION").NewLine();

        sw("#if ENABLE_SHADER_REGISTRATION").NewLine();
        for(size_t i = 0; i < parsedShaders.size(); ++i)
        {
            GenerateShaderRegistration(sw, parsedShaders[i]);
        }
        sw("#endif // #if ENABLE_SHADER_REGISTRATION").NewLine();
    }

    if(args->inputFilename.ends_with(".material"))
    {
        const std::string fileContent = Rtrc::File::ReadTextFile(args->inputFilename);
        std::vector<Rtrc::RawMaterialRecord> rawMaterials;
        std::vector<Rtrc::RawShader> rawShaders;
        Rtrc::ParseRawMaterials(fileContent, rawMaterials, rawShaders);
        sw("#if ENABLE_MATERIAL_REGISTRATION").NewLine();
        GenerateMaterialRegistration(sw, rawMaterials);
        sw("#endif // #if ENABLE_MATERIAL_REGISTRATION").NewLine();
    }

    sw("#endif // #if !RTRC_REFLECTION_TOOL").NewLine();

    const std::string content = sw.ResolveResult();
    const auto outputFile = absolute(std::filesystem::path(args->outputFilename));
    WriteTxt(outputFile.string(), content);
}

int main(int argc, const char *argv[])
{
    try
    {
        Run(argc, argv);
    }
    catch(const Rtrc::Exception &e)
    {
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        Rtrc::LogError("{}\n{}", e.what(), e.stacktrace());
#else
        Rtrc::LogErrorUnformatted(e.what());
#endif
        return -1;
    }
    catch(const std::exception &e)
    {
        Rtrc::LogErrorUnformatted(e.what());
        return -1;
    }
}
