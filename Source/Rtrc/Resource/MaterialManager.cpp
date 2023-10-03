#include <Core/Serialization/TextSerializer.h>
#include <Rtrc/Resource/MaterialManager.h>
#include <Core/ReflectedStruct.h>

RTRC_BEGIN

MaterialManager::MaterialManager()
{
    const auto workDir = absolute(std::filesystem::current_path()).lexically_normal();
    shaderDatabase_.AddIncludeDirectory(ReflectedStruct::GetGeneratedFilePath());
    shaderDatabase_.AddIncludeDirectory((workDir / "Source").string());

    RegisterAllPreprocessedMaterialsInMaterialManager(*this);

    localShaderCache_ = MakeBox<LocalShaderCache>(this);
    localMaterialCache_ = MakeBox<LocalMaterialCache>(this);
}

void MaterialManager::SetDevice(ObserverPtr<Device> device)
{
    device_ = device;
    shaderDatabase_.SetDevice(device);
}

void MaterialManager::SetDebug(bool debug)
{
    shaderDatabase_.SetDebug(debug);
}

void MaterialManager::AddMaterial(RawMaterialRecord rawMaterial)
{
    const GeneralPooledString pooledName(rawMaterial.name);
    materialRecords_.insert({ pooledName, std::move(rawMaterial) });
}

RC<Material> MaterialManager::GetMaterial(std::string_view name)
{
    return GetMaterial(GeneralPooledString(name));
}

RC<Material> MaterialManager::GetMaterial(GeneralPooledString name)
{
    return materialPool_.GetOrCreate(name, [&]
    {
        auto it = materialRecords_.find(name);
        if(it == materialRecords_.end())
        {
            throw Exception(fmt::format("MaterialManager: unknown material {}", name));
        }
        auto &rawMaterial = it->second;

        // Properties

        std::vector<MaterialProperty> properties(rawMaterial.properties.size());
        for(size_t i = 0; i < properties.size(); ++i)
        {
            auto &in = rawMaterial.properties[i];
            auto &out = properties[i];
            out.name = ShaderPropertyName(in.name);

            using enum MaterialProperty::Type;
            static const std::map<std::string, MaterialProperty::Type, std::less<>> NAME_TO_PROPERTY_TYPE =
            {
                { "float",                 Float                 },
                { "float2",                Float2                },
                { "float3",                Float3                },
                { "float4",                Float4                },
                { "uint",                  UInt                  },
                { "uint2",                 UInt2                 },
                { "uint3",                 UInt3                 },
                { "uint4",                 UInt4                 },
                { "int",                   Int                   },
                { "int2",                  Int2                  },
                { "int3",                  Int3                  },
                { "int4",                  Int4                  },
                { "Buffer",                Buffer                },
                { "Texture",               Texture               },
                { "SamplerState",          Sampler               },
                { "AccelerationStructure", AccelerationStructure }
            };
            if(auto jt = NAME_TO_PROPERTY_TYPE.find(in.type); jt != NAME_TO_PROPERTY_TYPE.end())
            {
                out.type = jt->second;
            }
            else
            {
                throw Exception(fmt::format("Unknown material property type: {}", in.type));
            }
        }
        auto propertyLayout = MakeRC<MaterialPropertyHostLayout>(std::move(properties));

        // Pass

        std::vector<RC<MaterialPass>> passes(rawMaterial.passes.size());
        for(size_t i = 0; i < passes.size(); ++i)
        {
            auto &in = rawMaterial.passes[i];
            auto &out = passes[i];

            out = MakeRC<MaterialPass>();
            for(auto &tag : in.tags)
            {
                out->tags_.push_back(tag);
                out->pooledTags_.push_back(MaterialPassTag(tag));
            }
            out->shaderTemplate_ = GetShaderTemplate(in.shaderName, false);
            if(!out->shaderTemplate_)
            {
                throw Exception("Unknown shader: " + in.shaderName);
            }
            out->hostMaterialPropertyLayout_ = propertyLayout.get();
            out->materialPassPropertyLayouts_.resize(1 << out->shaderTemplate_->GetKeywordSet().GetTotalBitCount());
        }

        auto ret = MakeRC<Material>();
        ret->device_         = device_;
        ret->name_           = rawMaterial.name;
        ret->passes_         = std::move(passes);
        ret->propertyLayout_ = std::move(propertyLayout);
        for(size_t i = 0; i < ret->passes_.size(); ++i)
        {
            for(auto &tag : ret->passes_[i]->pooledTags_)
            {
                ret->tagToIndex_[tag] = i;
            }
        }
        return ret;
    });
}

RC<ShaderTemplate> MaterialManager::GetShaderTemplate(std::string_view name, bool persistent)
{
    return shaderDatabase_.GetShaderTemplate(name, persistent);
}

RC<Shader> MaterialManager::GetShader(std::string_view name, bool persistent)
{
    return shaderDatabase_.GetShaderTemplate(name, persistent)->GetVariant(FastKeywordSetValue{}, persistent);
}

RC<MaterialInstance> MaterialManager::CreateMaterialInstance(std::string_view name)
{
    return GetMaterial(name)->CreateInstance();
}

RC<MaterialInstance> MaterialManager::CreateMaterialInstance(GeneralPooledString name)
{
    return GetMaterial(name)->CreateInstance();
}

LocalMaterialCache &MaterialManager::GetLocalMaterialCache()
{
    return *localMaterialCache_;
}

LocalShaderCache &MaterialManager::GetLocalShaderCache()
{
    return *localShaderCache_;
}

RTRC_END
