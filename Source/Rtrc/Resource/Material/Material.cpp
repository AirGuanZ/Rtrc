#include <algorithm>
#include <ranges>

#include <Core/Enumerate.h>
#include <Core/Unreachable.h>
#include <Rtrc/Resource/Material/Material.h>
#include <Rtrc/Resource/Material/MaterialInstance.h>

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

    const ShaderUniformBlock *uniformBlock = nullptr;
    for(auto &b : shader.GetUniformBlocks())
    {
        if(b.name == "Material")
        {
            uniformBlock = &b;
            break;
        }
    }

    bindingGroupIndex_ = shader.GetBuiltinBindingGroupIndex(ShaderBindingLayoutInfo::BuiltinBindingGroup::Material);
    if(bindingGroupIndex_ >= 0)
    {
        bindingGroupLayout_ = shader.GetBindingGroupLayoutByIndex(bindingGroupIndex_);
    }

    if(uniformBlock)
    {
        if(uniformBlock->group != bindingGroupIndex_)
        {
            throw Exception("Constant buffer 'Material' must be defined in 'Material' binding group");
        }

        int dwordOffset = 0;
        for(auto &member : uniformBlock->variables)
        {
            if(member.type == ShaderUniformBlock::Unknown)
            {
                throw Exception("Only int/uint/float0123 properties are supported in 'Material' cbuffer");
            }

            const size_t memberSize = ShaderUniformBlock::GetTypeSize(member.type);
            assert(memberSize % 4 == 0);
            const int memberDWordCount = static_cast<int>(memberSize / sizeof(uint32_t));
            if(memberDWordCount >= 4 || (dwordOffset % 4) + memberDWordCount > 4)
            {
                dwordOffset = (dwordOffset + 3) / 4 * 4;
            }

            size_t offsetInValueBuffer;
            {
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
                auto Equal = [](MaterialProperty::Type a, ShaderUniformBlock::UniformType b)
                {
                    switch(b)
                    {
                    case ShaderUniformBlock::Float:  return a == MaterialProperty::Type::Float;
                    case ShaderUniformBlock::Float2: return a == MaterialProperty::Type::Float2;
                    case ShaderUniformBlock::Float3: return a == MaterialProperty::Type::Float3;
                    case ShaderUniformBlock::Float4: return a == MaterialProperty::Type::Float4;
                    case ShaderUniformBlock::UInt:  return a == MaterialProperty::Type::UInt;
                    case ShaderUniformBlock::UInt2: return a == MaterialProperty::Type::UInt2;
                    case ShaderUniformBlock::UInt3: return a == MaterialProperty::Type::UInt3;
                    case ShaderUniformBlock::UInt4: return a == MaterialProperty::Type::UInt4;
                    case ShaderUniformBlock::Int:  return a == MaterialProperty::Type::Int;
                    case ShaderUniformBlock::Int2: return a == MaterialProperty::Type::Int2;
                    case ShaderUniformBlock::Int3: return a == MaterialProperty::Type::Int3;
                    case ShaderUniformBlock::Int4: return a == MaterialProperty::Type::Int4;
                    default:
                        Unreachable();
                    }
                };
                if(!Equal(prop.type, member.type))
                {
                    throw Exception(fmt::format(
                        "Type of value property {} doesn't match its declaration in material", member.name));
                }
                offsetInValueBuffer = materialPropertyLayout.GetValueOffset(propIndex);
            }

            ValueReference ref;
            ref.offsetInValueBuffer    = offsetInValueBuffer;
            ref.offsetInConstantBuffer = dwordOffset * 4;
            ref.sizeInConstantBuffer   = memberDWordCount * 4;
            valueReferences_.push_back(ref);

            dwordOffset += memberDWordCount;
        }

        constantBufferSize_ = dwordOffset * 4;
        constantBufferIndexInBindingGroup_ = uniformBlock->indexInGroup;
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

RC<MaterialInstance> Material::CreateInstance() const
{
    return MakeRC<MaterialInstance>(shared_from_this(), device_);
}

RTRC_END
