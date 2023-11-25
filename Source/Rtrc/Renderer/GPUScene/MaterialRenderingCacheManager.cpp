#include <Rtrc/Renderer/GPUScene/MaterialRenderingCacheManager.h>

RTRC_RENDERER_BEGIN

void MaterialRenderingCacheManager::Update(const Scene &scene)
{
    struct MaterialRecord
    {
        const LegacyMaterialInstance *materialInstance = nullptr;
        const BindlessTextureEntry *albedoTextureIndex = nullptr;
        auto operator<=>(const MaterialRecord &rhs) const = default;
    };
    std::vector<MaterialRecord> materialRecords;

    {
        scene.ForEachMeshRenderer([&](const MeshRenderer* renderer)
        {
            auto material = renderer->GetMaterial().get();
            materialRecords.push_back({ material, {} });
        });
        std::ranges::sort(materialRecords);

        auto removeRange = std::ranges::unique(materialRecords);
        materialRecords.erase(std::unique(materialRecords.begin(), materialRecords.end()), materialRecords.end());
    }

    std::vector<Box<MaterialRenderingCache>> newCachedMaterials;
    auto it = cachedMaterials_.begin();
    for(const MaterialRecord &record : materialRecords)
    {
        if(it == cachedMaterials_.end() || it->get()->materialInstance->GetUniqueID() > record.materialInstance->GetUniqueID())
        {
            auto &propertySheet = record.materialInstance->GetPropertySheet();
            auto &cache = *newCachedMaterials.emplace_back(MakeBox<MaterialRenderingCache>());
            cache.materialInstance = record.materialInstance;
            cache.material = record.materialInstance->GetMaterial().get();
            
            auto albedoEntry = propertySheet.GetBindlessTextureEntry(RTRC_SHADER_PROPERTY_NAME(albedoTextureIndex));
            if(albedoEntry)
            {
                cache.albedoTextureEntry = *albedoEntry;
            }

            cache.albedoScale = 1.0f;
            if(auto scaleProp = propertySheet.GetValue(RTRC_SHADER_PROPERTY_NAME(albedoScale)))
            {
                cache.albedoScale = *reinterpret_cast<const float *>(scaleProp);
            }
            
            cache.gbufferPassIndex = cache.materialInstance->GetMaterial()->GetPassIndexByTag(RTRC_MATERIAL_PASS_TAG(GBuffer));
            if(cache.gbufferPassIndex >= 0)
            {
                auto passes = cache.materialInstance->GetMaterial()->GetPasses();
                auto shaderTemplate = passes[cache.gbufferPassIndex]->GetShaderTemplate();
                cache.supportGBufferInstancing = shaderTemplate->HasBuiltinKeyword(BuiltinKeyword::EnableInstance);
            }

            continue;
        }

        if(it->get()->materialInstance->GetUniqueID() == record.materialInstance->GetUniqueID())
        {
            newCachedMaterials.emplace_back(std::move(*it));
            ++it;
            continue;
        }

        assert(it->get()->materialInstance->GetUniqueID() < record.materialInstance->GetUniqueID());
        ++it;
    }

    cachedMaterials_ = std::move(newCachedMaterials);
}

const MaterialRenderingCache *MaterialRenderingCacheManager::FindMaterialRenderingCache(const LegacyMaterialInstance *material) const
{
    size_t beg = 0, end = cachedMaterials_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        MaterialRenderingCache *data = cachedMaterials_[mid].get();
        if(data->materialInstance->GetUniqueID() == material->GetUniqueID())
        {
            return cachedMaterials_[mid].get();
        }
        if(data->materialInstance->GetUniqueID() < material->GetUniqueID())
        {
            beg = mid;
        }
        else
        {
            end = mid;
        }
    }
    return nullptr;
}

RTRC_RENDERER_END
