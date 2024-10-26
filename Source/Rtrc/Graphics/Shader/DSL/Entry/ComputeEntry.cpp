#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ComputeEntry.h>

RTRC_EDSL_BEGIN

namespace ComputeEntryDetail
{

    void BuildComputeEntry(
        Ref<Device>                            device,
        std::string                           &resourceDefinitions,
        const Vector3u                        &threadGroupSize,
        RecordContext                         &recordContext,
        BindingGroupLayout::Desc              &defaultBindingGroupLayoutDesc,
        std::vector<ShaderUniformType>        &defaultUniformTypes,
        std::vector<BindingGroupLayout::Desc> &bindingGroupLayoutDescs,
        RC<Shader>                            &outShader,
        int                                   &outDefaultBindingGroupIndex,
        std::vector<ValueItem>                &outDefaultBindingGroupValueItems)
    {
        // Default binding group

        std::vector<ValueItem> defaultBindingGroupValueItems;
        size_t defaultUniformBufferSize = 0;
        {
            defaultBindingGroupValueItems.reserve(defaultUniformTypes.size());
            for(ShaderUniformType type : defaultUniformTypes)
            {
                auto &variable = defaultBindingGroupValueItems.emplace_back();
                
                variable.size = GetShaderUniformSize(type);
                assert(variable.size % 4 == 0);

                const size_t lineOffset = defaultUniformBufferSize % 16;
                if(lineOffset && lineOffset + variable.size > 16)
                {
                    defaultUniformBufferSize = (defaultUniformBufferSize + 15) / 16 * 16;
                }
                variable.offset = defaultUniformBufferSize;
                defaultUniformBufferSize += variable.size;
            }
            defaultUniformBufferSize = (defaultUniformBufferSize + 15) / 16 * 16;
        }

        if(defaultUniformBufferSize)
        {
            auto &binding = defaultBindingGroupLayoutDesc.bindings.emplace_back();
            binding.type = RHI::BindingType::ConstantBuffer;
        }

        int defaultBindingGroupIndex = -1;
        if(!defaultBindingGroupLayoutDesc.bindings.empty())
        {
            defaultBindingGroupIndex = static_cast<int>(bindingGroupLayoutDescs.size());
            bindingGroupLayoutDescs.push_back(defaultBindingGroupLayoutDesc);
        }

        // Inline samplers

        int staticSamplerBindingGroupIndex = -1;
        if(auto &staticSamplerMap = recordContext.GetAllStaticSamplers(); !staticSamplerMap.empty())
        {
            staticSamplerBindingGroupIndex = static_cast<int>(bindingGroupLayoutDescs.size());
            auto &groupLayoutDesc = bindingGroupLayoutDescs.emplace_back();
            auto &binding = groupLayoutDesc.bindings.emplace_back();
            binding.type = RHI::BindingType::Sampler;
            binding.arraySize = static_cast<uint32_t>(staticSamplerMap.size());

            auto &samplers = binding.immutableSamplers;
            samplers.resize(staticSamplerMap.size());
            for(auto &[desc, index] : staticSamplerMap)
            {
                samplers[index] = device->CreateSampler(desc);
            }

            resourceDefinitions += std::format(
                "SamplerState _rtrcStaticSamplers[{}];\n", recordContext.GetAllStaticSamplers().size());
        }

        // Generate type definitions

        const std::string typeDefinitions = recordContext.BuildTypeDefinitions();

        // Generate final source

        std::string source;
        source += "#define RTRC_DEFINE_SV_GroupID(NAME)          uint3 NAME : SV_GroupID\n";
        source += "#define RTRC_DEFINE_SV_GroupThreadID(NAME)    uint3 NAME : SV_GroupThreadID\n";
        source += "#define RTRC_DEFINE_SV_DispatchThreadID(NAME) uint3 NAME : SV_DispatchThreadID\n";

        source += typeDefinitions;
        source += resourceDefinitions;

        source += std::format("[numthreads({}, {}, {})]\n", threadGroupSize.x, threadGroupSize.y, threadGroupSize.z);

        std::string systemValueDefinitions;
        for(int i = 0; i < std::to_underlying(RecordContext::BuiltinValue::Count); ++i)
        {
            const auto e = static_cast<RecordContext::BuiltinValue>(i);
            if(recordContext.GetBuiltinValueRead(e))
            {
                if(!systemValueDefinitions.empty())
                {
                    systemValueDefinitions += ", ";
                }
                systemValueDefinitions += std::format(
                    "RTRC_DEFINE_{}(_rtrc{})",
                    RecordContext::GetBuiltinValueName(e),
                    RecordContext::GetBuiltinValueName(e));
            }
        }
        source += std::format("void CSMain({})\n", systemValueDefinitions);

        source += "{\n";
        source += recordContext.BuildRootScope("    ");
        source += "}\n";

        LogInfo(source);
        
        DXC::ShaderInfo dxcShaderInfo;
        dxcShaderInfo.source         = std::move(source);
        dxcShaderInfo.sourceFilename = "anonymous.hlsl";
        dxcShaderInfo.entryPoint     = "CSMain";
        const DXC::Target dxcTarget = device->GetBackendType() == RHI::BackendType::DirectX12 ?
            DXC::Target::DirectX12_CS_6_8 : DXC::Target::Vulkan_1_3_CS_6_8;
        const auto csData = DXC().Compile(dxcShaderInfo, dxcTarget, RTRC_DEBUG, nullptr, nullptr, nullptr);
        auto computeShader = device->GetRawDevice()->CreateShader(
            csData.data(), csData.size(),
            { RHI::RawShaderEntry{.stage = RHI::ShaderStage::ComputeShader, .name = "CSMain" } });

        std::vector<RC<BindingGroupLayout>> bindingGroupLayouts;
        bindingGroupLayouts.reserve(bindingGroupLayoutDescs.size());
        for(auto& desc : bindingGroupLayoutDescs)
        {
            bindingGroupLayouts.push_back(device->CreateBindingGroupLayout(desc));
        }

        BindingLayout::Desc bindingLayoutDesc;
        bindingLayoutDesc.groupLayouts = bindingGroupLayouts;
        auto bindingLayout = device->CreateBindingLayout(bindingLayoutDesc);

        ShaderBuilder::Desc desc;
        desc.category                     = ShaderCategory::Compute;
        desc.computeShader                = std::move(computeShader);
        desc.bindingGroupLayouts          = std::move(bindingGroupLayouts);
        desc.bindingLayout                = std::move(bindingLayout);
        desc.computeShaderThreadGroupSize = threadGroupSize;
        if(staticSamplerBindingGroupIndex >= 0)
        {
            auto bindingGroup = desc.bindingGroupLayouts[staticSamplerBindingGroupIndex]->CreateBindingGroup();;
            desc.bindingGroupForInlineSamplers = std::move(bindingGroup);
        }

        // Commit

        outShader                        = ShaderBuilder::BuildShader(device, std::move(desc));
        outDefaultBindingGroupIndex      = defaultBindingGroupIndex;
        outDefaultBindingGroupValueItems = std::move(defaultBindingGroupValueItems);
    }

} // namespace ComputeEntryDetail

RTRC_EDSL_END
