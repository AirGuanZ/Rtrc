#include <fmt/format.h>
#include <spirv_reflect.h>

#include <Rtrc/Shader/ShaderReflection.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_BEGIN

namespace
{

    std::string_view GetSPIRVReflectResultName(SpvReflectResult result)
    {
        static std::string_view names[] =
        {
            "SPV_REFLECT_RESULT_SUCCESS",
            "SPV_REFLECT_RESULT_NOT_READY",
            "SPV_REFLECT_RESULT_ERROR_PARSE_FAILED",
            "SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED",
            "SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED",
            "SPV_REFLECT_RESULT_ERROR_NULL_POINTER",
            "SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR",
            "SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH",
            "SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT",
            "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE"
        };
        const int index = result;
        if(index < 0 || index >= GetArraySize<int>(names))
        {
            return "SPV_REFLECT_RESULT_ERROR_UNKNOWN";
        }
        return names[index];
    }

    void Check(SpvReflectResult result, std::string_view errMsg)
    {
        if(result != SPV_REFLECT_RESULT_SUCCESS)
        {
            throw Exception(fmt::format("{}. error code: {}", errMsg, GetSPIRVReflectResultName(result)));
        }
    }

    void ReflectInputVariables(
        const SpvReflectShaderModule &shaderModule, const char *entryPoint, ShaderReflection &result)
    {
        assert(result.inputVariables.empty());

        uint32_t count;
        Check(
            spvReflectEnumerateEntryPointInputVariables(&shaderModule, entryPoint, &count, nullptr),
            "failed to reflect input variable count of spv shader");
        std::vector<SpvReflectInterfaceVariable*> inputVars(count);
        Check(
            spvReflectEnumerateEntryPointInputVariables(&shaderModule, entryPoint, &count, inputVars.data()),
            "failed to reflect input variables of spv shader");

        result.inputVariables.reserve(count);
        for(uint32_t i = 0; i < count; ++i)
        {
            auto &in = *inputVars[i];
            auto &out = result.inputVariables.emplace_back();
            out.semantic = in.semantic ? in.semantic : std::string{};
            out.location = static_cast<int>(in.location);
            out.isBuiltin = (in.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) != 0;
            switch(in.format)
            {
            case SPV_REFLECT_FORMAT_R32_UINT:            out.type = RHI::VertexAttributeType::UInt;   break;
            case SPV_REFLECT_FORMAT_R32G32_UINT:         out.type = RHI::VertexAttributeType::UInt2;  break;
            case SPV_REFLECT_FORMAT_R32G32B32_UINT:      out.type = RHI::VertexAttributeType::UInt3;  break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   out.type = RHI::VertexAttributeType::UInt4;  break;
            case SPV_REFLECT_FORMAT_R32_SINT:            out.type = RHI::VertexAttributeType::Int;    break;
            case SPV_REFLECT_FORMAT_R32G32_SINT:         out.type = RHI::VertexAttributeType::Int2;   break;
            case SPV_REFLECT_FORMAT_R32G32B32_SINT:      out.type = RHI::VertexAttributeType::Int3;   break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   out.type = RHI::VertexAttributeType::Int4;   break;
            case SPV_REFLECT_FORMAT_R32_SFLOAT:          out.type = RHI::VertexAttributeType::Float;  break;
            case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       out.type = RHI::VertexAttributeType::Float2; break;
            case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    out.type = RHI::VertexAttributeType::Float3; break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: out.type = RHI::VertexAttributeType::Float4; break;
            default: throw Exception("unsupported input variable format");
            }
        }
    }

} // namespace anonymous

void ReflectSPIRV(Span<unsigned char> code, const char *entryPoint, ShaderReflection &result)
{
    SpvReflectShaderModule shaderModule = {};
    Check(
        spvReflectCreateShaderModule(code.size(), code.GetData(), &shaderModule),
        "failed to reflect spv shader");
    RTRC_SCOPE_EXIT{ spvReflectDestroyShaderModule(&shaderModule); };

    if(shaderModule.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
    {
        ReflectInputVariables(shaderModule, entryPoint, result);
    }
}

RTRC_END
