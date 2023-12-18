#include <Core/Serialization/TextSerializer.h>
#include <Rtrc/Resource/LegacyMaterialManager.h>
#include <Core/ReflectedStruct.h>

RTRC_BEGIN

LegacyMaterialManager::LegacyMaterialManager()
{
    RegisterAllPreprocessedMaterialsInMaterialManager(*this);
    localMaterialCache_ = MakeBox<LocalMaterialCache>(this);
}

void LegacyMaterialManager::SetDevice(Ref<Device> device)
{
    device_ = device;
}

void LegacyMaterialManager::SetShaderManager(Ref<ShaderManager> shaderManager)
{
    shaderManager_ = shaderManager;
}

void LegacyMaterialManager::AddMaterial(RawMaterialRecord rawMaterial)
{
    const GeneralPooledString pooledName(rawMaterial.name);
    materialRecords_.insert({ pooledName, std::move(rawMaterial) });
}

RC<LegacyMaterial> LegacyMaterialManager::GetMaterial(std::string_view name)
{
    return GetMaterial(GeneralPooledString(name));
}

RC<LegacyMaterial> LegacyMaterialManager::GetMaterial(GeneralPooledString name)
{
    return materialPool_.GetOrCreate(name, [&]
    {
        auto it = materialRecords_.find(name);
        if(it == materialRecords_.end())
        {
            throw Exception(fmt::format("LegacyMaterialManager: unknown material {}", name));
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

        std::vector<RC<LegacyMaterialPass>> passes(rawMaterial.passes.size());
        for(size_t i = 0; i < passes.size(); ++i)
        {
            auto &in = rawMaterial.passes[i];
            auto &out = passes[i];

            out = MakeRC<LegacyMaterialPass>();
            for(auto &tag : in.tags)
            {
                out->tags_.push_back(tag);
                out->pooledTags_.push_back(MaterialPassTag(tag));
            }
            out->shaderTemplate_ = shaderManager_->GetShaderTemplate(in.shaderName, false);
            if(!out->shaderTemplate_)
            {
                throw Exception("Unknown shader: " + in.shaderName);
            }
            out->hostMaterialPropertyLayout_ = propertyLayout.get();
            out->materialPassPropertyLayouts_.resize(1 << out->shaderTemplate_->GetKeywordSet().GetTotalBitCount());
        }

        auto ret = MakeRC<LegacyMaterial>();
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

RC<LegacyMaterialInstance> LegacyMaterialManager::CreateMaterialInstance(std::string_view name)
{
    return GetMaterial(name)->CreateInstance();
}

RC<LegacyMaterialInstance> LegacyMaterialManager::CreateMaterialInstance(GeneralPooledString name)
{
    return GetMaterial(name)->CreateInstance();
}

LocalMaterialCache &LegacyMaterialManager::GetLocalMaterialCache()
{
    return *localMaterialCache_;
}

RTRC_END
