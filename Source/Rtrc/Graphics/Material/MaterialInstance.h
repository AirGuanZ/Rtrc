#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Resource/BindlessTextureManager.h>
#include <Rtrc/Utility/SmartPointer/CopyOnWritePtr.h>

RTRC_BEGIN

class CommandBuffer;
class MaterialInstance;

class MaterialPropertySheet : public Uncopyable
{
public:

    using MaterialResource = MaterialPassPropertyLayout::MaterialResource;

    explicit MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout);

    void CopyFrom(const MaterialPropertySheet &other);

    void Set(std::string_view name, float           value);
    void Set(std::string_view name, const Vector2f &value);
    void Set(std::string_view name, const Vector3f &value);
    void Set(std::string_view name, const Vector4f &value);

    void Set(std::string_view name, uint32_t        value);
    void Set(std::string_view name, const Vector2u &value);
    void Set(std::string_view name, const Vector3u &value);
    void Set(std::string_view name, const Vector4u &value);

    void Set(std::string_view name, int32_t         value);
    void Set(std::string_view name, const Vector2i &value);
    void Set(std::string_view name, const Vector3i &value);
    void Set(std::string_view name, const Vector4i &value);

    void Set(std::string_view name, const RC<Texture> &tex);
    void Set(std::string_view name, const TextureSrv  &srv);
    void Set(std::string_view name, const BufferSrv   &srv);
    void Set(std::string_view name, const RC<Sampler> &sampler);
    void Set(std::string_view name, const RC<Tlas>    &tlas);

    void Set(std::string_view name, const BindlessTextureEntry &entry);

    const unsigned char *GetValueBuffer() const { return valueBuffer_.data(); }
    Span<MaterialResource> GetResources() const { return resources_; }

private:

    template<MaterialProperty::Type Type, typename T>
    void SetImpl(std::string_view name, const T &value);

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

    RC<Shader> GetShader(const KeywordValueContext &keywordValues);
    RC<Shader> GetShader(KeywordSet::ValueMask keywordMask);

    const MaterialPass *GetPass() const;

    void BindGraphicsProperties(const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindGraphicsProperties(KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const;

    void BindComputeProperties(const KeywordValueContext &keywordValues, const CommandBuffer &commandBuffer) const;
    void BindComputeProperties(KeywordSet::ValueMask mask, const CommandBuffer &commandBuffer) const;

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
    void BindPropertiesImpl(KeywordSet::ValueMask mask, const RHI::CommandBufferPtr &commandBuffer) const;

    Device                 *device_ = nullptr;
    const MaterialInstance *materialInstance_ = nullptr;
    MaterialPass           *pass_;

    int                       recordCount_;
    std::unique_ptr<Record[]> keywordMaskToRecord_;
};

void BindMaterialProperties(
    const MaterialPassInstance &instance,
    const KeywordValueContext  &keywords,
    const CommandBuffer        &commandBuffer,
    bool                        graphics);

class MaterialInstance
{
public:

    class SharedRenderingData : public ReferenceCounted, public Uncopyable
    {
    public:

        SharedRenderingData(RC<const Material> material, const MaterialInstance *materialInstance, Device *device);

        SharedRenderingData *Clone() const;

        const RC<const Material> &GetMaterial() const { return material_; }
        const MaterialPropertySheet &GetPropertySheet() const { return propertySheet_; }

        MaterialPassInstance *GetPassInstance(std::string_view tag) const;
        MaterialPassInstance *GetPassInstance(size_t index) const;

    private:

        friend class MaterialInstance;

        Device                                *device_;
        RC<const Material>                     material_;
        MaterialPropertySheet                  propertySheet_;
        std::vector<Box<MaterialPassInstance>> passInstances_;
    };

    MaterialInstance(RC<const Material> material, Device *device);

    const RC<const Material> &GetMaterial() const { return data_->GetMaterial(); }
    const MaterialPropertySheet &GetPropertySheet() const { return data_->GetPropertySheet(); }

    // return nullptr when not found
    MaterialPassInstance *GetPassInstance(std::string_view tag) const { return data_->GetPassInstance(tag); }
    MaterialPassInstance *GetPassInstance(size_t index) const { return data_->GetPassInstance(index); }
    
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

    void Set(std::string_view name, const TextureSrv &srv) { SetPropertyImpl<false>(name, srv);     }
    void Set(std::string_view name, const BufferSrv &srv)  { SetPropertyImpl<false>(name, srv);     }
    void Set(std::string_view name, RC<Sampler> sampler)   { SetPropertyImpl<false>(name, sampler); }

    template<typename T>
    void Set(std::string_view name, const T &value)
    {
        data_.Unshare()->propertySheet_.Set(name, value);
    }

    const ReferenceCountedPtr<SharedRenderingData> &GetRenderingData() const { return data_; }
    SharedRenderingData                            *GetMutableRenderingData() { return data_.Unshare(); }

private:

    template<bool CBuffer, typename T>
    void SetPropertyImpl(std::string_view name, const T &value)
    {
        data_.Unshare()->propertySheet_.Set(name, value);
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

    CopyOnWritePtr<SharedRenderingData> data_;
};

RTRC_END
