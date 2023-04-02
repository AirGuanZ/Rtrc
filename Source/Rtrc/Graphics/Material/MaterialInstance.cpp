#include <Rtrc/Graphics/Material/MaterialInstance.h>

RTRC_BEGIN

template <MaterialProperty::Type Type, typename T>
void MaterialPropertySheet::SetImpl(MaterialPropertyName name, const T &value)
{
    AssertIsMainThread();
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    MaterialPropertySheet::SetImpl<Type>(index, value);
}

template<MaterialProperty::Type Type, typename T>
void MaterialPropertySheet::SetImpl(int index, const T &value)
{
    AssertIsMainThread();
    auto &prop = layout_->GetProperties()[index];
    if(prop.type != Type)
    {
        throw Exception(fmt::format(
            "MaterialPropertySheet::Set: type of property {} (index {}) is unmatched. Given: {}, actual: {}",
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

MaterialPropertySheet::MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout)
    : layout_(std::move(layout))
{
    valueBuffer_.resize(layout_->GetValueBufferSize());
    resources_.resize(layout_->GetResourcePropertyCount());
    bindlessEntries_.resize(layout_->GetValuePropertyCount());
}

void MaterialPropertySheet::CopyFrom(const MaterialPropertySheet &other)
{
    AssertIsMainThread();
    assert(layout_ == other.layout_);
    valueBuffer_ = other.valueBuffer_;
    resources_ = other.resources_;
}

void MaterialPropertySheet::Set(MaterialPropertyName name, const BindlessTextureEntry &entry)
{
    AssertIsMainThread();
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    Set(index, entry);
}

void MaterialPropertySheet::Set(int index, const BindlessTextureEntry &entry)
{
    AssertIsMainThread();
    auto &prop = layout_->GetProperties()[index];
    if(prop.type == MaterialProperty::Type::UInt)
    {
        if(entry.GetCount() > 1)
        {
            throw Exception(fmt::format(
                "Material property {} (index {}) can bind only one bindless texture index, "
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
            "Material property {} (index {}) cannot be bound with bindless texture entry. Type must be "
            "UInt (offset) or UInt2 (offset & count).", prop.name, index));
    }

    bindlessEntries_[index] = entry;
}

#define RTRC_IMPL_SET(VALUE_TYPE, TYPE)                                          \
    void MaterialPropertySheet::Set(MaterialPropertyName name, VALUE_TYPE value) \
    {                                                                            \
        this->SetImpl<MaterialProperty::Type::TYPE>(name, value);                \
    }                                                                            \
    void MaterialPropertySheet::Set(int index, VALUE_TYPE value)                 \
    {                                                                            \
        this->SetImpl<MaterialProperty::Type::TYPE>(index, value);               \
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

const unsigned char *MaterialPropertySheet::GetValue(MaterialPropertyName name) const
{
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    return GetValue(index);
}

const unsigned char *MaterialPropertySheet::GetValue(int index) const
{
    return GetValueBuffer() + layout_->GetValueOffset(index);
}

const BindlessTextureEntry *MaterialPropertySheet::GetBindlessTextureEntry(MaterialPropertyName name) const
{
    const int index = layout_->GetPropertyIndexByName(name);
    if(index < 0)
    {
        throw Exception(fmt::format("Unknown material property: {}", name));
    }
    return GetBindlessTextureEntry(index);
}

const BindlessTextureEntry *MaterialPropertySheet::GetBindlessTextureEntry(int index) const
{
    return bindlessEntries_[index] ? &bindlessEntries_[index] : nullptr;
}

RC<Shader> MaterialPassInstance::GetShader(const KeywordContext &keywordValues)
{
    return pass_->GetShader(keywordValues);
}

RC<Shader> MaterialPassInstance::GetShader(KeywordSet::ValueMask keywordMask)
{
    return pass_->GetShader(keywordMask);
}

const MaterialPass *MaterialPassInstance::GetPass() const
{
    return pass_;
}

void MaterialPassInstance::BindGraphicsProperties(
    const KeywordContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    KeywordSet::ValueMask mask = pass_->ExtractKeywordValueMask(keywordValues);
    BindGraphicsProperties(mask, commandBuffer);
}

void MaterialPassInstance::BindGraphicsProperties(
    KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<true>(mask, commandBuffer.GetRHIObject());
}

void MaterialPassInstance::BindComputeProperties(
    const KeywordContext &keywordValues, const CommandBuffer &commandBuffer) const
{
    KeywordSet::ValueMask mask = pass_->ExtractKeywordValueMask(keywordValues);
    BindComputeProperties(mask, commandBuffer);
}

void MaterialPassInstance::BindComputeProperties(
    KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const
{
    BindPropertiesImpl<false>(mask, commandBuffer.GetRHIObject());
}

template <bool Graphics>
void MaterialPassInstance::BindPropertiesImpl(
    KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer) const
{
    auto subLayout = pass_->GetPropertyLayout(mask);
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
    const MaterialPassInstance &instance,
    const KeywordContext  &keywords,
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

MaterialInstance::SharedRenderingData::SharedRenderingData(
    RC<const Material>      material,
    const MaterialInstance *materialInstance,
    Device                 *device)
    : device_(device)
    , material_(std::move(material))
    , propertySheet_(material_->GetPropertyLayout())
{
    auto passes = material_->GetPasses();
    passInstances_.resize(passes.size());
    for(size_t i = 0; i < passes.size(); ++i)
    {
        auto &pass = passes[i];
        auto matPassInst = MakeBox<MaterialPassInstance>();
        matPassInst->device_              = device;
        matPassInst->materialInstance_    = materialInstance;
        matPassInst->pass_                = pass.get();
        matPassInst->recordCount_         = 1 << pass->GetShaderTemplate()->GetKeywordSet().GetTotalBitCount();
        matPassInst->keywordMaskToRecord_ = std::make_unique<MaterialPassInstance::Record[]>(matPassInst->recordCount_);
        passInstances_[i] = std::move(matPassInst);
    }
}

MaterialInstance::SharedRenderingData *MaterialInstance::SharedRenderingData::Clone() const
{
    assert(material_ && device_);
    auto ret = new SharedRenderingData(material_, passInstances_[0]->materialInstance_, device_);
    ret->propertySheet_.CopyFrom(propertySheet_);
    return ret;
}

MaterialPassInstance *MaterialInstance::SharedRenderingData::GetPassInstance(std::string_view tag) const
{
    const int index = material_->GetPassIndexByTag(tag);
    return index >= 0 ? passInstances_[index].get() : nullptr;
}

MaterialPassInstance *MaterialInstance::SharedRenderingData::GetPassInstance(size_t index) const
{
    return passInstances_[index].get();
}

MaterialInstance::MaterialInstance(RC<const Material> material, Device *device)
{
    data_.Reset(new SharedRenderingData(std::move(material), this, device));
}

void MaterialInstance::InvalidateBindingGroups()
{
    assert(data_.IsUnique());
    for(auto &inst : data_->passInstances_)
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
    assert(data_.IsUnique());
    for(auto &inst : data_->passInstances_)
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
