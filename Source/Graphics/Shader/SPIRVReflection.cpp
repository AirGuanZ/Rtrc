#include <spirv_reflect.h>

#include <Graphics/Shader/SPIRVReflection.h>
#include <Core/ScopeGuard.h>
#include <Core/String.h>

RTRC_BEGIN

namespace ShaderReflDetail
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

    std::pair<std::string_view, int> ParseInputVarSemanticAndIndex(std::string_view inputName)
    {
        if(!StartsWith(inputName, "in.var."))
        {
            return { inputName, 0 };
        }

        inputName = inputName.substr(7);
        if(inputName.empty())
        {
            return { inputName, 0 };
        }

        // Find semantic string

        size_t semanticLastPos = inputName.size() - 1;
        while(semanticLastPos > 0 && std::isdigit(inputName[semanticLastPos]))
        {
            --semanticLastPos;
        }
        if(!semanticLastPos && std::isdigit(inputName[0]))
        {
            throw Exception(fmt::format("Invalid semantic name: {}", inputName));
        }
        const std::string_view semantic = inputName.substr(0, semanticLastPos + 1);

        // Parse semantic index

        int index = 0;
        if(semanticLastPos < inputName.size() - 1)
        {
            const std::string_view indexStr = inputName.substr(semanticLastPos + 1);
            index = std::stoi(std::string(indexStr));
        }

        return { semantic, index };
    }

    void GetInputVariables(
        const SpvReflectShaderModule &shaderModule, const char *entryPoint, std::vector<ShaderIOVar> &result)
    {
        assert(result.empty());

        uint32_t count;
        Check(
            spvReflectEnumerateEntryPointInputVariables(&shaderModule, entryPoint, &count, nullptr),
            "Failed to reflect input variable count of spv shader");
        std::vector<SpvReflectInterfaceVariable*> inputVars(count);
        Check(
            spvReflectEnumerateEntryPointInputVariables(&shaderModule, entryPoint, &count, inputVars.data()),
            "Failed to reflect input variables of spv shader");

        result.reserve(count);
        for(uint32_t i = 0; i < count; ++i)
        {
            auto &in = *inputVars[i];
            auto &out = result.emplace_back();
            auto [semanticName, semanticIndex] = ParseInputVarSemanticAndIndex(in.name ? in.name : "");
            out.semantic  = RHI::VertexSemantic(semanticName, semanticIndex);
            out.location  = static_cast<int>(in.location);
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
            default: throw Exception("Unsupported input variable format");
            }
        }
    }
    
} // namespace ShaderReflDetail

SPIRVReflection::SPIRVReflection(Span<unsigned char> code, std::string entryPoint)
    : entry_(std::move(entryPoint))
{
    auto shadermodule = new SpvReflectShaderModule{};
    RTRC_SCOPE_FAIL{ delete shadermodule; };
    ShaderReflDetail::Check(
        spvReflectCreateShaderModule(code.size(), code.GetData(), shadermodule),
        "Failed to reflect spv shader");
    shaderModule_.reset(shadermodule);

    for(uint32_t i = 0; i < shaderModule_->descriptor_binding_count; ++i)
    {
        const SpvReflectDescriptorBinding &binding = shaderModule_->descriptor_bindings[i];
        if(binding.accessed)
        {
            usedBindings_.insert(binding.name);
        }
    }
}

bool SPIRVReflection::IsBindingUsed(std::string_view name) const
{
    return usedBindings_.contains(name);
}

std::vector<ShaderIOVar> SPIRVReflection::GetInputVariables() const
{
    std::vector<ShaderIOVar> result;
    ShaderReflDetail::GetInputVariables(*shaderModule_, entry_.c_str(), result);
    return result;
}

Vector3i SPIRVReflection::GetComputeShaderThreadGroupSize() const
{
    const auto &localSize = shaderModule_->entry_points[0].local_size;
    Vector3i ret;
    ret.x = static_cast<int>(localSize.x);
    ret.y = static_cast<int>(localSize.y);
    ret.z = static_cast<int>(localSize.z);
    return ret;
}

std::vector<RHI::RawShaderEntry> SPIRVReflection::GetEntries() const
{
    std::vector<RHI::RawShaderEntry> ret;
    ret.reserve(shaderModule_->entry_point_count);
    for(uint32_t i = 0; i < shaderModule_->entry_point_count; ++i)
    {
        using enum RHI::ShaderStage;
        RHI::RawShaderEntry &entry = ret.emplace_back();
        switch(shaderModule_->entry_points[i].shader_stage)
        {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:           entry.stage = VertexShader;          break;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:         entry.stage = FragmentShader;        break;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:          entry.stage = ComputeShader;         break;
        case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:       entry.stage = RT_RayGenShader;       break;
        case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:      entry.stage = RT_AnyHitShader;       break;
        case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:  entry.stage = RT_ClosestHitShader;   break;
        case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:         entry.stage = RT_MissShader;         break;
        case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR: entry.stage = RT_IntersectionShader; break;
        case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:     entry.stage = CallableShader;        break;
        default:
            throw Exception("Unsupported shader entry type");
        }
        entry.name = shaderModule_->entry_points[i].name;
    }
    return ret;
}

void SPIRVReflection::DeleteShaderModule::operator()(SpvReflectShaderModule *ptr) const
{
    if(ptr)
    {
        spvReflectDestroyShaderModule(ptr);
        delete ptr;
    }
}

RTRC_END
