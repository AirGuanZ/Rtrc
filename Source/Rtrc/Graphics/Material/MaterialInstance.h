#pragma once

#include <Rtrc/Graphics/Material/Material.h>

RTRC_BEGIN

class CommandBuffer;
class MaterialInstance;

class MaterialPropertySheet
{
public:

    using MaterialResource = SubMaterialPropertyLayout::MaterialResource;

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

    void Set(std::string_view name, const TextureSRV &srv);
    void Set(std::string_view name, const BufferSRV &srv);
    void Set(std::string_view name, RC<Sampler> sampler);

    const unsigned char *GetValueBuffer() const { return valueBuffer_.data(); }
    Span<MaterialResource> GetResources() const { return resources_; }

private:

    template<MaterialProperty::Type Type, typename T>
    void SetImpl(std::string_view name, const T &value);

    RC<MaterialPropertyHostLayout> layout_;
    std::vector<unsigned char> valueBuffer_;
    std::vector<MaterialResource> resources_;
};

/*
    Thread Safety:

        TS(1)
            BindGraphicsProperties
            BindComputeProperties

        TS(0)
            ParentMaterialInstance::Set
*/
class SubMaterialInstance
{
public:

    RC<Shader> GetShader(const KeywordValueContext &keywordValues);
    RC<Shader> GetShader(KeywordSet::ValueMask keywordMask);

    void BindGraphicsProperties(const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindGraphicsProperties(KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const;

    void BindComputeProperties(const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindComputeProperties(KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const;

private:

    friend class MaterialInstance;

    struct Record
    {
        mutable RC<BindingGroup> bindingGroup;
        mutable RC<ConstantBuffer> constantBuffer;
        mutable bool isConstantBufferDirty = true;
        mutable tbb::spin_rw_mutex mutex;
    };

    // return binding group index
    template<bool Graphics>
    void BindPropertiesImpl(KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer) const;

    RenderContext *renderContext_ = nullptr;
    const MaterialInstance *parentMaterialInstance_ = nullptr;
    RC<SubMaterial> subMaterial_;

    std::unique_ptr<Record[]> keywordMaskToRecord_;
    int recordCount_;
};

class MaterialInstance
{
public:

    MaterialInstance(RC<const Material> material, RenderContext *renderContext);

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

    void SetFloat(std::string_view name, float value)      { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2f &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3f &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4f &value) { SetPropertyImpl<true>(name, value); }

    void SetUInt(std::string_view name, uint32_t value)    { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2u &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3u &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4u &value) { SetPropertyImpl<true>(name, value); }

    void SetInt(std::string_view name, int32_t value)      { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector2i &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector3i &value) { SetPropertyImpl<true>(name, value); }
    void Set(std::string_view name, const Vector4i &value) { SetPropertyImpl<true>(name, value); }

    void Set(std::string_view name, const TextureSRV &srv) { SetPropertyImpl<false>(name, srv); }
    void Set(std::string_view name, const BufferSRV &srv)  { SetPropertyImpl<false>(name, srv); }
    void Set(std::string_view name, RC<Sampler> sampler)   { SetPropertyImpl<false>(name, sampler); }

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
