#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Object/Pipeline.h>
#include <Rtrc/Graphics/Mesh/MeshLayout.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

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

PipelineManager::PipelineManager(GeneralGPUObjectManager &objectManager)
    : objectManager_(objectManager)
{
    
}

RC<GraphicsPipeline> PipelineManager::CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc)
{
#if RTRC_DEBUG
    desc.Validate();
#endif

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
            auto &vertexBufferLayout = desc.meshLayout->GetVertexBufferLayouts()[indexInMeshLayout];

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

    rhiDesc.primitiveTopology = desc.primitiveTopology;
    rhiDesc.fillMode          = desc.fillMode;
    rhiDesc.cullMode          = desc.cullMode;
    rhiDesc.frontFaceMode     = desc.frontFaceMode;

    rhiDesc.enableDepthBias      = desc.enableDepthBias;
    rhiDesc.depthBiasConstFactor = desc.depthBiasConstFactor;
    rhiDesc.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;
    rhiDesc.depthBiasClampValue  = desc.depthBiasClampValue;

    rhiDesc.multisampleCount = desc.multisampleCount;

    rhiDesc.enableDepthTest = desc.enableDepthTest;
    rhiDesc.frontStencilOp  = desc.frontStencil;
    rhiDesc.backStencilOp   = desc.backStencil;

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
    ret->rhiObject_ = objectManager_.GetDevice()->CreateGraphicsPipeline(rhiDesc);
    ret->manager_ = &objectManager_;
    ret->desc_ = desc;
    return ret;
}

RTRC_END
