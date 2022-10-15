#pragma once

#include <any>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Object/BatchReleaseHelper.h>

RTRC_BEGIN

class BindingGroupLayout;
class BindingLayoutManager;
class ConstantBuffer;

struct BufferSRV;
struct BufferUAV;
struct TextureSRV;
struct TextureUAV;
class Sampler;

class BindingGroup : public Uncopyable
{
public:

    BindingGroup() = default;
    ~BindingGroup();

    const RC<const BindingGroupLayout> &GetLayout() const;

    void Set(int slot, RC<ConstantBuffer> cbuffer);
    void Set(int slot, RC<Sampler>        sampler);
    void Set(int slot, const BufferSRV   &srv);
    void Set(int slot, const BufferUAV   &uav);
    void Set(int slot, const TextureSRV  &srv);
    void Set(int slot, const TextureUAV  &uav);

    void Set(std::string_view name, RC<ConstantBuffer> cbuffer);
    void Set(std::string_view name, RC<Sampler>        sampler);
    void Set(std::string_view name, const BufferSRV   &srv);
    void Set(std::string_view name, const BufferUAV   &uav);
    void Set(std::string_view name, const TextureSRV  &srv);
    void Set(std::string_view name, const TextureUAV  &uav);

    const RHI::BindingGroupPtr &GetRHIObject();

private:

    friend class BindingLayoutManager;

    BindingLayoutManager *manager_ = nullptr;
    RC<const BindingGroupLayout> layout_;
    RHI::BindingGroupPtr rhiGroup_;
    std::vector<std::any> boundObjects_;
};

class BindingGroupLayout :
    public Uncopyable,
    public std::enable_shared_from_this<BindingGroupLayout>,
    public WithUniqueObjectID
{
public:

    BindingGroupLayout() = default;
    ~BindingGroupLayout();

    int GetBindingSlotByName(std::string_view name) const;

    const RHI::BindingGroupLayoutPtr &GetRHIObject();

    RC<BindingGroup> CreateBindingGroup() const;

private:

    struct Record
    {
        std::map<std::string, int, std::less<>> nameToSlot;
        RHI::BindingGroupLayoutPtr rhiLayout;
    };

    friend class BindingLayoutManager;

    BindingLayoutManager *manager_ = nullptr;
    RC<Record> record_;
};

class BindingLayout : public Uncopyable
{
public:

    struct Desc
    {
        std::vector<RC<BindingGroupLayout>> groupLayouts;

        auto operator<=>(const Desc &) const = default;
    };

    BindingLayout() = default;
    ~BindingLayout();

    const RHI::BindingLayoutPtr &GetRHIObject() const;

private:

    friend class BindingLayoutManager;

    struct Record
    {
        Desc desc; // group layouts will not be released before the parent binding layout is released
        RHI::BindingLayoutPtr rhiLayout;
    };

    BindingLayoutManager *manager_ = nullptr;
    RC<Record> record_;
};

class BindingLayoutManager : public Uncopyable
{
public:

    using BindingLayoutDesc = BindingLayout::Desc;

    BindingLayoutManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    RC<BindingGroupLayout> CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

    RC<BindingGroup> CreateBindingGroup(RC<const BindingGroupLayout> layout);

    RC<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc);

    void _rtrcReleaseInternal(BindingGroup &group);
    void _rtrcReleaseInternal(BindingGroupLayout &groupLayout);
    void _rtrcReleaseInternal(BindingLayout &layout);

private:

    using BindingGroupLayoutRecord = BindingGroupLayout::Record;
    using BindingLayoutRecord = BindingLayout::Record;

    struct ReleaseBindingGroupLayoutData
    {
        RHI::BindingGroupLayoutDesc desc;
        RHI::BindingGroupLayoutPtr rhiLayout;
    };

    struct ReleaseBindingGroupData
    {
        RHI::BindingGroupPtr rhiGroup;
    };

    struct ReleaseBindingLayoutData
    {
        BindingLayoutDesc desc;
        RHI::BindingLayoutPtr rhiLayout;
    };

    RHI::DevicePtr device_;

    std::map<RHI::BindingGroupLayoutDesc, std::weak_ptr<BindingGroupLayoutRecord>> groupLayoutCache_;
    tbb::spin_rw_mutex groupLayoutCacheMutex_;
    std::vector<ReleaseBindingGroupLayoutData> pendingGroupLayoutReleaseData_;
    tbb::spin_mutex pendingGroupLayoutReleaseDataMutex_;
    BatchReleaseHelper<ReleaseBindingGroupLayoutData> groupLayoutReleaser_;

    std::map<BindingLayoutDesc, std::weak_ptr<BindingLayoutRecord>> layoutCache_;
    tbb::spin_rw_mutex layoutCacheMutex_;
    std::vector<ReleaseBindingLayoutData> pendingLayoutReleaseData_;
    tbb::spin_mutex pendingLayoutReleaseDataMutex_;
    BatchReleaseHelper<ReleaseBindingLayoutData> layoutReleaser_;

    std::vector<ReleaseBindingGroupData> pendingGroupReleaseData_;
    tbb::spin_mutex pendingGroupReleaseDataMutex_;
    BatchReleaseHelper<ReleaseBindingGroupData> groupReleaser_;
};

RTRC_END
