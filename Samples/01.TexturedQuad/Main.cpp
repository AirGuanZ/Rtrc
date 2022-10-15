#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

void Run()
{
    // render context

    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: TexturedQuad")
        .Create();

    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc
    {
        .extensions = Window::GetRequiredVulkanInstanceExtensions(),
        .debugMode = RTRC_DEBUG
    });

    auto device = instance->CreateDevice();
    RenderContext renderContext(device, &window);
    auto &copyContext = renderContext.GetCopyContext();
    auto graphicsQueue = renderContext.GetMainQueue();

    // pipeline

    MaterialManager materialManager;
    materialManager.SetRenderContext(&renderContext);
    materialManager.SetRootDirectory("Asset/01.TexturedQuad/");

    KeywordValueContext keywords;
    keywords.Set(RTRC_KEYWORD(DADADA), 1);

    auto material = materialManager.GetMaterial("Quad");
    auto subMaterial = material->GetSubMaterialByTag("Default");
    auto shader = subMaterial->GetShader(keywords);

    const int bindingGroupIndex = shader->GetBindingGroupIndexByName("TestGroup");
    auto bindingGroupLayout = shader->GetBindingGroupLayoutByIndex(bindingGroupIndex);
    auto bindingLayout = shader->GetBindingLayout();

    auto pipeline = renderContext.CreateGraphicsPipeline({
        .shader = shader,
        .colorAttachmentFormats = { renderContext.GetRenderTargetDesc().format }
    });

    // uploader

    ResourceUploader uploader(device);
    std::vector<RHI::TextureAcquireBarrier> uploadTextureAcquireBarriers;

    // vertex position buffer

    const std::array vertexPositionData =
    {
        Vector2f(-0.8f, -0.8f),
        Vector2f(-0.8f, +0.8f),
        Vector2f(+0.8f, +0.8f),
        Vector2f(-0.8f, -0.8f),
        Vector2f(+0.8f, +0.8f),
        Vector2f(+0.8f, -0.8f)
    };
    
    auto vertexPositionBuffer = copyContext.CreateBuffer(
        sizeof(vertexPositionData),
        RHI::BufferUsage::ShaderBuffer,
        RHI::BufferHostAccessType::None,
        vertexPositionData.data());

    auto vertexPositionBufferSRV = vertexPositionBuffer->GetSRV(RHI::BufferSRVDesc
    {
        .format = RHI::Format::R32G32_Float,
        .offset = 0,
        .range = sizeof(vertexPositionData)
    });

    // vertex texcoord buffer

    const std::array vertexTexCoordData = {
        Vector2f(0.0f, 1.0f),
        Vector2f(0.0f, 0.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(0.0f, 1.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(1.0f, 1.0f)
    };
    
    auto vertexTexCoordBuffer = copyContext.CreateBuffer(
        sizeof(vertexTexCoordData),
        RHI::BufferUsage::ShaderBuffer,
        RHI::BufferHostAccessType::None,
        vertexTexCoordData.data());

    auto vertexTexCoordBufferSRV = vertexTexCoordBuffer->GetSRV(RHI::BufferSRVDesc
    {
        .format = RHI::Format::R32G32_Float,
        .offset = 0,
        .range  = sizeof(vertexTexCoordData)
    });

    // main texture

    auto mainTexData = ImageDynamic::Load("Asset/01.TexturedQuad/MainTexture.png");
    auto mainTex = renderContext.CreateTexture2D(
        mainTexData.GetWidth(), mainTexData.GetHeight(), 1, 1,
        RHI::Format::B8G8R8A8_UNorm, RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferDst,
        RHI::QueueConcurrentAccessMode::Concurrent, 1, false);

    mainTex->GetRHIObject()->SetName("MainTexture");

    auto mainTexSRV = mainTex->GetSRV({ .isArray = true });

    uploadTextureAcquireBarriers.push_back(uploader.Upload(
        mainTex->GetRHIObject(), 0, 0, mainTexData,
        graphicsQueue,
        RHI::PipelineStage::FragmentShader,
        RHI::ResourceAccess::TextureRead,
        RHI::TextureLayout::ShaderTexture).GetAcquireBarrier());

    // main sampler

    auto mainSampler = renderContext.CreateSampler(RHI::SamplerDesc
    {
        .magFilter = RHI::FilterMode::Linear,
        .minFilter = RHI::FilterMode::Linear,
        .mipFilter = RHI::FilterMode::Point,
        .addressModeU = RHI::AddressMode::Clamp,
        .addressModeV = RHI::AddressMode::Clamp,
        .addressModeW = RHI::AddressMode::Clamp
    });

    // upload

    {
        uploader.SubmitAndSync();
        auto uploadCommandBuffer = StandaloneCommandBuffer::Create(device, graphicsQueue);
        uploadCommandBuffer->Begin();
        uploadCommandBuffer->ExecuteBarriers(uploadTextureAcquireBarriers);
        uploadTextureAcquireBarriers.clear();
        uploadCommandBuffer->End();
        uploadCommandBuffer.SubmitAndWait();
    }

    auto materialInstance = material->CreateInstance();
    materialInstance->Set("MainTexture", mainTexSRV);
    materialInstance->Set("MainSampler", mainSampler);
    materialInstance->Set("scale", 1.5f);

    auto subMaterialInstance = materialInstance->GetSubMaterialInstance(0);

    // binding group

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->Set("VertexPositionBuffer", vertexPositionBufferSRV);
    bindingGroup->Set("VertexTexCoordBuffer", vertexTexCoordBufferSRV);

    // render graph

    RG::Executer executer(renderContext);

    // render loop

    renderContext.BeginRenderLoop();
    while(!window.ShouldClose())
    {
        if(!renderContext.BeginFrame())
        {
            continue;
        }

        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        RG::RenderGraph graph;
        graph.SetQueue(graphicsQueue);
        
        auto renderTarget = graph.RegisterSwapchainTexture(renderContext.GetSwapchain());

        auto quadPass = graph.CreatePass("DrawQuad");
        quadPass->Use(renderTarget, RG::RENDER_TARGET);
        quadPass->SetCallback([&](RG::PassContext &context)
        {
            auto rt = renderTarget->Get();
            auto &commandBuffer = context.GetCommandBuffer();
            commandBuffer.BeginRenderPass(ColorAttachment
            {
                .renderTargetView = rt->GetRTV().rhiRTV,
                .loadOp           = AttachmentLoadOp::Clear,
                .storeOp          = AttachmentStoreOp::Store,
                .clearValue       = ColorClearValue{ 0, 1, 1, 1 }
            });
            commandBuffer.BindPipeline(pipeline);
            commandBuffer.BindGraphicsGroup(bindingGroupIndex, bindingGroup);
            commandBuffer.BindGraphicsSubMaterial(subMaterialInstance, keywords);
            commandBuffer.SetViewports(rt->GetViewport());
            commandBuffer.SetScissors(rt->GetScissor());
            commandBuffer.Draw(6, 1, 0, 0);
            commandBuffer.EndRenderPass();
        });
        quadPass->SetSignalFence(renderContext.GetFrameFence());

        executer.Execute(graph);

        renderContext.Present();
    }
    renderContext.EndRenderLoop();
    renderContext.WaitIdle();
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
