#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

class ShaderResourceRegisterAllocator
{
public:

    static Box<ShaderResourceRegisterAllocator> Create(RHI::BackendType type);

    virtual ~ShaderResourceRegisterAllocator() = default;

    virtual void NewBindingGroup(int groupIndex) = 0;
    virtual void NewBinding(int slot, RHI::BindingType type) = 0;

    virtual const std::string &GetPrefix() const = 0;
    virtual const std::string &GetSuffix() const = 0;
};

class D3D12ShaderResourceRegisterAllocator : public ShaderResourceRegisterAllocator
{
public:

    // b, s, t, u
    static const char *GetD3D12RegisterType(RHI::BindingType type);

    void NewBindingGroup(int groupIndex) override;
    void NewBinding(int slot, RHI::BindingType type) override;

    const std::string& GetPrefix() const override;
    const std::string& GetSuffix() const override;

private:

    int groupIndex_ = 0;
    int s_ = 0; int lastBindingIndexOfS_ = -1;
    int b_ = 0; int lastBindingIndexOfB_ = -1;
    int t_ = 0; int lastBindingIndexOfT_ = -1;
    int u_ = 0; int lastBindingIndexOfU_ = -1;
    std::string suffix_;
};

class VulkanShaderResourceRegisterAllocator : public ShaderResourceRegisterAllocator
{
public:

    void NewBindingGroup(int groupIndex) override;
    void NewBinding(int slot, RHI::BindingType type) override;

    const std::string& GetPrefix() const override;
    const std::string& GetSuffix() const override;

private:

    int groupIndex_ = 0;
    std::string prefix_;
};

RTRC_END
