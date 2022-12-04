#include <Rtrc/Graphics/Material/MaterialInstance.h>

RTRC_BEGIN

template <MaterialProperty::Type Type, typename T>
void MaterialPropertySheet::SetImpl(std::string_view name, const T &value)
{
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }

    auto &prop = layout_->GetProperties()[index];
    if(prop.type != Type)
    {
        throw Exception(fmt::format(
            "MaterialPropertySheet::Set: type of property {} is unmatched. Given: {}, actual: {}",
            name, MaterialProperty::GetTypeName(Type), MaterialProperty::GetTypeName(prop.type)));
    }              

    if constexpr(MaterialProperty::IsValue(Type))
    {
        const size_t offset = layout_->GetValueOffset(index);
        std::memcpy(valueBuffer_.data() + offset, &value, sizeof(value));
    }
    else
    {
        resources_[index - layout_->GetValuePropertyCount()] = value;
    }
}

MaterialPropertySheet::MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout)
    : layout_(std::move(layout))
{
    valueBuffer_.resize(layout_->GetValueBufferSize());
    resources_.resize(layout_->GetResourcePropertyCount());
}

#define RTRC_IMPL_SET(VALUE_TYPE, TYPE)                                      \
    void MaterialPropertySheet::Set(std::string_view name, VALUE_TYPE value) \
    {                                                                        \
        this->SetImpl<MaterialProperty::Type::TYPE>(name, value);            \
    }

RTRC_IMPL_SET(float,            Float)
RTRC_IMPL_SET(const Vector2f &, Float2)
RTRC_IMPL_SET(const Vector3f &, Float3)
RTRC_IMPL_SET(const Vector4f &, Float4)

RTRC_IMPL_SET(uint32_t,         UInt)
RTRC_IMPL_SET(const Vector2u &, UInt2)
RTRC_IMPL_SET(const Vector3u &, UInt3)
RTRC_IMPL_SET(const Vector4u &, UInt4)

RTRC_IMPL_SET(int32_t,          Int)
RTRC_IMPL_SET(const Vector2i &, Int2)
RTRC_IMPL_SET(const Vector3i &, Int3)
RTRC_IMPL_SET(const Vector4i &, Int4)

RTRC_IMPL_SET(const RC<Texture> &, Texture2D)
RTRC_IMPL_SET(const TextureSRV &,  Texture2D)
RTRC_IMPL_SET(const BufferSRV  &,  Buffer)
RTRC_IMPL_SET(RC<Sampler>,         Sampler)

#undef RTRC_IMPL_SET

RC<Shader> SubMaterialInstance::GetShader(const KeywordValueContext &keywordValues)
{
    return subMaterial_->GetShader(keywordValues);
}

RC<Shader> SubMaterialInstance::GetShader(KeywordSet::ValueMask keywordMask)
{
    return subMaterial_->GetShader(keywordMask);
}

const RC<SubMaterial> &SubMaterialInstance::GetSubMaterial() const
{
    return subMaterial_;
}

void SubMaterialInstance::BindGraphicsProperties(
    const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    KeywordSet::ValueMask mask = subMaterial_->ExtractKeywordValueMask(keywordValues);
    BindGraphicsProperties(mask, commandBuffer);
}

void SubMaterialInstance::BindGraphicsProperties(
    KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<true>(mask, commandBuffer.GetRHIObject());
}

void SubMaterialInstance::BindComputeProperties(
    const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    KeywordSet::ValueMask mask = subMaterial_->ExtractKeywordValueMask(keywordValues);
    BindComputeProperties(mask, commandBuffer);
}

void SubMaterialInstance::BindComputeProperties(
    KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<false>(mask, commandBuffer.GetRHIObject());
}

template <bool Graphics>
void SubMaterialInstance::BindPropertiesImpl(
    KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer) const
{
    auto shader = subMaterial_->GetShader(mask);
    if(auto index = shader->GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        if constexpr(Graphics)
        {
            commandBuffer->BindGroupToGraphicsPipeline(
                index, shader->GetBindingGroupForInlineSamplers()->GetRHIObject());
        }
        else
        {
            commandBuffer->BindGroupToComputePipeline(
                index, shader->GetBindingGroupForInlineSamplers()->GetRHIObject());
        }
    }

    auto subLayout = subMaterial_->GetPropertyLayout(mask);
    const int bindingGroupIndex = subLayout->GetBindingGroupIndex();
    if(bindingGroupIndex < 0)
    {
        return;
    }

    const Record &record = keywordMaskToRecord_[mask];
    auto bind = [&]
    {
        if constexpr(Graphics)
        {
            commandBuffer->BindGroupToGraphicsPipeline(bindingGroupIndex, record.bindingGroup->GetRHIObject());
        }
        else
        {
            commandBuffer->BindGroupToComputePipeline(bindingGroupIndex, record.bindingGroup->GetRHIObject());
        }
    };

    {
        std::shared_lock readLock(record.mutex);
        if(record.bindingGroup)
        {
            bind();
            return;
        }
    }

    std::unique_lock writeLock(record.mutex);
    if(!record.bindingGroup)
    {
        const size_t cbSize = subLayout->GetConstantBufferSize();
        if(cbSize && !record.constantBuffer)
        {
            record.constantBuffer = device_->CreateDynamicBuffer();
        }

        auto &properties = parentMaterialInstance_->GetPropertySheet();
        if(cbSize && record.isConstantBufferDirty)
        {
            std::vector<unsigned char> data(cbSize);
            subLayout->FillConstantBufferContent(properties.GetValueBuffer(), data.data());
            record.constantBuffer->SetData(data.data(), cbSize, true);
            record.isConstantBufferDirty = false;
        }

        auto &bindingGroupLayout = subLayout->GetBindingGroupLayout();
        record.bindingGroup = bindingGroupLayout->CreateBindingGroup();
        subLayout->FillBindingGroup(*record.bindingGroup, properties.GetResources(), record.constantBuffer);
    }
    bind();
}

MaterialInstance::MaterialInstance(RC<const Material> material, Device *device)
    : parentMaterial_(std::move(material)), properties_(parentMaterial_->GetPropertyLayout())
{
    auto subMaterials = parentMaterial_->GetSubMaterials();
    subMaterialInstances_.resize(subMaterials.size());
    for(size_t i = 0; i < subMaterials.size(); ++i)
    {
        auto &subMat = subMaterials[i];
        auto subMatInst = MakeBox<SubMaterialInstance>();
        subMatInst->device_ = device;
        subMatInst->parentMaterialInstance_ = this;
        subMatInst->subMaterial_ = subMat;
        subMatInst->recordCount_ = 1 << subMat->GetShaderTemplate()->GetKeywordSet().GetTotalBitCount();
        subMatInst->keywordMaskToRecord_ = std::make_unique<SubMaterialInstance::Record[]>(subMatInst->recordCount_);
        subMaterialInstances_[i] = std::move(subMatInst);
    }
}

SubMaterialInstance *MaterialInstance::GetSubMaterialInstance(std::string_view tag)
{
    const int index = parentMaterial_->GetSubMaterialIndexByTag(tag);
    return index >= 0 ? subMaterialInstances_[index].get() : nullptr;
}

SubMaterialInstance *MaterialInstance::GetSubMaterialInstance(size_t index)
{
    return subMaterialInstances_[index].get();
}

void MaterialInstance::InvalidateBindingGroups()
{
    for(auto &inst : subMaterialInstances_)
    {
        for(int i = 0; i < inst->recordCount_; ++i)
        {
            auto &record = inst->keywordMaskToRecord_[i];
            record.bindingGroup = nullptr;
        }
    }
}

void MaterialInstance::InvalidateConstantBuffers()
{
    for(auto &inst : subMaterialInstances_)
    {
        for(int i = 0; i < inst->recordCount_; ++i)
        {
            auto &record = inst->keywordMaskToRecord_[i];
            record.bindingGroup = nullptr;
            record.isConstantBufferDirty = true;
        }
    }
}

RTRC_END
