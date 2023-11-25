#include <Rtrc/Resource/Material/LegacyMaterialInstance.h>

RTRC_BEGIN

template <MaterialProperty::Type Type, typename T>
void LegacyMaterialPropertySheet::SetImpl(ShaderPropertyName name, const T &value)
{
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    LegacyMaterialPropertySheet::SetImpl<Type>(index, value);
}

template<MaterialProperty::Type Type, typename T>
void LegacyMaterialPropertySheet::SetImpl(int index, const T &value)
{
    auto &prop = layout_->GetProperties()[index];
    if(prop.type != Type)
    {
        throw Exception(fmt::format(
            "LegacyMaterialPropertySheet::Set: type of property {} (index {}) is unmatched. Given: {}, actual: {}",
            prop.name, index, MaterialProperty::GetTypeName(Type), MaterialProperty::GetTypeName(prop.type)));
    }

    if constexpr(MaterialProperty::IsValue(Type))
    {
        const size_t offset = layout_->GetValueOffset(index);
        std::memcpy(valueBuffer_.data() + offset, &value, sizeof(value));
        bindlessEntries_[index] = {};
    }
    else
    {
        resources_[index - layout_->GetValuePropertyCount()] = value;
    }
}

LegacyMaterialPropertySheet::LegacyMaterialPropertySheet(RC<MaterialPropertyHostLayout> layout)
    : layout_(std::move(layout))
{
    valueBuffer_.resize(layout_->GetValueBufferSize());
    resources_.resize(layout_->GetResourcePropertyCount());
    bindlessEntries_.resize(layout_->GetValuePropertyCount());
}

void LegacyMaterialPropertySheet::CopyFrom(const LegacyMaterialPropertySheet &other)
{
    assert(layout_ == other.layout_);
    valueBuffer_ = other.valueBuffer_;
    resources_ = other.resources_;
}

void LegacyMaterialPropertySheet::Set(ShaderPropertyName name, const BindlessTextureEntry &entry)
{
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    Set(index, entry);
}

void LegacyMaterialPropertySheet::Set(int index, const BindlessTextureEntry &entry)
{
    auto &prop = layout_->GetProperties()[index];
    if(prop.type == MaterialProperty::Type::UInt)
    {
        if(entry.GetCount() > 1)
        {
            throw Exception(fmt::format(
                "LegacyMaterial property {} (index {}) can bind only one bindless texture index, "
                "which multiple indices are provided", prop.name, index));
        }
        const size_t offset = layout_->GetValueOffset(index);
        const uint32_t value = entry.GetOffset();
        std::memcpy(valueBuffer_.data() + offset, &value, sizeof(value));
    }
    else if(prop.type == MaterialProperty::Type::UInt2)
    {
        const size_t offset = layout_->GetValueOffset(index);
        const Vector2u value = { entry.GetOffset(), entry.GetCount() };
        std::memcpy(valueBuffer_.data() + offset, &value, sizeof(value));
    }
    else
    {
        throw Exception(fmt::format(
            "LegacyMaterial property {} (index {}) cannot be bound with bindless texture entry. Type must be "
            "UInt (offset) or UInt2 (offset & count).", prop.name, index));
    }

    bindlessEntries_[index] = entry;
}

#define RTRC_IMPL_SET(VALUE_TYPE, TYPE)                                              \
    void LegacyMaterialPropertySheet::Set(ShaderPropertyName name, VALUE_TYPE value) \
    {                                                                                \
        this->SetImpl<MaterialProperty::Type::TYPE>(name, value);                    \
    }                                                                                \
    void LegacyMaterialPropertySheet::Set(int index, VALUE_TYPE value)               \
    {                                                                                \
        this->SetImpl<MaterialProperty::Type::TYPE>(index, value);                   \
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

RTRC_IMPL_SET(const RC<Texture> &, Texture)
RTRC_IMPL_SET(const TextureSrv  &, Texture)
RTRC_IMPL_SET(const BufferSrv   &, Buffer)
RTRC_IMPL_SET(const RC<Sampler> &, Sampler)
RTRC_IMPL_SET(const RC<Tlas>    &, AccelerationStructure)

#undef RTRC_IMPL_SET

const unsigned char *LegacyMaterialPropertySheet::GetValue(ShaderPropertyName name) const
{
    const int index = layout_->GetPropertyIndexByName(name);
    return index >= 0 ? GetValue(index) : nullptr;
}

const unsigned char *LegacyMaterialPropertySheet::GetValue(int index) const
{
    return GetValueBuffer() + layout_->GetValueOffset(index);
}

const BindlessTextureEntry *LegacyMaterialPropertySheet::GetBindlessTextureEntry(ShaderPropertyName name) const
{
    const int index = layout_->GetPropertyIndexByName(name);
    return index >= 0 ? GetBindlessTextureEntry(index) : nullptr;
}

const BindlessTextureEntry *LegacyMaterialPropertySheet::GetBindlessTextureEntry(int index) const
{
    return bindlessEntries_[index] ? &bindlessEntries_[index] : nullptr;
}

RC<Shader> LegacyMaterialPassInstance::GetShader(const FastKeywordContext &keywordValues)
{
    return pass_->GetShader(keywordValues);
}

RC<Shader> LegacyMaterialPassInstance::GetShader(FastKeywordSetValue keywordMask)
{
    return pass_->GetShader(keywordMask);
}

const LegacyMaterialPass *LegacyMaterialPassInstance::GetPass() const
{
    return pass_;
}

void LegacyMaterialPassInstance::BindGraphicsProperties(
    const FastKeywordContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    FastKeywordSetValue mask = pass_->ExtractKeywordValueMask(keywordValues);
    BindGraphicsProperties(mask, commandBuffer);
}

void LegacyMaterialPassInstance::BindGraphicsProperties(
    FastKeywordSetValue mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<true>(mask, commandBuffer.GetRHIObject());
}

void LegacyMaterialPassInstance::BindGraphicsProperties(const CommandBuffer &commandBuffer) const
{
    BindGraphicsProperties(FastKeywordSetValue{}, commandBuffer);
}

void LegacyMaterialPassInstance::BindComputeProperties(
    const FastKeywordContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    FastKeywordSetValue mask = pass_->ExtractKeywordValueMask(keywordValues);
    BindComputeProperties(mask, commandBuffer);
}

void LegacyMaterialPassInstance::BindComputeProperties(
    FastKeywordSetValue mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<false>(mask, commandBuffer.GetRHIObject());
}

void LegacyMaterialPassInstance::BindComputeProperties(const CommandBuffer &commandBuffer) const
{
    BindComputeProperties(FastKeywordSetValue{}, commandBuffer);
}

template <bool Graphics>
void LegacyMaterialPassInstance::BindPropertiesImpl(
    FastKeywordSetValue mask, const RHI::CommandBufferRPtr &commandBuffer) const
{
    auto subLayout = pass_->GetPropertyLayout(mask);
    const int bindingGroupIndex = subLayout->GetBindingGroupIndex();
    if(bindingGroupIndex < 0)
    {
        return;
    }

    const Record &record = keywordMaskToRecord_[mask.GetInternalValue()];
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

        auto &properties = materialInstance_->GetPropertySheet();
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

void BindMaterialProperties(
    const LegacyMaterialPassInstance &instance,
    const FastKeywordContext       &keywords,
    const CommandBuffer        &commandBuffer,
    bool                        graphics)
{
    if(graphics)
    {
        instance.BindGraphicsProperties(keywords, commandBuffer);
    }
    else
    {
        instance.BindComputeProperties(keywords, commandBuffer);
    }
}

LegacyMaterialPassInstance *LegacyMaterialInstance::GetPassInstance(MaterialPassTag tag) const
{
    const int index = material_->GetPassIndexByTag(tag);
    return index >= 0 ? passInstances_[index].get() : nullptr;
}

LegacyMaterialPassInstance *LegacyMaterialInstance::GetPassInstance(size_t index) const
{
    return passInstances_[index].get();
}

LegacyMaterialInstance::LegacyMaterialInstance(RC<const LegacyMaterial> material, Device *device)
    : material_(material), propertySheet_(material->GetPropertyLayout())
{
    auto passes = material_->GetPasses();
    passInstances_.resize(passes.size());
    for(size_t i = 0; i < passes.size(); ++i)
    {
        auto &pass = passes[i];
        auto matPassInst = MakeBox<LegacyMaterialPassInstance>();
        matPassInst->device_              = device;
        matPassInst->materialInstance_    = this;
        matPassInst->pass_                = pass.get();
        matPassInst->recordCount_         = 1 << pass->GetShaderTemplate()->GetKeywordSet().GetTotalBitCount();
        matPassInst->keywordMaskToRecord_ =
            std::make_unique<LegacyMaterialPassInstance::Record[]>(matPassInst->recordCount_);
        passInstances_[i] = std::move(matPassInst);
    }
}

void LegacyMaterialInstance::InvalidateBindingGroups()
{
    for(auto &inst : passInstances_)
    {
        for(int i = 0; i < inst->recordCount_; ++i)
        {
            auto &record = inst->keywordMaskToRecord_[i];
            record.bindingGroup = nullptr;
        }
    }
}

void LegacyMaterialInstance::InvalidateConstantBuffers()
{
    for(auto &inst : passInstances_)
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
