#include <spirv_reflect.h>

#include <Rtrc/Graphics/Shader/ShaderReflection.h>
#include <Rtrc/Utility/ScopeGuard.h>
#include <Rtrc/Utility/String.h>

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

    std::string ParseInputVarName(const std::string &inputName)
    {
        if(StartsWith(inputName, "in.var."))
        {
            return inputName.substr(7);
        }
        return inputName;
    }

    void ReflectInputVariables(
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
            out.semantic = ParseInputVarName(in.name ? in.name : "");
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
            default: throw Exception("Unsupported input variable format");
            }
        }
    }

    Box<ShaderStruct> GetShaderStruct(std::string name, uint32_t memberCount, const SpvReflectTypeDescription *members)
    {
        auto ret = MakeBox<ShaderStruct>();
        ret->typeName = std::move(name);

        for(uint32_t i = 0; i < memberCount; ++i)
        {
            const SpvReflectTypeDescription *memberDesc = &members[i];
            auto &newMember = ret->members.emplace_back();

            newMember.name = memberDesc->struct_member_name;

            if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
            {
                assert(memberDesc->op == SpvOpTypeArray);
                for(uint32_t j = 0; j < memberDesc->traits.array.dims_count; ++j)
                {
                    newMember.arraySizes.PushBack(static_cast<int>(memberDesc->traits.array.dims[j]));
                }
            }

            if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
            {
                newMember.elementType = ShaderStruct::Member::ElementType::Float;
            }
            else if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
            {
                newMember.elementType = ShaderStruct::Member::ElementType::Int;
            }
            else if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
            {
                newMember.elementType = ShaderStruct::Member::ElementType::Struct;
                newMember.structInfo = GetShaderStruct(
                    memberDesc->type_name, memberDesc->member_count, memberDesc->members);
            }
            else
            {
                throw Exception("Unsupported basic element type in constant buffer struct");
            }

            if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
            {
                newMember.matrixType = ShaderStruct::Member::MatrixType::Vector;
                newMember.rowSize = static_cast<int>(memberDesc->traits.numeric.vector.component_count);
            }
            else if(memberDesc->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
            {
                newMember.matrixType = ShaderStruct::Member::MatrixType::Matrix;
                newMember.rowSize = static_cast<int>(memberDesc->traits.numeric.matrix.row_count);
                newMember.colSize = static_cast<int>(memberDesc->traits.numeric.matrix.column_count);
            }
        }

        return ret;
    }

    void ReflectConstantBuffers(const SpvReflectShaderModule &shaderModule, std::vector<ShaderConstantBuffer> &result)
    {
        for(uint32_t i = 0; i < shaderModule.descriptor_binding_count; ++i)
        {
            const SpvReflectDescriptorBinding &binding = shaderModule.descriptor_bindings[i];
            if(binding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                continue;
            }
            ShaderConstantBuffer cbuffer;
            cbuffer.name = binding.name;
            cbuffer.group = static_cast<int>(binding.set);
            cbuffer.indexInGroup = static_cast<int>(binding.binding);
            cbuffer.typeInfo = GetShaderStruct(
                binding.type_description->type_name,
                binding.type_description->member_count,
                binding.type_description->members);
            result.push_back(std::move(cbuffer));
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
    ShaderReflDetail::ReflectInputVariables(*shaderModule_, entry_.c_str(), result);
    return result;
}

std::vector<ShaderConstantBuffer> SPIRVReflection::GetConstantBuffers() const
{
    std::vector<ShaderConstantBuffer> result;
    ShaderReflDetail::ReflectConstantBuffers(*shaderModule_, result);
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

void SPIRVReflection::DeleteShaderModule::operator()(SpvReflectShaderModule *ptr) const
{
    if(ptr)
    {
        spvReflectDestroyShaderModule(ptr);
        delete ptr;
    }
}

RTRC_END
