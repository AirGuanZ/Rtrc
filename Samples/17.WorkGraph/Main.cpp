#include <Rtrc/Rtrc.h>

#include "WorkGraph.shader.outh"

using namespace Rtrc;

// TODO: write a non-trivial sample

class WorkGraphDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        pipeline_ = device_->CreateWorkGraphPipeline(WorkGraphPipeline::Desc
        {
            .shaders = { device_->GetShader<RtrcShader::WorkGraph::Name>() },
            .entryNodes = { { "EntryNode", 0 } }
        });

        auto inputTexture = StatefulTexture::FromTexture(device_->LoadTexture2D(
            "./Asset/Sample/01.TexturedQuad/MainTexture.png",
            RHI::Format::R32G32B32A32_Float,
            RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::TransferSrc,
            false,
            RHI::TextureLayout::ShaderRWTexture));
        inputTexture->SetState(TexSubrscState(
            RHI::INITIAL_QUEUE_SESSION_ID, RHI::TextureLayout::ShaderRWTexture,
            RHI::PipelineStage::None, RHI::ResourceAccess::None));

        auto texture = graph->RegisterTexture(inputTexture);

        RC<Buffer> backingMemory;
        const auto &memoryRequirements = pipeline_->GetMemoryRequirements();
        if(memoryRequirements.minSize > 0)
        {
            backingMemory = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = memoryRequirements.minSize,
                .usage = RHI::BufferUsage::ShaderRWBuffer | RHI::BufferUsage::BackingMemory
            });
        }

        auto pass = graph->CreatePass("WorkGraphPass");
        pass->Use(texture, RG::CS_RWTexture);
        pass->SetCallback([&]
        {
            RtrcShader::WorkGraph::Pass passData;
            passData.Texture = texture;
            passData.resolution = texture->GetSize();
            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            auto &commandBuffer = RGGetCommandBuffer();
            commandBuffer.BindWorkGraphPipeline(
                pipeline_,
                backingMemory ? backingMemory->GetDeviceAddress() : RHI::BufferDeviceAddress{},
                memoryRequirements.minSize, true);
            commandBuffer.BindWorkGraphGroup(0, passGroup);

            struct InputRecord
            {
                uint32_t gridSizeX;
                uint32_t gridSizeY;
            };
            const InputRecord inputRecord =
            {
                UpAlignTo(texture->GetWidth(), 8u) / 8u,
                UpAlignTo(texture->GetHeight(), 8u) / 8u,
            };
            commandBuffer.DispatchNode(0, 1, sizeof(InputRecord), &inputRecord);
        });

        Image<Vector4f> readbackResult(texture->GetWidth(), texture->GetHeight());
        RGReadbackTexture(graph, "Readback", texture, 0, 0, readbackResult.GetData());

        ExecuteStandaloneRenderGraph(graph, false, true);

        if(!std::filesystem::exists("./Asset/Sample/17.WorkGraph"))
        {
            std::filesystem::create_directories("./Asset/Sample/17.WorkGraph");
        }
        readbackResult.Save("./Asset/Sample/17.WorkGraph/Output.png");

        SetExitFlag(true);
    }

    RC<WorkGraphPipeline> pipeline_;
};

RTRC_APPLICATION_MAIN(
    WorkGraphDemo,
    .title             = "Rtrc Sample: Work Graph",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
