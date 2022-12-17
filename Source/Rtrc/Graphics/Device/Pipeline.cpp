#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Mesh/MeshLayout.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

size_t GraphicsPipeline::DescKey::Hash() const
{
    return Rtrc::Hash(
        shader,
        viewports,
        scissors,
        meshLayout,
        primitiveTopology,
        fillMode,
        cullMode,
        frontFaceMode,
        enableDepthBias,
        depthBiasConstFactor,
        depthBiasSlopeFactor,
        depthBiasClampValue,
        multisampleCount,
        enableDepthTest,
        enableDepthWrite,
        depthCompareOp,
        enableStencilTest,
        frontStencil,
        backStencil,
        enableBlending,
        blendingSrcColorFactor,
        blendingDstColorFactor,
        blendingColorOp,
        blendingSrcAlphaFactor,
        blendingDstAlphaFactor,
        blendingAlphaOp,
        colorAttachmentFormats,
        depthStencilFormat);
}

void GraphicsPipeline::Desc::Validate() const
{
#define VALIDATE_FAIL(...) [&]{ throw Exception("GraphicsPipeline::Desc::Validate: " __VA_ARGS__); }()

    if(!shader)
    {
        VALIDATE_FAIL("'shader' is not set");
    }

    if(!shader->GetRawShader(RHI::ShaderStage::VertexShader))
    {
        VALIDATE_FAIL("'shader' doesn't contain vertex shader");
    }

    if(!shader->GetRawShader(RHI::ShaderStage::FragmentShader))
    {
        VALIDATE_FAIL("'shader' doesn't contain fragment shader");
    }

    if(viewports.Is<std::monostate>())
    {
        VALIDATE_FAIL("'viewports' is not set");
    }

    if(scissors.Is<std::monostate>())
    {
        VALIDATE_FAIL("'scissors' is not set");
    }

    const bool useDepthStencil = enableDepthTest || enableDepthWrite || enableStencilTest;
    const bool hasDepthStencilFormat = depthStencilFormat != RHI::Format::Unknown;
    if(useDepthStencil != hasDepthStencilFormat)
    {
        VALIDATE_FAIL("depth stencil settings cannot match depthStencilFormat");
    }

#undef VALIDATE_FAIL
}

GraphicsPipeline::DescKey GraphicsPipeline::Desc::AsKey() const
{
    DescKey key;
    key.shader = shader->GetUniqueID();

#define RTRC_COPY_STATE(X) key.X = this->X

    RTRC_COPY_STATE(viewports);
    RTRC_COPY_STATE(scissors);
    RTRC_COPY_STATE(meshLayout);

    RTRC_COPY_STATE(primitiveTopology);
    RTRC_COPY_STATE(fillMode);
    RTRC_COPY_STATE(cullMode);
    RTRC_COPY_STATE(frontFaceMode);

    RTRC_COPY_STATE(enableDepthBias);
    RTRC_COPY_STATE(depthBiasConstFactor);
    RTRC_COPY_STATE(depthBiasSlopeFactor);
    RTRC_COPY_STATE(depthBiasClampValue);

    RTRC_COPY_STATE(multisampleCount);

    RTRC_COPY_STATE(enableDepthTest);
    RTRC_COPY_STATE(enableDepthWrite);
    RTRC_COPY_STATE(depthCompareOp);

    RTRC_COPY_STATE(enableStencilTest);
    RTRC_COPY_STATE(frontStencil);
    RTRC_COPY_STATE(backStencil);
    RTRC_COPY_STATE(enableBlending);
    RTRC_COPY_STATE(blendingSrcColorFactor);
    RTRC_COPY_STATE(blendingDstColorFactor);
    RTRC_COPY_STATE(blendingColorOp);
    RTRC_COPY_STATE(blendingSrcAlphaFactor);
    RTRC_COPY_STATE(blendingDstAlphaFactor);
    RTRC_COPY_STATE(blendingAlphaOp);

    RTRC_COPY_STATE(colorAttachmentFormats);
    RTRC_COPY_STATE(depthStencilFormat);

    return key;
}

PipelineManager::PipelineManager(RHI::DevicePtr device, DeviceSynchronizer &sync)
    : GeneralGPUObjectManager(sync), device_(std::move(device))
{
    
}

RC<GraphicsPipeline> PipelineManager::CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc)
{
#if RTRC_DEBUG
    desc.Validate();
#endif
    const auto key = desc.AsKey();
    return graphicsPipelineCache_.GetOrCreate(key, [&]
    {
        RHI::GraphicsPipelineDesc rhiDesc;

        rhiDesc.vertexShader   = desc.shader->GetRawShader(RHI::ShaderStage::VertexShader);
        rhiDesc.fragmentShader = desc.shader->GetRawShader(RHI::ShaderStage::FragmentShader);
        rhiDesc.bindingLayout  = desc.shader->GetBindingLayout()->GetRHIObject();
        rhiDesc.viewports      = desc.viewports;
        rhiDesc.scissors       = desc.scissors;

        auto reflectedShaderInput = desc.shader->GetInputVariables();
        std::vector<RHI::VertexInputBuffer> vertexBuffers;
        std::vector<RHI::VertexInputAttribute> vertexAttributes;
        if(desc.meshLayout)
        {
            // index in meshLayout -> index in vertexBuffers
            std::map<int, int> indexCache;

            for(auto &var : reflectedShaderInput)
            {
                if(var.isBuiltin)
                {
                    continue;
                }

                const int indexInMeshLayout = desc.meshLayout->GetVertexBufferIndexBySemantic(var.semantic);
                if(indexInMeshLayout < 0)
                {
                    throw Exception(fmt::format(
                        "Input variable with semantic '{}' is not provided by given mesh layout", var.semantic));
                }
                auto vertexBufferLayout = desc.meshLayout->GetVertexBufferLayouts()[indexInMeshLayout];

                int RHIVertexBufferIndex;
                if(auto it = indexCache.find(indexInMeshLayout); it == indexCache.end())
                {
                    RHIVertexBufferIndex = static_cast<int>(vertexBuffers.size());
                    vertexBuffers.push_back(RHI::VertexInputBuffer
                    {
                        .elementSize = static_cast<uint32_t>(vertexBufferLayout->GetElementStride()),
                        .perInstance = vertexBufferLayout->IsPerInstance()
                    });
                    indexCache.insert({ indexInMeshLayout, RHIVertexBufferIndex });
                }
                else
                {
                    RHIVertexBufferIndex = it->second;
                }

                auto attrib = vertexBufferLayout->GetAttributeBySemantic(var.semantic);
                assert(attrib);
                if(attrib->type != var.type)
                {
                    throw Exception(fmt::format(
                        "Type of semantic '{}' in mesh layout doesn't match shader input attribute", var.semantic));
                }

                vertexAttributes.push_back(RHI::VertexInputAttribute
                {
                    .location           = static_cast<uint32_t>(var.location),
                    .inputBufferIndex   = static_cast<uint32_t>(RHIVertexBufferIndex),
                    .byteOffsetInBuffer = static_cast<uint32_t>(attrib->byteOffsetInBuffer),
                    .type               = var.type
                });
            }
        }
        else
        {
            for(auto &var : reflectedShaderInput)
            {
                if(!var.isBuiltin)
                {
                    throw Exception("Mesh layout is not given while shader input variables are not empty");
                }
            }
        }

        rhiDesc.vertexBuffers = std::move(vertexBuffers);
        rhiDesc.vertexAttributs = std::move(vertexAttributes);

        rhiDesc.primitiveTopology = desc.primitiveTopology;
        rhiDesc.fillMode          = desc.fillMode;
        rhiDesc.cullMode          = desc.cullMode;
        rhiDesc.frontFaceMode     = desc.frontFaceMode;

        rhiDesc.enableDepthBias      = desc.enableDepthBias;
        rhiDesc.depthBiasConstFactor = desc.depthBiasConstFactor;
        rhiDesc.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;
        rhiDesc.depthBiasClampValue  = desc.depthBiasClampValue;

        rhiDesc.multisampleCount = desc.multisampleCount;

        rhiDesc.enableDepthTest  = desc.enableDepthTest;
        rhiDesc.enableDepthWrite = desc.enableDepthWrite;
        rhiDesc.depthCompareOp   = desc.depthCompareOp;

        rhiDesc.enableStencilTest = desc.enableStencilTest;
        rhiDesc.frontStencilOp    = desc.frontStencil;
        rhiDesc.backStencilOp     = desc.backStencil;

        rhiDesc.enableBlending         = desc.enableBlending;
        rhiDesc.blendingSrcColorFactor = desc.blendingSrcColorFactor;
        rhiDesc.blendingDstColorFactor = desc.blendingDstColorFactor;
        rhiDesc.blendingSrcAlphaFactor = desc.blendingSrcAlphaFactor;
        rhiDesc.blendingDstAlphaFactor = desc.blendingDstAlphaFactor;
        rhiDesc.blendingColorOp        = desc.blendingColorOp;
        rhiDesc.blendingAlphaOp        = desc.blendingAlphaOp;

        rhiDesc.colorAttachments   = desc.colorAttachmentFormats;
        rhiDesc.depthStencilFormat = desc.depthStencilFormat;

        auto ret = MakeRC<GraphicsPipeline>();
        ret->rhiObject_ = device_->CreateGraphicsPipeline(rhiDesc);
        ret->manager_ = this;
        ret->desc_ = desc;
        ret->desc_.shader = {}; // Don't store shader object
        ret->shaderInfo_ = desc.shader->GetInfo();
        return ret;
    });
}

RC<ComputePipeline> PipelineManager::CreateComputePipeline(const RC<Shader> &shader)
{
    return computePipelineCache_.GetOrCreate(shader->GetUniqueID(), [&]
    {
        RHI::ComputePipelineDesc rhiDesc;
        rhiDesc.computeShader = shader->GetRawShader(RHI::ShaderStage::ComputeShader);
        if(!rhiDesc.computeShader)
        {
            throw Exception("'shader' doesn't contain compute shader");
        }
        rhiDesc.bindingLayout = shader->GetBindingLayout()->GetRHIObject();

        auto ret = MakeRC<ComputePipeline>();
        ret->rhiObject_ = device_->CreateComputePipeline(rhiDesc);
        ret->manager_ = this;
        return ret;
    });
}

RTRC_END
