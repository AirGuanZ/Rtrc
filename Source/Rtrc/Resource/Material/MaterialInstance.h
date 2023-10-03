#pragma once

#include <Rtrc/Resource/BindlessResourceManager.h>
#include <Rtrc/Resource/Material/Material.h>

RTRC_BEGIN

class CommandBuffer;
class MaterialInstance;

class MaterialPropertySheet : public Uncopyable
{
public:

    using MaterialResource = MaterialPassPropertyLayout::MaterialResource;

    explicit MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout);

    void CopyFrom(const MaterialPropertySheet &other);

#define RTRC_DECL_SET(TYPE) \
    void Set(ShaderPropertyName name, TYPE value); \
    void Set(int index, TYPE value);

    RTRC_DECL_SET(float           )
    RTRC_DECL_SET(const Vector2f &)
    RTRC_DECL_SET(const Vector3f &)
    RTRC_DECL_SET(const Vector4f &)
    
    RTRC_DECL_SET(uint32_t        )
    RTRC_DECL_SET(const Vector2u &)
    RTRC_DECL_SET(const Vector3u &)
    RTRC_DECL_SET(const Vector4u &)
    
    RTRC_DECL_SET(int32_t         )
    RTRC_DECL_SET(const Vector2i &)
    RTRC_DECL_SET(const Vector3i &)
    RTRC_DECL_SET(const Vector4i &)

    RTRC_DECL_SET(const RC<Texture> &)
    RTRC_DECL_SET(const TextureSrv  &)
    RTRC_DECL_SET(const BufferSrv   &)
    RTRC_DECL_SET(const RC<Sampler> &)
    RTRC_DECL_SET(const RC<Tlas>    &)

#undef RTRC_DECL_SET

    void Set(ShaderPropertyName, const BindlessTextureEntry &entry);
    void Set(int index,            const BindlessTextureEntry &entry);

    const unsigned char *GetValueBuffer() const { return valueBuffer_.data(); }
    Span<MaterialResource> GetResources() const { return resources_; }

    const RC<MaterialPropertyHostLayout> &GetHostLayout() const { return layout_; }

    const unsigned char *GetValue(ShaderPropertyName name) const; // Returns nullptr if not found
    const unsigned char *GetValue(int index) const;

    const BindlessTextureEntry *GetBindlessTextureEntry(ShaderPropertyName name) const; // Returns nullptr if not found
    const BindlessTextureEntry *GetBindlessTextureEntry(int index) const;

private:

    template<MaterialProperty::Type Type, typename T>
    void SetImpl(ShaderPropertyName name, const T &value);

    template<MaterialProperty::Type Type, typename T>
    void SetImpl(int index, const T &value);

    RC<MaterialPropertyHostLayout>    layout_;
    std::vector<unsigned char>        valueBuffer_;
    std::vector<MaterialResource>     resources_;
    std::vector<BindlessTextureEntry> bindlessEntries_; // size == valueProperties.size
};

/*
    Thread Safety:

        TS(1)
            BindGraphicsProperties
            BindComputeProperties

        TS(0)
            ParentMaterialInstance::Set
*/
class MaterialPassInstance
{
public:

    RC<Shader> GetShader(const FastKeywordContext &keywordValues);
    RC<Shader> GetShader(FastKeywordSetValue keywordMask);

    const MaterialPass *GetPass() const;

    void BindGraphicsProperties(const FastKeywordContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindGraphicsProperties(FastKeywordSetValue mask, const CommandBuffer &commandBuffer) const;
    void BindGraphicsProperties(const CommandBuffer &commandBuffer) const;

    void BindComputeProperties(const FastKeywordContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindComputeProperties(FastKeywordSetValue mask, const CommandBuffer &commandBuffer) const;
    void BindComputeProperties(const CommandBuffer &commandBuffer) const;

private:

    friend class MaterialInstance;

    struct Record
    {
        mutable RC<BindingGroup>   bindingGroup;
        mutable RC<DynamicBuffer>  constantBuffer;
        mutable bool               isConstantBufferDirty = true;
        mutable tbb::spin_rw_mutex mutex;
    };

    // return binding group index
    template<bool Graphics>
    void BindPropertiesImpl(FastKeywordSetValue mask, const RHI::CommandBufferPtr &commandBuffer) const;

    Device                 *device_ = nullptr;
    const MaterialInstance *materialInstance_ = nullptr;
    MaterialPass           *pass_;

    int                       recordCount_;
    std::unique_ptr<Record[]> keywordMaskToRecord_;
};

void BindMaterialProperties(
    const MaterialPassInstance &instance,
    const FastKeywordContext       &keywords,
    const CommandBuffer        &commandBuffer,
    bool                        graphics);

class MaterialInstance : public WithUniqueObjectID, public Uncopyable
{
public:

    MaterialInstance(RC<const Material> material, Device *device);

    const RC<const Material> &GetMaterial() const { return material_; }
    const MaterialPropertySheet &GetPropertySheet() const { return propertySheet_; }

    // return nullptr when not found
    MaterialPassInstance *GetPassInstance(MaterialPassTag tag) const;
    MaterialPassInstance *GetPassInstance(size_t index) const;
    
    void SetFloat(ShaderPropertyName name, float value)      { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector2f &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector3f &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector4f &value) { SetPropertyImpl<true>(name, value); }

    void SetUInt(ShaderPropertyName name, uint32_t value)    { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector2u &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector3u &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector4u &value) { SetPropertyImpl<true>(name, value); }

    void SetInt(ShaderPropertyName name, int32_t value)      { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector2i &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector3i &value) { SetPropertyImpl<true>(name, value); }
    void Set(ShaderPropertyName name, const Vector4i &value) { SetPropertyImpl<true>(name, value); }

    void Set(ShaderPropertyName name, const TextureSrv &srv) { SetPropertyImpl<false>(name, srv); }
    void Set(ShaderPropertyName name, const BufferSrv &srv)  { SetPropertyImpl<false>(name, srv); }
    void Set(ShaderPropertyName name, RC<Sampler> sampler)   { SetPropertyImpl<false>(name, sampler); }

    template<typename T>
    void Set(ShaderPropertyName name, const T &value)
    {
        propertySheet_.Set(name, value);
    }
    
private:

    template<bool CBuffer, typename T>
    void SetPropertyImpl(ShaderPropertyName name, const T &value)
    {
        propertySheet_.Set(name, value);
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

    RC<const Material>                     material_;
    MaterialPropertySheet                  propertySheet_;
    std::vector<Box<MaterialPassInstance>> passInstances_;
};

RTRC_END
