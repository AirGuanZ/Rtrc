#include <fmt/format.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Shader/Shader.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/String.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_BEGIN

namespace
{

    std::string GenerateArraySizeSpecifier(const BindingInfo &desc)
    {
        if(desc.arraySize.has_value())
        {
            return fmt::format("[{}]", desc.arraySize.value());
        }
        return {};
    }

    std::string GenerateHLSLTypeName(RHI::BindingType type)
    {
        using enum RHI::BindingType;
        switch(type)
        {
        case Texture2D:          return "Texture2D";
        case RWTexture2D:        return "RWTexture2D";
        case Buffer:             return "Buffer";
        case RWBuffer:           return "RWBuffer";
        case StructuredBuffer:   return "StructuredBuffer";
        case RWStructuredBuffer: return "RWStructuredBuffer";
        case ConstantBuffer:     return "ConstantBuffer";
        case Sampler:            return "SamplerState";
        }
        Unreachable();
    }

    std::string GetPrefixTypeName(const TypeInfo *info)
    {
        switch(info->type)
        {
        case StructMemberType::Int:    return "int";
        case StructMemberType::Int2:   return "int2";
        case StructMemberType::Int3:   return "int3";
        case StructMemberType::Int4:   return "int4";
        case StructMemberType::UInt:   return "uint";
        case StructMemberType::UInt2:  return "uint2";
        case StructMemberType::UInt3:  return "uint3";
        case StructMemberType::UInt4:  return "uint4";
        case StructMemberType::Float:  return "float";
        case StructMemberType::Float2: return "float2";
        case StructMemberType::Float3: return "float3";
        case StructMemberType::Float4: return "float4";
        case StructMemberType::Struct: return info->structInfo->name;
        case StructMemberType::Array:  return GetPrefixTypeName(info->arrayInfo->elementType);
        }
        Unreachable();
    }

    std::string GetSuffixTypeName(const TypeInfo *info)
    {
        if(info->type == StructMemberType::Array)
        {
            return fmt::format("[{}]{}", info->arrayInfo->arraySize, GetSuffixTypeName(info->arrayInfo->elementType));
        }
        return {};
    }

    void GenerateStructDefinition(
        std::string &result, std::set<std::string> &generatedStructName, const TypeInfo *info)
    {
        if(info->type == StructMemberType::Array)
        {
            GenerateStructDefinition(result, generatedStructName, info->arrayInfo->elementType);
        }
        else if(info->type != StructMemberType::Struct)
        {
            return;
        }

        auto structInfo = info->structInfo;
        if(generatedStructName.contains(structInfo->name))
        {
            return;
        }

        for(auto &mem : structInfo->members)
        {
            GenerateStructDefinition(result, generatedStructName, mem.type);
        }

        result += fmt::format("struct {}\n", structInfo->name);
        result += "{\n";
        for(auto &mem : structInfo->members)
        {
            result += fmt::format("    {} {}{};\n", GetPrefixTypeName(mem.type), mem.name, GetSuffixTypeName(mem.type));
        }
        result += "};\n";

        generatedStructName.insert(structInfo->name);
    }

    std::string GenerateHLSLBindingDeclarationForVulkan(
        std::set<std::string> &generatedStructName, const BindingGroupLayoutInfo &info, int setIndex)
    {
        std::string result;
        for(auto &&[bindingIndex, aliasedBindings] : Enumerate(info.bindings))
        {
            for(auto &binding : aliasedBindings)
            {
                if(binding.templateParameter == RHI::BindingTemplateParameterType::Undefined)
                {
                    const std::string typeName = GenerateHLSLTypeName(binding.type);
                    const std::string line = fmt::format(
                        "[[vk::binding({}, {})]] {} {}{};\n",
                        bindingIndex, setIndex, typeName, binding.name, GenerateArraySizeSpecifier(binding));
                    result += line;
                }
                else
                {
                    if(binding.structInfo)
                    {
                        const TypeInfo dummyTypeInfo = {
                            .type = StructMemberType::Struct,
                            .structInfo = binding.structInfo
                        };
                        GenerateStructDefinition(result, generatedStructName, &dummyTypeInfo);
                    }

                    std::string templateParamName;
                    switch(binding.templateParameter)
                    {
                    case RHI::BindingTemplateParameterType::Int:    templateParamName = "int";    break;
                    case RHI::BindingTemplateParameterType::Int2:   templateParamName = "int2";   break;
                    case RHI::BindingTemplateParameterType::Int3:   templateParamName = "int3";   break;
                    case RHI::BindingTemplateParameterType::Int4:   templateParamName = "int4";   break;
                    case RHI::BindingTemplateParameterType::UInt:   templateParamName = "uint";   break;
                    case RHI::BindingTemplateParameterType::UInt2:  templateParamName = "uint2";  break;
                    case RHI::BindingTemplateParameterType::UInt3:  templateParamName = "uint3";  break;
                    case RHI::BindingTemplateParameterType::UInt4:  templateParamName = "uint4";  break;
                    case RHI::BindingTemplateParameterType::Float:  templateParamName = "float";  break;
                    case RHI::BindingTemplateParameterType::Float2: templateParamName = "float2"; break;
                    case RHI::BindingTemplateParameterType::Float3: templateParamName = "float3"; break;
                    case RHI::BindingTemplateParameterType::Float4: templateParamName = "float4"; break;
                    case RHI::BindingTemplateParameterType::Struct: templateParamName = binding.structInfo->name; break;
                    default: break;
                    }

                    const std::string typeName = GenerateHLSLTypeName(binding.type);
                    const std::string line = fmt::format(
                        "[[vk::binding({}, {})]] {}<{}> {}{};\n",
                        bindingIndex, setIndex, typeName, templateParamName,
                        binding.name, GenerateArraySizeSpecifier(binding));

                    result += line;
                }
            }
        }
        return result;
    }

} // namespace anonymous

Shader::Shader(RC<RHI::RawShader> vertexShader, RC<RHI::RawShader> fragmentShader, RC<RHI::RawShader> computeShader)
    : vertexShader_(std::move(vertexShader)),
      fragmentShader_(std::move(fragmentShader)),
      computeShader_(std::move(computeShader))
{
    
}

const RC<RHI::RawShader> &Shader::GetVertexShader() const
{
    return vertexShader_;
}

const RC<RHI::RawShader> &Shader::GetFragmentShader() const
{
    return fragmentShader_;
}

const RC<RHI::RawShader> &Shader::GetComputeShader() const
{
    return computeShader_;
}

ShaderCompiler &ShaderCompiler::SetTarget(Target target)
{
    target_ = target;
    return *this;
}

ShaderCompiler &ShaderCompiler::SetDebugMode(bool debug)
{
    debug_ = debug;
    return *this;
}

ShaderCompiler &ShaderCompiler::SetVertexShaderSource(std::string source, std::string entry)
{
    vertexShaderSource_.source = std::move(source);
    vertexShaderSource_.entry = std::move(entry);
    return *this;
}

ShaderCompiler &ShaderCompiler::SetFragmentShaderSource(std::string source, std::string entry)
{
    fragmentShaderSource_.source = std::move(source);
    fragmentShaderSource_.entry = std::move(entry);
    return *this;
}

ShaderCompiler &ShaderCompiler::SetComputeShaderSource(std::string source, std::string entry)
{
    computeShaderSource_.source = std::move(source);
    computeShaderSource_.entry = std::move(entry);
    return *this;
}

ShaderCompiler &ShaderCompiler::AddMacro(std::string name, std::string value)
{
    macros_.insert({ std::move(name), std::move(value) });
    return *this;
}

ShaderCompiler &ShaderCompiler::AddBindingGroup(const BindingGroupLayoutInfo *info)
{
    bindingGroupLayouts_.push_back(info);
    return *this;
}

RC<Shader> ShaderCompiler::Compile(RHI::Device &device) const
{
    switch(target_)
    {
    case Target::Vulkan:
        return CompileHLSLForVulkan(device);
    }
    Unreachable();
}

RC<Shader> ShaderCompiler::CompileHLSLForVulkan(RHI::Device &device) const
{
    std::set<std::string> generatedStructNames;

    std::string bindingDeclarations;
    for(auto &&[setIndex, group] : Enumerate(bindingGroupLayouts_))
    {
        auto decl = GenerateHLSLBindingDeclarationForVulkan(generatedStructNames, *group, static_cast<int>(setIndex));
        bindingDeclarations += decl;
    }

    bindingDeclarations = "#pragma once\n" + bindingDeclarations;
    const std::map<std::string, std::string> virtualFiles = { { "./RtrcBindings", bindingDeclarations }};

    DXC dxc;
    RC<RHI::RawShader> vertexShader, fragmentShader, computeShader;

    if(!vertexShaderSource_.source.empty())
    {
        const auto vertexShaderBlob = dxc.Compile(
            DXC::ShaderInfo{
                .source = vertexShaderSource_.source,
                .entryPoint = vertexShaderSource_.entry
            },
            DXC::Target::Vulkan_1_3_VS_6_0, debug_, virtualFiles);

        vertexShader = device.CreateShader(
            vertexShaderBlob.data(), vertexShaderBlob.size(),
            vertexShaderSource_.entry, RHI::ShaderStage::VertexShader);
    }

    if(!fragmentShaderSource_.source.empty())
    {
        const auto fragmentShaderBlob = dxc.Compile(
            DXC::ShaderInfo{
                .source = fragmentShaderSource_.source,
                .entryPoint = fragmentShaderSource_.entry
            },
            DXC::Target::Vulkan_1_3_PS_6_0, debug_, virtualFiles);
        fragmentShader = device.CreateShader(
            fragmentShaderBlob.data(), fragmentShaderBlob.size(),
            fragmentShaderSource_.entry, RHI::ShaderStage::FragmentShader);
    }

    if(!computeShaderSource_.source.empty())
    {
        const auto computeShaderBlob = dxc.Compile(
            DXC::ShaderInfo{
                .source = computeShaderSource_.source,
                .entryPoint = computeShaderSource_.entry
            },
            DXC::Target::Vulkan_1_3_CS_6_0, debug_, virtualFiles);

        computeShader = device.CreateShader(
            computeShaderBlob.data(), computeShaderBlob.size(),
            computeShaderSource_.entry, RHI::ShaderStage::ComputeShader);
    }

    return MakeRC<Shader>(std::move(vertexShader), std::move(fragmentShader), std::move(computeShader));
}

RTRC_END
