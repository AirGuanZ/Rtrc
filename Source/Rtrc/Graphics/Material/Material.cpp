#include <fmt/format.h>

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Shader/ShaderBindingParser.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/String.h>

RTRC_BEGIN

namespace
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
                throw Exception(fmt::format("{} name expected after '{}'", keyword, keyword));
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

} // namespace anonymous

SubMaterial::SubMaterial()
{
    static std::atomic<uint32_t> nextInstanceID = 0;
    subMaterialInstanceID_ = nextInstanceID++;
}

uint32_t SubMaterial::GetSubMaterialInstanceID() const
{
    return subMaterialInstanceID_;
}

const std::set<std::string> &SubMaterial::GetTags() const
{
    return tags_;
}

RC<Shader> SubMaterial::GetShader(const KeywordValueContext &keywordValues)
{
    KeywordSet::ValueMask mask = shaderTemplate_->GetKeywordValueMask(keywordValues);
    mask = overrideKeywords_.Apply(mask);
    return shaderTemplate_->GetShader(mask);
}

const std::string &Material::GetName() const
{
    return name_;
}

RC<SubMaterial> Material::GetSubMaterialByIndex(int index)
{
    return subMaterials_[index];
}

RC<SubMaterial> Material::GetSubMaterialByTag(std::string_view tag)
{
    const int index = GetSubMaterialIndexByTag(tag);
    return GetSubMaterialByIndex(index);
}

int Material::GetSubMaterialIndexByTag(std::string_view tag) const
{
    auto it = tagToIndex_.find(tag);
    if(it == tagToIndex_.end())
    {
        throw Exception(fmt::format("Tag {} not found in material {}", tag, name_));
    }
    return it->second;
}

MaterialManager::MaterialManager()
{
    debug_ = RTRC_DEBUG;
}

void MaterialManager::SetDevice(RHI::DevicePtr device)
{
    device_ = device;
    shaderCompiler_.SetDevice(std::move(device));
}

void MaterialManager::SetRootDirectory(std::string_view directory)
{
    shaderCompiler_.SetRootDirectory(directory);

    std::vector<std::string> shaderFilenames;
    std::vector<std::string> materialFilenames;

    for(const auto &p : std::filesystem::recursive_directory_iterator(directory))
    {
        if(p.is_regular_file())
        {
            const std::string ext = ToLower(p.path().extension().string());
            if(ext == ".material")
            {
                auto filename = absolute(p.path()).lexically_normal().string();
                materialFilenames.push_back(filename);
                shaderFilenames.push_back(std::move(filename));
            }
            else if(ext == ".shader")
            {
                shaderFilenames.push_back(absolute(p.path()).lexically_normal().string());
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

RC<Material> MaterialManager::GetMaterial(std::string_view name)
{
    assert(materialNameToFilename_.contains(name));
    return materialPool_.GetOrCreate(name, [&] { return CreateMaterial(name); });
}

RC<ShaderTemplate> MaterialManager::GetShaderTemplate(std::string_view name)
{
    assert(shaderNameToFilename_.contains(name));
    return shaderPool_.GetOrCreate(name, [&] { return CreateShaderTemplate(name); });
}

RC<BindingGroupLayout> MaterialManager::GetBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc)
{
    return shaderCompiler_.GetBindingGroupLayout(desc);
}

void MaterialManager::GC()
{
    materialPool_.GC();
    shaderPool_.GC();
}

void MaterialManager::ProcessShaderFile(int filenameIndex, const std::string &filename)
{
    std::string source = File::ReadTextFile(filename);
    ShaderPreprocess::RemoveComments(source);

    size_t begPos = 0;
    while (true)
    {
        std::string name; size_t shaderBeg, shaderEnd;
        if (!FindSection("Shader", source, begPos, name, shaderBeg, shaderEnd))
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
        if(!FindSection("Material", source, begPos, name, matBeg, matEnd))
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

    std::vector<RC<SubMaterial>> subMaterials;
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
            subMaterials.push_back(ParsePass(tokens));
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
        else if(tokens.IsFinished())
        {
            break;
        }
        else
        {
            tokens.Throw(fmt::format("unexpected token '{}'", tokens.GetCurrentToken()));
        }
    }

    auto material = MakeRC<Material>();
    for(auto &&[index, subMaterial] : Enumerate(subMaterials))
    {
        for(auto &tag : subMaterial->GetTags())
        {
            if(material->tagToIndex_.contains(tag))
            {
                throw Exception(fmt::format("Tag {} exists in multiple passes of material {}", tag, name));
            }
            material->tagToIndex_.insert({ tag, static_cast<int>(index) });
        }
    }
    material->name_ = name;
    material->subMaterials_ = std::move(subMaterials);
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

    auto shaderTemplate = MakeRC<ShaderTemplate>();
    shaderTemplate->debug_           = debug_;
    shaderTemplate->keywordSet_      = std::move(keywordSet);
    shaderTemplate->source_.source   = source;
    shaderTemplate->source_.filename = filenames_[fileRef.filenameIndex];
    shaderTemplate->source_.VSEntry  = std::move(VSEntry);
    shaderTemplate->source_.FSEntry  = std::move(FSEntry);
    shaderTemplate->source_.CSEntry  = std::move(CSEntry);
    shaderTemplate->shaderCompiler_  = &shaderCompiler_;

    return shaderTemplate;
}

RC<SubMaterial> MaterialManager::ParsePass(ShaderTokenStream &tokens)
{
    // TODO: partial keyword values

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

    auto subMaterial = MakeRC<SubMaterial>();
    subMaterial->tags_ = std::move(passTags);
    subMaterial->shaderTemplate_ = std::move(shaderTemplate);
    return subMaterial;
}

RTRC_END
