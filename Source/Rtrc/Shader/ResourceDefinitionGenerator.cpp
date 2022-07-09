#include <fmt/format.h>

#include <Rtrc/Shader/ResourceDefinitionGenerator.h>
#include <Rtrc/Utils/Enumerate.h>
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

} // namespace anonymous

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
                case RHI::BindingTemplateParameterType::Undefined: break;
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

RTRC_END
