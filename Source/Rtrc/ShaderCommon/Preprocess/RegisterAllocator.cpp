#include <Rtrc/ShaderCommon/Preprocess/RegisterAllocator.h>

RTRC_BEGIN

namespace ShaderResourceRegisterAllocatorDetail
{

    enum class D3D12RegisterType
    {
        B, // Constant buffer
        S, // Sampler
        T, // SRV
        U, // UAV
    };

    D3D12RegisterType BindingTypeToD3D12RegisterType(RHI::BindingType type)
    {
        using enum RHI::BindingType;
        using enum D3D12RegisterType;
        switch(type)
        {
        case Texture:
        case Buffer:
        case StructuredBuffer:
        case ByteAddressBuffer:
        case AccelerationStructure:
            return T;
        case ConstantBuffer:
            return B;
        case Sampler:
            return S;
        case RWTexture:
        case RWBuffer:
        case RWStructuredBuffer:
        case RWByteAddressBuffer:
            return U;
        }
        Unreachable();
    }

} // namespace ShaderResourceRegisterAllocatorDetail

Box<ShaderResourceRegisterAllocator> ShaderResourceRegisterAllocator::Create(RHI::BackendType type)
{
    if(type == RHI::BackendType::DirectX12)
    {
        return MakeBox<D3D12ShaderResourceRegisterAllocator>();
    }
    else
    {
        assert(type == RHI::BackendType::Vulkan);
        return MakeBox<VulkanShaderResourceRegisterAllocator>();
    }
}

const char* D3D12ShaderResourceRegisterAllocator::GetD3D12RegisterType(RHI::BindingType type)
{
    using ShaderResourceRegisterAllocatorDetail::D3D12RegisterType;
    switch(ShaderResourceRegisterAllocatorDetail::BindingTypeToD3D12RegisterType(type))
    {
    case D3D12RegisterType::B: return "b"; 
    case D3D12RegisterType::S: return "s"; 
    case D3D12RegisterType::T: return "t"; 
    case D3D12RegisterType::U: return "u"; 
    }
    Unreachable();
}

void D3D12ShaderResourceRegisterAllocator::NewBindingGroup(int groupIndex)
{
    groupIndex_ = groupIndex;
    s_ = b_ = t_ = u_ = 0;
    lastBindingIndexOfS_ = lastBindingIndexOfB_ = lastBindingIndexOfT_ = lastBindingIndexOfU_ = -1;
    suffix_ = {};
}

void D3D12ShaderResourceRegisterAllocator::NewBinding(int slot, RHI::BindingType type)
{
    using ShaderResourceRegisterAllocatorDetail::D3D12RegisterType;

    int *reg = nullptr; int *lastBindingIndex = nullptr; const char *regStr = nullptr;
    switch(ShaderResourceRegisterAllocatorDetail::BindingTypeToD3D12RegisterType(type))
    {
    case D3D12RegisterType::B: reg = &b_; lastBindingIndex = &lastBindingIndexOfB_; regStr = "b"; break;
    case D3D12RegisterType::S: reg = &s_; lastBindingIndex = &lastBindingIndexOfS_; regStr = "s"; break;
    case D3D12RegisterType::T: reg = &t_; lastBindingIndex = &lastBindingIndexOfT_; regStr = "t"; break;
    case D3D12RegisterType::U: reg = &u_; lastBindingIndex = &lastBindingIndexOfU_; regStr = "u"; break;
    }

    if(groupIndex_ < 0)
    {
        suffix_ = fmt::format(" : register({}0, space{})", regStr, 9000 + slot);
        return;
    }

    assert(*lastBindingIndex < slot &&
           "The allocation order for each type of register must be monotonic with respect to the binding index.");

    suffix_ = fmt::format(" : register({}{}, space{})", regStr, *reg, groupIndex_);
    ++*reg;
    *lastBindingIndex = slot;
}

const std::string& D3D12ShaderResourceRegisterAllocator::GetPrefix() const
{
    static const std::string ret;
    return ret;
}

const std::string& D3D12ShaderResourceRegisterAllocator::GetSuffix() const
{
    return suffix_;
}

void VulkanShaderResourceRegisterAllocator::NewBindingGroup(int groupIndex)
{
    groupIndex_ = groupIndex;
    prefix_ = {};
}

void VulkanShaderResourceRegisterAllocator::NewBinding(int slot, RHI::BindingType type)
{
    prefix_ = fmt::format("[[vk::binding({}, {})]] ", slot, groupIndex_);
}

const std::string& VulkanShaderResourceRegisterAllocator::GetPrefix() const
{
    return prefix_;
}

const std::string& VulkanShaderResourceRegisterAllocator::GetSuffix() const
{
    static const std::string ret;
    return ret;
}

RTRC_END
