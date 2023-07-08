#include <Rtrc/Renderer/Scene/RenderMaterials.h>

RTRC_RENDERER_BEGIN

void RenderMaterials::UpdateCachedMaterialData(const RenderCommand_RenderStandaloneFrame &frame)
{
    struct MaterialRecord
    {
        const MaterialInstance::SharedRenderingData *materialRenderingData = nullptr;
        const BindlessTextureEntry *albedoTextureIndex = nullptr;

        auto operator<=>(const MaterialRecord &rhs) const = default;
    };

    std::vector<MaterialRecord> materialRecords;
    for(const StaticMeshRenderProxy *proxy : frame.scene->GetStaticMeshRenderObjects())
    {
        auto material = proxy->materialRenderingData.Get();
        materialRecords.push_back({ material, {} });
    }
    std::ranges::sort(materialRecords);

    const auto [removeBegin, removeEnd] = std::ranges::unique(materialRecords);
    materialRecords.erase(removeBegin, removeEnd);

    auto it = cachedMaterials_.begin();
    for(const MaterialRecord &materialRecord : materialRecords)
    {
        if(it == cachedMaterials_.end() ||
           it->get()->materialRenderingDataId > materialRecord.materialRenderingData->GetUniqueID())
        {
            auto &propertySheet = materialRecord.materialRenderingData->GetPropertySheet();

            auto albedoEntry = propertySheet.GetBindlessTextureEntry(RTRC_MATERIAL_PROPERTY_NAME(GIAlbedoTextureIndex));
            if(!albedoEntry)
            {
                albedoEntry = propertySheet.GetBindlessTextureEntry(RTRC_MATERIAL_PROPERTY_NAME(albedoTextureIndex));
            }

            float albedoScale = 1.0f;
            if(auto pScale = propertySheet.GetValue(RTRC_MATERIAL_PROPERTY_NAME(GIAlbedoScale)))
            {
                albedoScale = *reinterpret_cast<const float *>(pScale);
            }

            auto &data = *cachedMaterials_.emplace(it, MakeBox<RenderMaterial>());

            data->material                = materialRecord.materialRenderingData->GetMaterial().get();
            data->materialId              = data->material->GetUniqueID();
            data->materialRenderingDataId = materialRecord.materialRenderingData->GetUniqueID();
            data->materialRenderingData   = materialRecord.materialRenderingData;
            data->albedoTextureEntry      = albedoEntry;
            data->albedoScale             = albedoScale;

            data->gbufferPassIndex = data->material->GetBuiltinPassIndex(Material::BuiltinPass::GBuffer);
            if(data->gbufferPassIndex >= 0)
            {
                auto shaderT = data->material->GetPasses()[data->gbufferPassIndex]->GetShaderTemplate();
                data->gbufferPassSupportInstancing = shaderT->HasBuiltinKeyword(BuiltinKeyword::EnableInstance);
            }

            it = cachedMaterials_.end();
            continue;
        }

        if(it->get()->materialRenderingDataId == materialRecord.materialRenderingData->GetUniqueID())
        {
            ++it;
            continue;
        }

        assert(it->get()->materialRenderingDataId < materialRecord.materialRenderingData->GetUniqueID());
        it = cachedMaterials_.erase(it);
    }

    linearCachedMaterials_.clear();
    std::ranges::transform(
        cachedMaterials_, std::back_inserter(linearCachedMaterials_),
        [](const Box<RenderMaterial> &box) { return box.get(); });
}

const RenderMaterials::RenderMaterial *RenderMaterials::FindCachedMaterial(UniqueId materialId) const
{
    size_t beg = 0, end = linearCachedMaterials_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        RenderMaterial *data = linearCachedMaterials_[mid];
        if(data->materialRenderingDataId == materialId)
        {
            return linearCachedMaterials_[mid];
        }
        if(data->materialRenderingDataId < materialId)
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
