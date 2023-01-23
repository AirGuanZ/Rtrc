#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Graphics/Shader/ShaderBindingParser.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/File.h>
#include <Rtrc/Utility/String.h>

RTRC_BEGIN

namespace MaterialDetail
{

    // Parse "keyword { ... }"
    bool FindSection(
        std::string_view   keyword,
        const std::string &source,
        size_t             begPos,
        std::string       &outName,
        size_t            &outBegPos,
        size_t            &outEndPos)
    {
        while (true)
        {
            const size_t matKeywordPos = source.find(keyword, begPos);
            if (matKeywordPos == std::string::npos)
            {
                return false;
            }

            // Check for non-identifier char

            auto IsIdentifierChar = [](char c)
            {
                return std::isalnum(c) || c == '_';
            };

            if ((matKeywordPos > 0 && IsIdentifierChar(source[matKeywordPos - 1])) ||
                IsIdentifierChar(source[matKeywordPos + keyword.size()]))
            {
                begPos = matKeywordPos + keyword.size();
                continue;
            }

            // Skip whitespaces

            size_t nameBegPos = matKeywordPos + keyword.size();
            while (std::isspace(source[nameBegPos]))
            {
                ++nameBegPos;
            }

            // Get name

            if (source[nameBegPos] != '"' && source[nameBegPos != '\''])
            {
                begPos = matKeywordPos + keyword.size();
                continue;
                //throw Exception(fmt::format("{} name expected after '{}'", keyword, keyword));
            }
            const char delimiter = source[nameBegPos++];
            size_t nameEndPos = nameBegPos;

            while (nameEndPos < source.size() && source[nameEndPos] != '\n' && source[nameEndPos] != delimiter)
            {
                ++nameEndPos;
            }
            if (source[nameEndPos] != delimiter)
            {
                throw Exception(fmt::format("Unclosed {} name string", keyword));
            }
            const std::string name = source.substr(nameBegPos, nameEndPos - nameBegPos);

            // Skip whitespaces

            size_t leftBracePos = nameEndPos + 1;
            while (std::isspace(source[leftBracePos]))
            {
                ++leftBracePos;
            }

            // Check '{'

            if (source[leftBracePos] != '{')
            {
                throw Exception("'{' expected");
            }

            // Find enclosing '}'

            size_t nestedCount = 1, rightBracePos = leftBracePos;
            while (true)
            {
                ++rightBracePos;
                if (rightBracePos >= source.size())
                {
                    throw Exception("Enclosing '}' not found");
                }
                if (source[rightBracePos] == '{')
                {
                    ++nestedCount;
                }
                else if (source[rightBracePos] == '}' && !--nestedCount)
                {
                    break;
                }
            }
            assert(source[rightBracePos] == '}');

            outName = std::move(name);
            outBegPos = leftBracePos + 1;
            outEndPos = rightBracePos;
            return true;
        }
    }

} // namespace MaterialDetail

MaterialManager::MaterialManager()
{
    device_ = nullptr;
    debug_ = RTRC_DEBUG;
}

void MaterialManager::SetDevice(Device *device)
{
    assert(device);
    device_ = device;
    shaderCompiler_.SetDevice(device);
}

void MaterialManager::AddIncludeDirectory(std::string_view directory)
{
    shaderCompiler_.AddIncludeDirectory(directory);
}

void MaterialManager::AddFiles(const std::set<std::filesystem::path> &filenames)
{
    std::vector<std::string> shaderFilenames;
    std::vector<std::string> materialFilenames;

    for(const auto &p : filenames)
    {
        if(is_regular_file(p))
        {
            const std::string ext = ToLower(p.extension().string());
            if(ext == ".material")
            {
                auto filename = absolute(p).lexically_normal().string();
                materialFilenames.push_back(filename);
                shaderFilenames.push_back(std::move(filename));
            }
            else if(ext == ".shader")
            {
                shaderFilenames.push_back(absolute(p).lexically_normal().string());
            }
        }
    }

    std::map<std::string, int> filenameToIndex;
    for(auto &filename : shaderFilenames)
    {
        const int index = static_cast<int>(filenames_.size());
        filenameToIndex.insert({ filename, index });
        filenames_.push_back(filename);
    }

    for(auto &filename : shaderFilenames)
    {
        ProcessShaderFile(filenameToIndex.at(filename), filename);
    }
    for(auto &filename : materialFilenames)
    {
        ProcessMaterialFile(filenameToIndex.at(filename), filename);
    }
}

void MaterialManager::SetDebugMode(bool debug)
{
    debug_ = debug;
}

RC<Material> MaterialManager::GetMaterial(const std::string &name)
{
    //assert(materialNameToFilename_.contains(name));
    return materialPool_.GetOrCreate(name, [&] { return CreateMaterial(name); });
}

RC<ShaderTemplate> MaterialManager::GetShaderTemplate(const std::string &name)
{
    //assert(shaderNameToFilename_.contains(name));
    return shaderPool_.GetOrCreate(name, [&] { return CreateShaderTemplate(name); });
}

RC<Shader> MaterialManager::GetShader(const std::string &name)
{
    auto shaderTemplate = GetShaderTemplate(name);
    assert(shaderTemplate->GetKeywordSet().GetTotalBitCount() == 0);
    return shaderTemplate->GetShader(0);
}

RC<MaterialInstance> MaterialManager::CreateMaterialInstance(const std::string &name)
{
    return GetMaterial(name)->CreateInstance();
}

void MaterialManager::ProcessShaderFile(int filenameIndex, const std::string &filename)
{
    std::string source = File::ReadTextFile(filename);
    ShaderPreprocess::RemoveComments(source);

    size_t begPos = 0;
    while (true)
    {
        std::string name; size_t shaderBeg, shaderEnd;
        if (!MaterialDetail::FindSection("Shader", source, begPos, name, shaderBeg, shaderEnd))
        {
            break;
        }
        if (shaderNameToFilename_.contains(name))
        {
            throw Exception("Repeated shader name: " + name);
        }
        shaderNameToFilename_.insert(
            { std::move(name), FileReference{ filenameIndex, shaderBeg, shaderEnd } });
        begPos = shaderEnd + 1;
    }
}

void MaterialManager::ProcessMaterialFile(int filenameIndex, const std::string &filename)
{
    std::string source = File::ReadTextFile(filename);
    ShaderPreprocess::RemoveComments(source);

    size_t begPos = 0;
    while(true)
    {
        std::string name; size_t matBeg, matEnd;
        if(!MaterialDetail::FindSection("Material", source, begPos, name, matBeg, matEnd))
        {
            break;
        }
        if(materialNameToFilename_.contains(name))
        {
            throw Exception("Repeated material name: " + name);
        }
        materialNameToFilename_.insert(
            { std::move(name), FileReference{ filenameIndex, matBeg, matEnd } });
        begPos = matEnd + 1;
    }
}

RC<Material> MaterialManager::CreateMaterial(std::string_view name)
{
    auto it = materialNameToFilename_.find(name);
    if(it == materialNameToFilename_.end())
    {
        throw Exception(fmt::format("Unknown material '{}'", name));
    }
    const FileReference &fileRef = it->second;

    std::string source = File::ReadTextFile(filenames_[fileRef.filenameIndex]);
    source = source.substr(0, fileRef.endPos);
    ShaderPreprocess::RemoveComments(source);
    ShaderTokenStream tokens(source, fileRef.beginPos, ShaderTokenStream::ErrorMode::Material);

    std::vector<MaterialProperty> properties;
    std::vector<RC<MaterialPass>> passes;

    using enum MaterialProperty::Type;
    static const std::map<std::string, MaterialProperty::Type, std::less<>> NAME_TO_PROPERTY_TYPE =
    {
        { "float",        Float     },
        { "float2",       Float2    },
        { "float3",       Float3    },
        { "float4",       Float4    },
        { "uint",         UInt      },
        { "uint2",        UInt2     },
        { "uint3",        UInt3     },
        { "uint4",        UInt4     },
        { "int",          Int       },
        { "int2",         Int2      },
        { "int3",         Int3      },
        { "int4",         Int4      },
        { "Buffer",       Buffer    },
        { "Texture2D",    Texture2D },
        { "SamplerState", Sampler   }
    };

    while(true)
    {
        if(tokens.GetCurrentToken() == "Pass")
        {
            tokens.Next();
            if(tokens.GetCurrentToken() != "{")
            {
                tokens.Throw("'{' expected");
            }
            tokens.Next();
            passes.push_back(ParsePass(tokens));
            if(tokens.GetCurrentToken() != "}")
            {
                tokens.Throw("'}' expected");
            }
            tokens.Next();
            if(tokens.GetCurrentToken() == ";")
            {
                tokens.Next();
            }
        }
        else if(auto jt = NAME_TO_PROPERTY_TYPE.find(tokens.GetCurrentToken()); jt != NAME_TO_PROPERTY_TYPE.end())
        {
            properties.push_back(ParseProperty(jt->second, tokens));
        }
        else if(tokens.IsFinished())
        {
            break;
        }
        else
        {
            tokens.Throw(fmt::format("unexpected token '{}'", tokens.GetCurrentToken()));
        }
    }

    // Material properties

    auto propertyLayout = MakeRC<MaterialPropertyHostLayout>(std::move(properties));

    std::map<std::string, std::string> propMacros;
    for(auto &prop : propertyLayout->GetValueProperties())
    {
        const char *valueTypeName = prop.GetTypeName();
        propMacros.insert(
        {
            fmt::format("__PER_MATERIAL_VALUE_PROPERTY_{}", prop.name),
            fmt::format("{} {}", valueTypeName, prop.name)
        });
    }
    propMacros.insert({ "MATERIAL_VALUE_PROPERTY(X)", "__PER_MATERIAL_VALUE_PROPERTY_##X" });

    auto sharedCompileEnvir = MakeRC<ShaderTemplate::SharedCompileEnvironment>();
    sharedCompileEnvir->macros = std::move(propMacros);
    for(auto &pass : passes)
    {
        pass->parentPropertyLayout_ = propertyLayout.get();
        pass->shaderTemplate_->sharedEnvir_ = sharedCompileEnvir;
    }

    // Material tag

    auto material = MakeRC<Material>();
    for(auto &&[index, pass] : Enumerate(passes))
    {
        for(auto &tag : pass->GetTags())
        {
            if(material->tagToIndex_.contains(tag))
            {
                throw Exception(fmt::format("Tag {} exists in multiple passes of material {}", tag, name));
            }
            material->tagToIndex_.insert({ tag, static_cast<int>(index) });
        }
    }
    material->device_ = device_;
    material->name_ = name;
    material->passes_ = std::move(passes);
    material->propertyLayout_ = std::move(propertyLayout);

    return material;
}

RC<ShaderTemplate> MaterialManager::CreateShaderTemplate(std::string_view name)
{
    auto it = shaderNameToFilename_.find(name);
    if(it == shaderNameToFilename_.end())
    {
        throw Exception(fmt::format("Unknown shader '{}'", name));
    }
    const FileReference &fileRef = it->second;

    std::string source = File::ReadTextFile(filenames_[fileRef.filenameIndex]);
    ShaderPreprocess::RemoveComments(source);
    for(size_t i = 0; i < fileRef.beginPos; ++i)
    {
        if(source[i] != '\n')
        {
            source[i] = ' ';
        }
    }
    source = source.substr(0, fileRef.endPos);
    
    std::string VSEntry, FSEntry, CSEntry;
    for(auto s : { "vertex", "vert" })
    {
        if(ShaderPreprocess::ParseEntryPragma(source, s, VSEntry))
        {
            break;
        }
    }
    for(auto &s : { "fragment", "frag", "pixel" })
    {
        if(ShaderPreprocess::ParseEntryPragma(source, s, FSEntry))
        {
            break;
        }
    }
    for(auto &s : { "compute", "comp" })
    {
        if(ShaderPreprocess::ParseEntryPragma(source, s, CSEntry))
        {
            break;
        }
    }

    std::vector<std::string> keywordStrings;
    KeywordSet keywordSet;
    ShaderPreprocess::ParseKeywords(source, keywordStrings);
    for(auto &s : keywordStrings)
    {
        keywordSet.AddKeyword(Keyword(s), 1);
    }
    const int totalKeywordBitCount = keywordSet.GetTotalBitCount();

    auto shaderTemplate = MakeRC<ShaderTemplate>();
    shaderTemplate->debug_           = debug_;
    shaderTemplate->keywordSet_      = std::move(keywordSet);
    shaderTemplate->source_.source   = source;
    shaderTemplate->source_.filename = filenames_[fileRef.filenameIndex];
    shaderTemplate->source_.vertexEntry  = std::move(VSEntry);
    shaderTemplate->source_.fragmentEntry  = std::move(FSEntry);
    shaderTemplate->source_.computeEntry  = std::move(CSEntry);
    shaderTemplate->shaderCompiler_  = &shaderCompiler_;
    shaderTemplate->compiledShaders_.resize(1 << totalKeywordBitCount);

    return shaderTemplate;
}

RC<MaterialPass> MaterialManager::ParsePass(ShaderTokenStream &tokens)
{
    // Parse pass source

    std::set<std::string> passTags;
    RC<ShaderTemplate> shaderTemplate;

    auto ParseShader = [&]
    {
        if(shaderTemplate)
        {
            tokens.Throw("More than one shaders found in a material pass");
        }

        assert(tokens.GetCurrentToken() == "Shader");
        tokens.Next();

        std::string name(tokens.GetCurrentToken());
        if(name[0] != '\'' && name[0] != '"')
        {
            tokens.Throw("Shader name expected");
        }
        assert(name.size() >= 2);
        name = name.substr(1, name.size() - 2);
        tokens.Next();

        if(tokens.GetCurrentToken() != "{")
        {
            tokens.Throw("'{' expected");
        }
        shaderTemplate = GetShaderTemplate(name);
        const FileReference &fileRef = shaderNameToFilename_.at(name);
        tokens.SetStartPosition(fileRef.endPos);
        assert(tokens.GetCurrentToken() == "}");
        tokens.Next();
        if(tokens.GetCurrentToken() == ";")
        {
            tokens.Next();
        }
    };

    auto ParseShaderRef = [&]
    {
        if(shaderTemplate)
        {
            tokens.Throw("More than one shaders found in a material pass");
        }

        assert(tokens.GetCurrentToken() == "ShaderRef");
        tokens.Next();

        std::string name(tokens.GetCurrentToken());
        if(name[0] != '\'' && name[0] != '"')
        {
            tokens.Throw("Shader name expected");
        }
        assert(name.size() >= 2);
        name = name.substr(1, name.size() - 2);
        tokens.Next();

        if(tokens.GetCurrentToken() == ";")
        {
            tokens.Next();
        }
        shaderTemplate = GetShaderTemplate(name);
    };

    auto ParseTags = [&]
    {
        assert(tokens.GetCurrentToken() == "Tag");
        tokens.Next();

        while(true)
        {
            std::string_view name(tokens.GetCurrentToken());
            if(name[0] != '"')
            {
                break;
            }
            assert(name.size() >= 3);
            name = name.substr(1, name.size() - 2);
            passTags.insert(std::string(name));
            tokens.Next();
        }

        if(tokens.GetCurrentToken() == ";")
        {
            tokens.Next();
        }
    };

    while(true)
    {
        if(tokens.GetCurrentToken() == "Shader")
        {
            ParseShader();
        }
        else if(tokens.GetCurrentToken() == "ShaderRef")
        {
            ParseShaderRef();
        }
        else if(tokens.GetCurrentToken() == "Tag")
        {
            ParseTags();
        }
        else
        {
            break;
        }
    }

    if(!shaderTemplate)
    {
        tokens.Throw("Shader not found in material pass");
    }

    auto pass = MakeRC<MaterialPass>();
    pass->tags_ = std::move(passTags);
    pass->shaderTemplate_ = std::move(shaderTemplate);
    pass->materialPassPropertyLayouts_.resize(
        1 << pass->shaderTemplate_->GetKeywordSet().GetTotalBitCount());
    return pass;
}

MaterialProperty MaterialManager::ParseProperty(MaterialProperty::Type propertyType, ShaderTokenStream &tokens)
{
    tokens.Next();

    // Property name
    if (!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
    {
        tokens.Throw("Property name expected");
    }
    std::string propertyName = tokens.GetCurrentToken();
    tokens.Next();

    // Optional ";"
    if (tokens.GetCurrentToken() == ";")
    {
        tokens.Next();
    }

    MaterialProperty prop;
    prop.type = propertyType;
    prop.name = std::move(propertyName);
    return prop;
}

RTRC_END
