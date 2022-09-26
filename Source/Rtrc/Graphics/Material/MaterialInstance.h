#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Resource/ConstantBuffer.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>

RTRC_BEGIN

class MaterialInstance;

class MaterialPropertySheet
{
public:

    explicit MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout);

    void Set(std::string_view name, float value);
    void Set(std::string_view name, const Vector2f &value);
    void Set(std::string_view name, const Vector3f &value);
    void Set(std::string_view name, const Vector4f &value);

    void Set(std::string_view name, uint32_t value);
    void Set(std::string_view name, const Vector2u &value);
    void Set(std::string_view name, const Vector3u &value);
    void Set(std::string_view name, const Vector4u &value);

    void Set(std::string_view name, int32_t value);
    void Set(std::string_view name, const Vector2i &value);
    void Set(std::string_view name, const Vector3i &value);
    void Set(std::string_view name, const Vector4i &value);

    void Set(std::string_view name, const RHI::TextureSRVPtr &srv);
    void Set(std::string_view name, const RHI::BufferSRVPtr &srv);
    void Set(std::string_view name, const RHI::SamplerPtr &sampler);

    const unsigned char *GetValueBuffer() const { return valueBuffer_.data(); }
    Span<RHI::RHIObjectPtr> GetResources() const { return resources_; }

private:

    template<MaterialProperty::Type Type, typename T>
    void SetImpl(std::string_view name, const T &value);

    RC<MaterialPropertyHostLayout> layout_;
    std::vector<unsigned char> valueBuffer_;
    std::vector<RHI::RHIObjectPtr> resources_;
};

class SubMaterialInstance
{
public:

    RC<Shader> GetShader(const KeywordValueContext &keywordValues);
    RC<Shader> GetShader(KeywordSet::ValueMask keywordMask);

    void BindGraphicsProperties(const KeywordValueContext &keywordValues, const RHI::CommandBufferPtr &commandBuffer);
    void BindGraphicsProperties(KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer);

    void BindComputeProperties(const KeywordValueContext &keywordValues, const RHI::CommandBufferPtr &commandBuffer);
    void BindComputeProperties(KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer);

private:

    friend class MaterialInstance;

    struct Record
    {
        RC<BindingGroup> bindingGroup;
        Box<PersistentConstantBuffer> constantBuffer;
        bool isConstantBufferDirty = true;
    };

    // return binding group index
    template<bool Graphics>
    void BindPropertiesImpl(KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer);

    ResourceManager *resourceManager_ = nullptr;
    const MaterialInstance *parentMaterialInstance_ = nullptr;
    RC<SubMaterial> subMaterial_;
    std::vector<Record> keywordMaskToRecord_;
};

class MaterialInstance
{
public:

    MaterialInstance(RC<const Material> material, ResourceManager *resourceManager);

    template<typename T>
    void Set(std::string_view name, const T &value)
    {
        properties_.Set(name, value);
    }

    const RC<const Material> &GetMaterial() const { return parentMaterial_; }

    // return nullptr when not found
    SubMaterialInstance *GetSubMaterialInstance(std::string_view tag);
    SubMaterialInstance *GetSubMaterialInstance(size_t index);

    const MaterialPropertySheet &GetPropertySheet() const { return properties_; }

    void Set(std::string_view name, float value)           { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2f &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3f &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4f &value) { SetPropertyImpl<true>(name, value); }

    void Set(std::string_view name, uint32_t value)        { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2u &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3u &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4u &value) { SetPropertyImpl<true>(name, value); }

    void Set(std::string_view name, int32_t value)         { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2i &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3i &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4i &value) { SetPropertyImpl<true>(name, value); }

    void Set(std::string_view name, const RHI::TextureSRVPtr &srv)  { SetPropertyImpl<false>(name, srv); }
    void Set(std::string_view name, const RHI::BufferSRVPtr &srv)   { SetPropertyImpl<false>(name, srv); }
    void Set(std::string_view name, const RHI::SamplerPtr &sampler) { SetPropertyImpl<false>(name, sampler); }

private:

    template<bool CBuffer, typename T>
    void SetPropertyImpl(std::string_view name, const T &value)
    {
        properties_.Set(name, value);
        if constexpr(CBuffer)
        {
            InvalidateConstantBuffers();
        }
        else
        {
            InvalidateBindingGroups();
        }
    }

    void InvalidateConstantBuffers();
    void InvalidateBindingGroups();

    RC<const Material> parentMaterial_;
    MaterialPropertySheet properties_;
    std::vector<Box<SubMaterialInstance>> subMaterialInstances_;
};

RTRC_END
