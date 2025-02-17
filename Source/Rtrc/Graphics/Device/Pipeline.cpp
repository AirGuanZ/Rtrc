#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Shader/Shader.h>
#include <Rtrc/Graphics/Device/MeshLayout.h>

RTRC_BEGIN

namespace PipelineDetail
{

    bool IsCompatible(RHI::VertexAttributeType dataType, RHI::VertexAttributeType shaderInputType)
    {
        if(dataType == RHI::VertexAttributeType::UChar4Norm)
        {
            dataType = RHI::VertexAttributeType::Float4;
        }
        return dataType == shaderInputType;
    }

} // namespace PipelineDetail

void RayTracingPipeline::GetShaderGroupHandles(
    uint32_t               startGroupIndex,
    uint32_t               groupCount,
    MutSpan<unsigned char> outputData) const
{
    assert(rhiObject_);
    rhiObject_->GetShaderGroupHandles(startGroupIndex, groupCount, outputData);
}

uint32_t RayTracingPipeline::GetShaderGroupCount() const
{
    return shaderGroupCount_;
}

uint32_t RayTracingPipeline::GetShaderGroupHandleSize() const
{
    assert(manager_);
    auto &device = static_cast<PipelineManager *>(manager_)->_internalGetRHIDevice();
    return device->GetShaderGroupRecordRequirements().shaderGroupHandleSize;
}

PipelineManager::PipelineManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync)
    : GeneralGPUObjectManager(sync), device_(std::move(device))
{
    
}

RC<RayTracingLibrary> PipelineManager::CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc)
{
    const RHI::RayTracingLibraryDesc rhiDesc =
    {
        .rawShader              = desc.shader->GetRawShader(RHI::ShaderType::RayTracingShader),
        .shaderGroups           = desc.shaderGroups,
        .maxRayPayloadSize      = desc.maxRayPayloadSize,
        .maxRayHitAttributeSize = desc.maxRayHitAttributeSize,
        .maxRecursiveDepth      = desc.maxRecursiveDepth
    };
    auto ret = MakeRC<RayTracingLibrary>();
    ret->library_ = device_->CreateRayTracingLibrary(rhiDesc);
    ret->shaderGroupCount_ = static_cast<uint32_t>(desc.shaderGroups.size());
    ret->bindingLayoutInfo_ = desc.shader->GetInfo()->GetBindingLayoutInfo();
    return ret;
}

RC<RayTracingPipeline> PipelineManager::CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc)
{
    uint32_t shaderGroupCount = 0;
    shaderGroupCount += static_cast<uint32_t>(desc.shaderGroups.size());
    for(auto &library : desc.libraries)
    {
        shaderGroupCount += library->GetShaderGroupCount();
    }

    std::vector<RHI::RawShaderOPtr> rhiShaders;
    rhiShaders.reserve(desc.shaders.size());
    for(auto &rawShader : desc.shaders)
    {
        rhiShaders.push_back(rawShader->GetRawShader(RHI::ShaderType::RayTracingShader));
    }

    std::vector<RHI::RayTracingLibraryOPtr> rhiLibraries;
    rhiLibraries.reserve(desc.libraries.size());
    for(auto &library : desc.libraries)
    {
        rhiLibraries.push_back(library->library_);
    }

    const RHI::RayTracingPipelineDesc rhiDesc =
    {
        .rawShaders             = std::move(rhiShaders),
        .shaderGroups           = desc.shaderGroups,
        .libraries              = std::move(rhiLibraries),
        .bindingLayout          = desc.bindingLayout->GetRHIObject(),
        .maxRayPayloadSize      = desc.maxRayPayloadSize,
        .maxRayHitAttributeSize = desc.maxRayHitAttributeSize,
        .maxRecursiveDepth      = desc.maxRecursiveDepth,
        .useCustomStackSize     = desc.useCustomStackSize
    };
    auto rhiPipeline = device_->CreateRayTracingPipeline(rhiDesc);

    auto ret = MakeRC<RayTracingPipeline>();
    ret->rhiObject_ = std::move(rhiPipeline);
    ret->manager_ = this;
    ret->shaderGroupCount_ = shaderGroupCount;
    
    if(!desc.shaders.empty())
    {
        ret->bindingLayoutInfo_ = desc.shaders.front()->GetInfo()->GetBindingLayoutInfo();
    }
    else
    {
        assert(!desc.libraries.empty());
        ret->bindingLayoutInfo_ = desc.libraries.front()->bindingLayoutInfo_;
    }
    for(auto &shader : desc.shaders)
    {
        assert(ret->bindingLayoutInfo_->GetBindingLayout() == shader->GetBindingLayout());
    }
    for(auto &library : desc.libraries)
    {
        assert(ret->bindingLayoutInfo_->GetBindingLayout() == library->bindingLayoutInfo_->GetBindingLayout());
    }

    return ret;
}

RC<GraphicsPipeline> PipelineManager::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
{
    RHI::GraphicsPipelineDesc rhiDesc;

    if(desc.shader->GetCategory() == ShaderCategory::ClassicalGraphics)
    {
        rhiDesc.vertexShader = desc.shader->GetRawShader(RHI::ShaderType::VertexShader);
    }
    else
    {
        rhiDesc.taskShader = desc.shader->GetRawShader(RHI::ShaderType::TaskShader);
        rhiDesc.meshShader = desc.shader->GetRawShader(RHI::ShaderType::MeshShader);
    }
    rhiDesc.fragmentShader = desc.shader->GetRawShader(RHI::ShaderType::FragmentShader);
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
                throw Exception(std::format(
                    "Input variable with semantic '{}' is not provided by given mesh layout",
                    var.semantic.ToString()));
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
            if(!PipelineDetail::IsCompatible(attrib->type, var.type))
            {
                throw Exception(std::format(
                    "Type of semantic '{}' in mesh layout doesn't match shader input attribute",
                    var.semantic.ToString()));
            }
    
            vertexAttributes.push_back(RHI::VertexInputAttribute
            {
                .location           = static_cast<uint32_t>(var.location),
                .inputBufferIndex   = static_cast<uint32_t>(RHIVertexBufferIndex),
                .byteOffsetInBuffer = static_cast<uint32_t>(attrib->byteOffsetInBuffer),
                .type               = attrib->type,
                .semanticName       = attrib->semantic.GetName(),
                .semanticIndex      = attrib->semantic.GetIndex()
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

    auto &rasterizerDesc = desc.rasterizerState.GetDesc();
    rhiDesc.enableConservativeRasterization = rasterizerDesc.conservativeRasterization;
    rhiDesc.primitiveTopology               = rasterizerDesc.primitiveTopology;
    rhiDesc.fillMode                        = rasterizerDesc.fillMode;
    rhiDesc.cullMode                        = rasterizerDesc.cullMode;
    rhiDesc.frontFaceMode                   = rasterizerDesc.frontFaceMode;

    auto &depthStencilDesc = desc.depthStencilState.GetDesc();
    rhiDesc.enableDepthBias      = depthStencilDesc.enableDepthBias;
    rhiDesc.depthBiasConstFactor = desc.depthBiasConstFactor;
    rhiDesc.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;
    rhiDesc.depthBiasClampValue  = desc.depthBiasClampValue;

    rhiDesc.enableDepthClip = depthStencilDesc.enableDepthClip;
    
    rhiDesc.enableDepthTest  = depthStencilDesc.enableDepthTest;
    rhiDesc.enableDepthWrite = depthStencilDesc.enableDepthWrite;
    rhiDesc.depthCompareOp   = depthStencilDesc.depthCompareOp;
    
    rhiDesc.enableStencilTest = depthStencilDesc.enableStencilTest;
    rhiDesc.stencilReadMask   = depthStencilDesc.stencilReadMask;
    rhiDesc.stencilWriteMask  = depthStencilDesc.stencilWriteMask;
    rhiDesc.frontStencilOp    = depthStencilDesc.frontStencil;
    rhiDesc.backStencilOp     = depthStencilDesc.backStencil;

    auto &blendDesc = desc.blendState.GetDesc();
    rhiDesc.enableBlending         = blendDesc.enableBlending;
    rhiDesc.blendingSrcColorFactor = blendDesc.blendingSrcColorFactor;
    rhiDesc.blendingDstColorFactor = blendDesc.blendingDstColorFactor;
    rhiDesc.blendingSrcAlphaFactor = blendDesc.blendingSrcAlphaFactor;
    rhiDesc.blendingDstAlphaFactor = blendDesc.blendingDstAlphaFactor;
    rhiDesc.blendingColorOp        = blendDesc.blendingColorOp;
    rhiDesc.blendingAlphaOp        = blendDesc.blendingAlphaOp;

    auto &attachmentDesc = desc.attachmentState.GetDesc();
    rhiDesc.multisampleCount   = attachmentDesc.multisampleCount;
    rhiDesc.colorAttachments   = attachmentDesc.colorAttachmentFormats;
    rhiDesc.depthStencilFormat = attachmentDesc.depthStencilFormat;
    
    auto ret = MakeRC<GraphicsPipeline>();
    ret->rhiObject_  = device_->CreateGraphicsPipeline(rhiDesc);
    ret->manager_    = this;
    ret->shaderInfo_ = desc.shader->GetInfo();
    return ret;
}

RC<ComputePipeline> PipelineManager::CreateComputePipeline(const RC<Shader> &shader)
{
    RHI::ComputePipelineDesc rhiDesc;
    rhiDesc.computeShader = shader->GetRawShader(RHI::ShaderType::ComputeShader);
    if(!rhiDesc.computeShader)
    {
        throw Exception("'shader' doesn't contain compute shader");
    }
    rhiDesc.bindingLayout = shader->GetBindingLayout()->GetRHIObject();

    auto ret = MakeRC<ComputePipeline>();
    ret->rhiObject_  = device_->CreateComputePipeline(rhiDesc);
    ret->manager_    = this;
    ret->shaderInfo_ = shader->GetInfo();
    return ret;
}

RC<WorkGraphPipeline> PipelineManager::CreateWorkGraphPipeline(const WorkGraphPipeline::Desc &desc)
{
    assert(!desc.shaders.empty());
    for(auto &shader : desc.shaders)
    {
        assert(desc.shaders[0]->GetBindingLayout() == shader->GetBindingLayout());
    }

    RHI::WorkGraphPipelineDesc rhiDesc;
    rhiDesc.rawShaders.reserve(desc.shaders.size());
    for(auto &rawShader : desc.shaders)
    {
        rhiDesc.rawShaders.push_back(rawShader->GetRawShader(RHI::ShaderType::WorkGraphShader));
    }
    rhiDesc.bindingLayout = desc.shaders[0]->GetBindingLayout()->GetRHIObject();
    rhiDesc.entryPoints = desc.entryNodes;
    rhiDesc.graphics = desc.graphics;

    auto ret = MakeRC<WorkGraphPipeline>();
    ret->rhiObject_         = device_->CreateWorkGraphPipeline(rhiDesc);
    ret->manager_           = this;
    ret->bindingLayoutInfo_ = desc.shaders.front()->GetInfo()->GetBindingLayoutInfo();
    return ret;
}

const RHI::DeviceOPtr &PipelineManager::_internalGetRHIDevice() const
{
    return device_;
}

RTRC_END
