#include <Rtrc/Graphics/Shader/D3D12Reflection.h>

#include <Windows.h>
#include <dxc/dxcapi.h>
#include <dxc/d3d12shader.h>
#include <wrl/client.h>


RTRC_BEGIN

D3D12Reflection::D3D12Reflection(
    IDxcUtils                                        *dxcUtils,
    Span<std::byte>                                   code,
    const std::map<std::string, std::pair<int, int>> &bindingNameToLocation)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<ID3D12ShaderReflection> reflection;
    const DxcBuffer codeBuffer =
    {
        .Ptr      = code.GetData(),
        .Size     = code.GetSize(),
        .Encoding = 0
    };
    dxcUtils->CreateReflection(&codeBuffer, IID_PPV_ARGS(reflection.GetAddressOf()));
    D3D12_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    // Input variables

    for(uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC signatureParameterDesc;
        if(FAILED(reflection->GetInputParameterDesc(i, &signatureParameterDesc)))
        {
            throw Exception(fmt::format( "Fail to get directx12 shader input parameter {}", i));
        }

        int dimension;
        switch(signatureParameterDesc.Mask)
        {
        case 0b1:    dimension = 1; break;
        case 0b11:   dimension = 2; break;
        case 0b111:  dimension = 3; break;
        case 0b1111: dimension = 4; break;
        default:
            throw Exception(fmt::format("Invalid input variable mask {}", signatureParameterDesc.Mask));
        }
        
        RHI::VertexAttributeType vertexAttributeType;

        if(signatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        {
            vertexAttributeType = static_cast<RHI::VertexAttributeType>(
                std::to_underlying(RHI::VertexAttributeType::UInt) + (dimension - 1));
        }
        else if(signatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        {
            vertexAttributeType = static_cast<RHI::VertexAttributeType>(
                std::to_underlying(RHI::VertexAttributeType::Int) + (dimension - 1));
        }
        else if(signatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
        {
            vertexAttributeType = static_cast<RHI::VertexAttributeType>(
                std::to_underlying(RHI::VertexAttributeType::Float) + (dimension - 1));
        }
        else
        {
            throw Exception("Unknown input variable component type");
        }

        auto &var = inputVariables_.emplace_back();
        var.semantic  = VertexSemantic(signatureParameterDesc.SemanticName, signatureParameterDesc.SemanticIndex);
        var.type      = vertexAttributeType;
        var.isBuiltin = signatureParameterDesc.SystemValueType != D3D_NAME_UNDEFINED;
        var.location  = -1; // Not used by directx12 backend
    }
    
    // Thread group size

    UINT x = 0, y = 0, z = 0;
    reflection->GetThreadGroupSize(&x, &y, &z);
    threadGroupSize_ = Vector3i(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));

    // Used bindings

    for(uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
        if(FAILED(reflection->GetResourceBindingDesc(i, &bindDesc)))
        {
            throw Exception(fmt::format("Fail to get directx12 shader resource binding {}", i));
        }
        if(bindDesc.Type <= D3D_SIT_RTACCELERATIONSTRUCTURE && (bindDesc.uFlags & D3D_SVF_USED))
        {
            usedBindings_.insert(bindDesc.Name);
        }
    }
}

std::vector<RHI::RawShaderEntry> D3D12Reflection::GetEntries() const
{
    // TODO: for library
    return {};
}

bool D3D12Reflection::IsBindingUsed(std::string_view name) const
{
    return usedBindings_.contains(name);
}

RTRC_END
