#include <algorithm>
#include <ranges>

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Shader/ShaderBindingParser.h>
#include <Rtrc/Graphics/Shader/ShaderReflection.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/File.h>
#include <Rtrc/Utils/String.h>
#include <Rtrc/Utils/Unreachable.h>

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

} // namespace anonymous

MaterialPropertyHostLayout::MaterialPropertyHostLayout(std::vector<MaterialProperty> properties)
    : sortedProperties_(std::move(properties))
{
    std::ranges::partition(sortedProperties_, [&](const MaterialProperty &a){ return a.IsValue(); });

    valueBufferSize_ = 0;
    for(auto &p : sortedProperties_)
    {
        if(!p.IsValue())
        {
            break;
        }
        valueIndexToOffset_.push_back(valueBufferSize_);
        valueBufferSize_ += p.GetValueSize();
    }

    for(auto &&[index, prop] : Enumerate(sortedProperties_))
    {
        nameToIndex_.insert({ prop.name, static_cast<int>(index) });
    }
}

SubMaterialPropertyLayout::SubMaterialPropertyLayout(const MaterialPropertyHostLayout &materialPropertyLayout, const Shader &shader)
    : constantBufferSize_(0), constantBufferIndexInBindingGroup_(-1), bindingGroupIndex_(-1)
{
    // Constant buffer

    const ShaderConstantBuffer *cbuffer = nullptr;
    for(auto &cb : shader.GetConstantBuffers())
    {
        if(cb.name == "Material")
        {
            cbuffer = &cb;
            break;
        }
    }

    bindingGroupIndex_ = shader.TryGetBindingGroupIndexByName("Material");
    if(bindingGroupIndex_ >= 0)
    {
        bindingGroupLayout_ = shader.GetBindingGroupLayoutByIndex(bindingGroupIndex_);
    }

    if(cbuffer)
    {
        if(cbuffer->group != bindingGroupIndex_)
        {
            throw Exception("Constant buffer 'Material' must be defined in 'Material' binding group");
        }

        int dwordOffset = 0;
        for(auto &member : cbuffer->typeInfo->members)
        {
            if((member.elementType != ShaderStruct::Member::ElementType::Int &&
                member.elementType != ShaderStruct::Member::ElementType::Float) ||
               (member.matrixType != ShaderStruct::Member::MatrixType::Scalar &&
                member.matrixType != ShaderStruct::Member::MatrixType::Vector))
            {
                throw Exception("Only int/uint/float0123 properties are supported in 'Material' cbuffer");
            }

            const int memberDWordCount =
                member.matrixType == ShaderStruct::Member::MatrixType::Scalar ? 1 : member.rowSize;
            const bool needNewLine = memberDWordCount >= 4 || (dwordOffset % 4) + memberDWordCount > 4;
            if(needNewLine)
            {
                dwordOffset = (dwordOffset + 3) / 4 * 4;
            }

            size_t offsetInValueBuffer;
            {
                using enum ShaderStruct::Member::ElementType;
                using enum ShaderStruct::Member::MatrixType;

                using Tuple = std::tuple<ShaderStruct::Member::ElementType, ShaderStruct::Member::MatrixType, int>;
                static const Tuple typeToMemberInfo[] =
                {
                    { Float, Scalar, 0 }, { Float, Vector, 2 }, { Float, Vector, 3 }, { Float, Vector, 4 },
                    { Int,   Scalar, 0 }, { Int,   Vector, 2 }, { Int,   Vector, 3 }, { Int,   Vector, 4 },
                    { Int,   Scalar, 0 }, { Int,   Vector, 2 }, { Int,   Vector, 3 }, { Int,   Vector, 4 },
                };

                const int propIndex = materialPropertyLayout.GetPropertyIndexByName(member.name);
                if(propIndex == -1)
                {
                    throw Exception(fmt::format("Value property {} is not found in 'Material' cbuffer", member.name));
                }
                const MaterialProperty &prop = materialPropertyLayout.GetProperties()[propIndex];
                if(!prop.IsValue())
                {
                    throw Exception(fmt::format("Property {} is not declared as value type in material", member.name));
                }

                const auto requiredMemberInfo = typeToMemberInfo[static_cast<int>(prop.type)];
                if(member.elementType != std::get<0>(requiredMemberInfo) ||
                   member.matrixType  != std::get<1>(requiredMemberInfo) ||
                   member.rowSize     != std::get<2>(requiredMemberInfo))
                {
                    throw Exception(fmt::format(
                        "Type of value property {} doesn't match its declaration in material", member.name));
                }

                offsetInValueBuffer = materialPropertyLayout.GetValueOffset(propIndex);
            }

            ValueReference ref;
            ref.offsetInValueBuffer = offsetInValueBuffer;
            ref.offsetInConstantBuffer = dwordOffset * 4;
            ref.sizeInConstantBuffer = memberDWordCount * 4;
            valueReferences_.push_back(ref);

            dwordOffset += memberDWordCount;
        }

        constantBufferSize_ = dwordOffset * 4;
        constantBufferIndexInBindingGroup_ = cbuffer->indexInGroup;
    }

    // TODO optimize: merge neighboring values

    // Resources

    if(bindingGroupLayout_)
    {
        auto &desc = bindingGroupLayout_->GetRHIBindingGroupLayout()->GetDesc();
        for(auto &&[bindingIndex, aliased] : Enumerate(desc.bindings))
        {
            if(aliased.size() > 1)
            {
                throw Exception("Binding aliasing are not supported in 'Material' binding group");
            }
            const RHI::BindingDesc &binding = aliased[0];

            if(binding.type == RHI::BindingType::ConstantBuffer)
            {
                if(binding.name != "Material")
                {
                    throw Exception("Constant buffer in 'Material' binding group must have name 'Material'");
                }
                continue;
            }

            MaterialProperty::Type propType;
            switch(binding.type)
            {
            case RHI::BindingType::Texture2D:
            case RHI::BindingType::Texture3D:
            case RHI::BindingType::Texture2DArray:
            case RHI::BindingType::RWTexture3DArray:
                propType = MaterialProperty::Type::Texture2D;
                break;
            case RHI::BindingType::Buffer:
            case RHI::BindingType::StructuredBuffer:
                propType = MaterialProperty::Type::Buffer;
                break;
            case RHI::BindingType::Sampler:
                propType = MaterialProperty::Type::Sampler;
                break;
            default:
                throw Exception(fmt::format(
                    "Unsupported binding type in 'Material' group: {} {}",
                    GetBindingTypeName(binding.type), binding.name));
            }

            const int indexInProperties = materialPropertyLayout.GetPropertyIndexByName(binding.name)
                                        - materialPropertyLayout.GetValuePropertyCount();
            if(indexInProperties < 0)
            {
                throw Exception(fmt::format(
                    "Binding '{} {}' in 'Material' binding group is not declared in material",
                    GetBindingTypeName(binding.type), binding.name));
            }

            ResourceReference ref;
            ref.type = propType;
            ref.indexInProperties = indexInProperties;
            ref.indexInBindingGroup = bindingIndex;
            resourceReferences_.push_back(ref);
        }
    }
}

void SubMaterialPropertyLayout::FillConstantBufferContent(const void *valueBuffer, void *outputData) const
{
    auto input = static_cast<const unsigned char *>(valueBuffer);
    auto output = static_cast<unsigned char *>(outputData);
    for(auto &ref : valueReferences_)
    {
        std::memcpy(output + ref.offsetInConstantBuffer, input + ref.offsetInValueBuffer, ref.sizeInConstantBuffer);
    }
}

void SubMaterialPropertyLayout::FillBindingGroup(
    BindingGroup                   &bindingGroup,
    Span<RHI::RHIObjectPtr>         materialResources,
    const PersistentConstantBuffer &cbuffer) const
{
    // Constant buffer

    if(constantBufferIndexInBindingGroup_ >= 0)
    {
        bindingGroup.Set(
            constantBufferIndexInBindingGroup_, cbuffer.GetBuffer(), cbuffer.GetOffset(), cbuffer.GetSize());
    }

    // Other resources

    for(auto &ref : resourceReferences_)
    {
        auto &resource = materialResources[ref.indexInProperties];
        switch(ref.type)
        {
        case MaterialProperty::Type::Buffer:
            bindingGroup.Set(ref.indexInBindingGroup, DynamicCast<RHI::BufferSRV>(resource));
            break;
        case MaterialProperty::Type::Texture2D:
            bindingGroup.Set(ref.indexInBindingGroup, DynamicCast<RHI::TextureSRV>(resource));
            break;
        case MaterialProperty::Type::Sampler:
            bindingGroup.Set(ref.indexInBindingGroup, DynamicCast<RHI::Sampler>(resource));
            break;
        default:
            Unreachable();
        }
    }
}

RC<MaterialInstance> Material::CreateInstance() const
{
    return MakeRC<MaterialInstance>(shared_from_this(), resourceManager_);
}

MaterialManager::MaterialManager()
{
    resourceManager_ = nullptr;
    debug_ = RTRC_DEBUG;
}

void MaterialManager::SetResourceManager(ResourceManager *resourceManager)
{
    assert(resourceManager);
    resourceManager_ = resourceManager;
    shaderCompiler_.SetDevice(resourceManager->GetDeviceWithFrameResourceProtection());
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

    std::vector<MaterialProperty> properties;
    std::vector<RC<SubMaterial>> subMaterials;

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
        if(tokens.GetCurrentToken() == "Pass" || tokens.GetCurrentToken() == "SubMaterial")
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
    for(auto &subMat : subMaterials)
    {
        subMat->parentPropertyLayout_ = propertyLayout.get();
        subMat->shaderTemplate_->sharedEnvir_ = sharedCompileEnvir;
    }

    // Material tag

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
    material->resourceManager_ = resourceManager_;
    material->name_ = name;
    material->subMaterials_ = std::move(subMaterials);
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
    shaderTemplate->source_.VSEntry  = std::move(VSEntry);
    shaderTemplate->source_.FSEntry  = std::move(FSEntry);
    shaderTemplate->source_.CSEntry  = std::move(CSEntry);
    shaderTemplate->shaderCompiler_  = &shaderCompiler_;
    shaderTemplate->compiledShaders_.resize(1 << totalKeywordBitCount);

    return shaderTemplate;
}

RC<SubMaterial> MaterialManager::ParsePass(ShaderTokenStream &tokens)
{
    // Parse submaterial source

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
    subMaterial->subMaterialPropertyLayouts_.resize(
        1 << subMaterial->shaderTemplate_->GetKeywordSet().GetTotalBitCount());

    return subMaterial;
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
