#include <Graphics/Shader/D3D12Reflection.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dxc/dxcapi.h>
#include <dxc/d3d12shader.h>
#include <wrl/client.h>

RTRC_BEGIN

namespace D3D12ReflectionDetail
{

    std::string_view DemangleFunctionName(std::string_view name)
    {
        if(!name.starts_with("\01?"))
        {
            return name;
        }
        size_t nameEnd = name.find('@');
        assert(nameEnd != std::string_view::npos);
        return name.substr(2, nameEnd - 2);
    }

} // namespace D3D12ReflectionDetail

D3D12Reflection::D3D12Reflection(IDxcUtils *dxcUtils, Span<std::byte> code, bool isLibrary)
{
    if(isLibrary)
    {
        InitializeLibraryReflection(dxcUtils, code);
    }
    else
    {
        InitializeRegularReflection(dxcUtils, code);
    }
}

std::vector<RHI::RawShaderEntry> D3D12Reflection::GetEntries() const
{
    return entries_;
}

bool D3D12Reflection::IsBindingUsed(std::string_view name) const
{
    return usedBindings_.contains(name);
}

void D3D12Reflection::InitializeRegularReflection(IDxcUtils *dxcUtils, Span<std::byte> code)
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
        var.semantic  = RHI::VertexSemantic(signatureParameterDesc.SemanticName, signatureParameterDesc.SemanticIndex);
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

void D3D12Reflection::InitializeLibraryReflection(IDxcUtils *dxcUtils, Span<std::byte> code)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<ID3D12LibraryReflection> reflection;
    const DxcBuffer codeBuffer =
    {
        .Ptr      = code.GetData(),
        .Size     = code.GetSize(),
        .Encoding = 0
    };
    if(FAILED(dxcUtils->CreateReflection(&codeBuffer, IID_PPV_ARGS(reflection.GetAddressOf()))))
    {
        throw Exception("Fail to get directx12 shader library reflection");
    }
    D3D12_LIBRARY_DESC libraryDesc;
    if(FAILED(reflection->GetDesc(&libraryDesc)))
    {
        throw Exception("Fail to get directx12 shader library desc");
    }

    // Entries

    for(uint32_t functionIndex = 0; functionIndex < libraryDesc.FunctionCount; ++functionIndex)
    {
        ID3D12FunctionReflection *function = reflection->GetFunctionByIndex(functionIndex);
        D3D12_FUNCTION_DESC functionDesc;
        if(FAILED(function->GetDesc(&functionDesc)))
        {
            throw Exception(fmt::format(
                "Fail to get directx12 function desc {} from library reflection", functionIndex));
        }

        RHI::ShaderStage stage;
        const uint32_t programType = (functionDesc.Version & 0xffff0000) >> 16;
        switch(programType)
        {
        case D3D12_SHVER_PIXEL_SHADER:          stage = RHI::ShaderStage::FragmentShader;        break;
        case D3D12_SHVER_VERTEX_SHADER:         stage = RHI::ShaderStage::VertexShader;          break;
        case D3D12_SHVER_COMPUTE_SHADER:        stage = RHI::ShaderStage::ComputeShader;         break;
        case D3D12_SHVER_RAY_GENERATION_SHADER: stage = RHI::ShaderStage::RT_RayGenShader;       break;
        case D3D12_SHVER_INTERSECTION_SHADER:   stage = RHI::ShaderStage::RT_IntersectionShader; break;
        case D3D12_SHVER_ANY_HIT_SHADER:        stage = RHI::ShaderStage::RT_AnyHitShader;       break;
        case D3D12_SHVER_CLOSEST_HIT_SHADER:    stage = RHI::ShaderStage::RT_ClosestHitShader;   break;
        case D3D12_SHVER_MISS_SHADER:           stage = RHI::ShaderStage::RT_MissShader;         break;
        case D3D12_SHVER_CALLABLE_SHADER:       stage = RHI::ShaderStage::CallableShader;        break;
        default:
            throw Exception(fmt::format(
                "Invalid directx12 shader program type {} in library", programType));
        }

        auto &entry = entries_.emplace_back();
        entry.name = D3D12ReflectionDetail::DemangleFunctionName(functionDesc.Name);
        entry.stage = stage;
    }

    // TODO: Used bindings (not necessary for basic implementation)
}

RTRC_END
