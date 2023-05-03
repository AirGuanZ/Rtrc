#include <algorithm>
#include <ranges>

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_BEGIN

MaterialPropertyHostLayout::MaterialPropertyHostLayout(std::vector<MaterialProperty> properties)
    : sortedProperties_(std::move(properties))
{
    std::ranges::partition(sortedProperties_, [&](const MaterialProperty &a){ return a.IsValue(); });

    valueBufferSize_ = 0;
    for(auto &p : sortedProperties_)
    {
        if(!p.IsValue())
        {
            break;
        }
        valueIndexToOffset_.push_back(valueBufferSize_);
        valueBufferSize_ += p.GetValueSize();
    }

    for(auto &&[index, prop] : Enumerate(sortedProperties_))
    {
        nameToIndex_.insert({ prop.name, static_cast<int>(index) });
    }
}

MaterialPassPropertyLayout::MaterialPassPropertyLayout(const MaterialPropertyHostLayout &materialPropertyLayout, const Shader &shader)
    : constantBufferSize_(0), constantBufferIndexInBindingGroup_(-1), bindingGroupIndex_(-1)
{
    // Constant buffer

    const ShaderConstantBuffer *cbuffer = nullptr;
    for(auto &cb : shader.GetConstantBuffers())
    {
        if(cb.name == "Material")
        {
            cbuffer = &cb;
            break;
        }
    }

    bindingGroupIndex_ = shader.GetBuiltinBindingGroupIndex(ShaderBindingLayoutInfo::BuiltinBindingGroup::Material);
    if(bindingGroupIndex_ >= 0)
    {
        bindingGroupLayout_ = shader.GetBindingGroupLayoutByIndex(bindingGroupIndex_);
    }

    if(cbuffer)
    {
        if(cbuffer->group != bindingGroupIndex_)
        {
            throw Exception("Constant buffer 'Material' must be defined in 'Material' binding group");
        }

        int dwordOffset = 0;
        for(auto &member : cbuffer->typeInfo->members)
        {
            if((member.elementType != ShaderStruct::Member::ElementType::Int &&
                member.elementType != ShaderStruct::Member::ElementType::Float) ||
               (member.matrixType != ShaderStruct::Member::MatrixType::Scalar &&
                member.matrixType != ShaderStruct::Member::MatrixType::Vector))
            {
                throw Exception("Only int/uint/float0123 properties are supported in 'Material' cbuffer");
            }

            const int memberDWordCount = member.matrixType == ShaderStruct::Member::MatrixType::Scalar ? 1 : member.rowSize;
            if(memberDWordCount >= 4 || (dwordOffset % 4) + memberDWordCount > 4)
            {
                dwordOffset = (dwordOffset + 3) / 4 * 4;
            }

            size_t offsetInValueBuffer;
            {
                using enum ShaderStruct::Member::ElementType;
                using enum ShaderStruct::Member::MatrixType;

                using Tuple = std::tuple<ShaderStruct::Member::ElementType, ShaderStruct::Member::MatrixType, int>;
                static constexpr Tuple typeToMemberInfo[] =
                {
                    { Float, Scalar, 0 }, { Float, Vector, 2 }, { Float, Vector, 3 }, { Float, Vector, 4 },
                    { Int,   Scalar, 0 }, { Int,   Vector, 2 }, { Int,   Vector, 3 }, { Int,   Vector, 4 },
                    { Int,   Scalar, 0 }, { Int,   Vector, 2 }, { Int,   Vector, 3 }, { Int,   Vector, 4 },
                };

                const int propIndex = materialPropertyLayout.GetPropertyIndexByName(member.pooledName);
                if(propIndex == -1)
                {
                    throw Exception(fmt::format("Value property {} is not found in 'Material' cbuffer", member.name));
                }
                const MaterialProperty &prop = materialPropertyLayout.GetProperties()[propIndex];
                if(!prop.IsValue())
                {
                    throw Exception(fmt::format("Property {} is not declared as value type in material", member.name));
                }

                const auto requiredMemberInfo = typeToMemberInfo[static_cast<int>(prop.type)];
                if(member.elementType != std::get<0>(requiredMemberInfo) ||
                   member.matrixType  != std::get<1>(requiredMemberInfo) ||
                   member.rowSize     != std::get<2>(requiredMemberInfo))
                {
                    throw Exception(fmt::format(
                        "Type of value property {} doesn't match its declaration in material", member.name));
                }

                offsetInValueBuffer = materialPropertyLayout.GetValueOffset(propIndex);
            }

            ValueReference ref;
            ref.offsetInValueBuffer = offsetInValueBuffer;
            ref.offsetInConstantBuffer = dwordOffset * 4;
            ref.sizeInConstantBuffer = memberDWordCount * 4;
            valueReferences_.push_back(ref);

            dwordOffset += memberDWordCount;
        }

        constantBufferSize_ = dwordOffset * 4;
        constantBufferIndexInBindingGroup_ = cbuffer->indexInGroup;
    }

    // TODO optimize: merge neighboring values

    // Resources

    if(bindingGroupLayout_)
    {
        auto &desc = bindingGroupLayout_->GetRHIObject()->GetDesc();
        for(auto &&[bindingIndex, binding] : Enumerate(desc.bindings))
        {
            const GeneralPooledString &bindingName =
                shader.GetBindingNameMap().GetPooledBindingName(bindingGroupIndex_, bindingIndex);
            
            if(binding.type == RHI::BindingType::ConstantBuffer)
            {
#if RTRC_DEBUG
                if(bindingName != RTRC_GENERAL_POOLED_STRING(Material))
                {
                    throw Exception("Constant buffer in 'Material' binding group must have name 'Material'");
                }
#endif
                continue;
            }

            MaterialProperty::Type propType;
            switch(binding.type)
            {
            case RHI::BindingType::Texture:
                propType = MaterialProperty::Type::Texture;
                break;
            case RHI::BindingType::Buffer:
            case RHI::BindingType::StructuredBuffer:
                propType = MaterialProperty::Type::Buffer;
                break;
            case RHI::BindingType::Sampler:
                propType = MaterialProperty::Type::Sampler;
                break;
            case RHI::BindingType::AccelerationStructure:
                propType = MaterialProperty::Type::AccelerationStructure;
                break;
            default:
                throw Exception(fmt::format(
                    "Unsupported binding type in 'Material' group: {} {}",
                    GetBindingTypeName(binding.type), bindingName));
            }

            const int indexInProperties = materialPropertyLayout.GetPropertyIndexByName(bindingName)
                                        - materialPropertyLayout.GetValuePropertyCount();
            if(indexInProperties < 0)
            {
                throw Exception(fmt::format(
                    "Binding '{} {}' in 'Material' binding group is not declared in material",
                    GetBindingTypeName(binding.type), bindingName));
            }

            ResourceReference ref;
            ref.type = propType;
            ref.indexInProperties = indexInProperties;
            ref.indexInBindingGroup = bindingIndex;
            resourceReferences_.push_back(ref);
        }
    }
}

void MaterialPassPropertyLayout::FillConstantBufferContent(const void *valueBuffer, void *outputData) const
{
    auto input = static_cast<const unsigned char *>(valueBuffer);
    auto output = static_cast<unsigned char *>(outputData);
    for(auto &ref : valueReferences_)
    {
        std::memcpy(output + ref.offsetInConstantBuffer, input + ref.offsetInValueBuffer, ref.sizeInConstantBuffer);
    }
}

void MaterialPassPropertyLayout::FillBindingGroup(
    BindingGroup          &bindingGroup,
    Span<MaterialResource> materialResources,
    RC<DynamicBuffer>      cbuffer) const
{
    // Constant buffer

    if(constantBufferIndexInBindingGroup_ >= 0)
    {
        bindingGroup.Set(constantBufferIndexInBindingGroup_, cbuffer);
    }

    // Other resources

    for(auto &ref : resourceReferences_)
    {
        auto &resource = materialResources[ref.indexInProperties];
        switch(ref.type)
        {
        case MaterialProperty::Type::Buffer:
            bindingGroup.Set(ref.indexInBindingGroup, resource.As<BufferSrv>());
            break;
        case MaterialProperty::Type::Texture:
            if(auto srv = resource.AsIf<TextureSrv>())
            {
                bindingGroup.Set(ref.indexInBindingGroup, *srv);
            }
            else
            {
                bindingGroup.Set(ref.indexInBindingGroup, resource.As<RC<Texture>>());
            }
            break;
        case MaterialProperty::Type::Sampler:
            bindingGroup.Set(ref.indexInBindingGroup, resource.As<RC<Sampler>>());
            break;
        case MaterialProperty::Type::AccelerationStructure:
            bindingGroup.Set(ref.indexInBindingGroup, resource.As<RC<Tlas>>());
            break;
        default:
            Unreachable();
        }
    }
}

const char *Material::BuiltinPassName[EnumCount<BuiltinPass>] =
{
    "GBuffer"
};

const MaterialPassTag Material::BuiltinPooledPassName[EnumCount<BuiltinPass>] =
{
    RTRC_MATERIAL_PASS_TAG(GBuffer)
};

RC<MaterialInstance> Material::CreateInstance() const
{
    return MakeRC<MaterialInstance>(shared_from_this(), device_);
}

RTRC_END
