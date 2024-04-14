#include "Mesh.h"
#include "SeamEnergy.h"
#include "VDMBake.h"

#include "FADM.shader.outh"

VDMBaker::VDMBaker(DeviceRef device)
    : device_(device)
{
    pipelineCache_.SetDevice(device);
}

RC<StatefulTexture> VDMBaker::BakeUniformVDM(GraphRef graph, const InputMesh& inputMesh, const Vector2u& res)
{
    auto device = graph->GetDevice();
    auto vdm = device->CreateStatefulTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32B32A32_Float,
        .width = res.x,
        .height = res.y,
        .usage = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferSrc,
        .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
    }, "VDM");

    auto rgVDM = graph->RegisterTexture(vdm);

    {
        using Shader = RtrcShader::FADM::BakeUniformVDM;
        RGAddRenderPass<true>(graph, Shader::Name, RGColorAttachment
        {
            .rtv = rgVDM->GetRtv(),
            .loadOp = RHI::AttachmentLoadOp::Clear,
            .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
        }, [rgVDM, &inputMesh, res, this]
        {
            Shader::Pass passData;
            passData.ParameterSpacePositionBuffer = inputMesh.parameterSpacePositionBuffer;
            passData.PositionBuffer = inputMesh.positionBuffer;
            passData.res = res.To<float>();
            passData.gridLower = inputMesh.gridLower;
            passData.gridUpper = inputMesh.gridUpper;
            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            auto pipeline = pipelineCache_.Get(
            {
                .shader = device_->GetShader<Shader::Name>(),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { rgVDM->GetFormat() }
                }
            });

            auto &commandBuffer = RGGetCommandBuffer();
            commandBuffer.BindGraphicsPipeline(pipeline);
            commandBuffer.BindGraphicsGroups(passGroup);
            commandBuffer.SetIndexBuffer(inputMesh.indexBuffer, RHI::IndexFormat::UInt32);
            commandBuffer.DrawIndexed(inputMesh.indices.size(), 1, 0, 0, 0);
        });
    }

    return vdm;
}

RC<StatefulTexture> VDMBaker::BakeVDMBySeamCarving(
    GraphRef         graph,
    const InputMesh &inputMesh,
    const Vector2u  &initialSampleResolution,
    const Vector2u  &res)
{
    RTRC_LOG_INFO_SCOPE("VDMBaker::BakeVDMBySeamCarving");

    if(res.x >= initialSampleResolution.x && res.y >= initialSampleResolution.y)
    {
        return BakeUniformVDM(graph, inputMesh, res);
    }

    auto device = graph->GetDevice();

    // Generate and read back high resolution vdm

    const unsigned initX = (std::max)(initialSampleResolution.x, res.x);
    const unsigned initY = (std::max)(initialSampleResolution.y, res.y);
    auto highResVDM = graph->RegisterTexture(BakeUniformVDM(
        graph, inputMesh, { initX, initY }));

    Image<Vector4f> highResVDMData(initX, initY);
    RGReadbackTexture(graph, "ReadbackHighResolutionVDM", highResVDM, 0, 0, highResVDMData.GetData());
    RGExecuter(device).ExecutePartially(graph);

    // Convert vdm to pos map

    const Rectf gridRect{ inputMesh.gridLower, inputMesh.gridUpper };
    auto positionMap = SeamEnergy::ComputePositionMap(highResVDMData, gridRect);

    // Resize pos map by seam carving

    while(positionMap.GetWidth() > res.x || positionMap.GetHeight() > res.y)
    {
        if(positionMap.GetWidth() > res.x)
        {
            auto energyMap = SeamEnergy::ComputeEnergyMapBasedOnInterpolationError(positionMap, true);
            auto dpTable = SeamEnergy::ComputeVerticalDPTable(energyMap);

            int entryX = 1; float seamEnergy = dpTable(1, 0);
            for(int x = 2; x < positionMap.GetSWidth() - 1; ++x)
            {
                const float e = dpTable(x, 0);
                if(e < seamEnergy)
                {
                    seamEnergy = e;
                    entryX = x;
                }
            }

            auto seam = SeamEnergy::GetVerticalSeam(dpTable, entryX);
            positionMap = SeamEnergy::RemoveVerticalSeam(positionMap, seam);
            assert(positionMap.GetWidth() == energyMap.GetWidth() - 1);
        }
        if(positionMap.GetHeight() > res.y)
        {
            auto energyMap = SeamEnergy::ComputeEnergyMapBasedOnInterpolationError(positionMap, false);
            auto dpTable = SeamEnergy::ComputeHorizontalDPTable(energyMap);

            int entryY = 1; float seamEnergy = dpTable(0, 1);
            for(int y = 2; y < positionMap.GetSHeight() - 1; ++y)
            {
                const float e = dpTable(0, y);
                if(e < seamEnergy)
                {
                    seamEnergy = e;
                    entryY = y;
                }
            }

            auto seam = SeamEnergy::GetHorizontalSeam(dpTable, entryY);
            positionMap = SeamEnergy::RemoveHorizontalSeam(positionMap, seam);
            assert(positionMap.GetHeight() == energyMap.GetHeight() - 1);
        }
    }

    // Convert pos map back to vdm and upload

    auto vdmData = SeamEnergy::ComputeDisplacementMap(positionMap, gridRect);

    auto vdm = device->CreateAndUploadTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32B32A32_Float,
        .width = vdmData.GetWidth(),
        .height = vdmData.GetHeight(),
        .usage = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource

    }, vdmData.GetData(), RHI::TextureLayout::ShaderTexture);
    return StatefulTexture::FromTexture(vdm);
}

RC<StatefulTexture> VDMBaker::BakeAlignedVDM(
    GraphRef                               graph,
    const InputMesh                       &inputMesh,
    const Image<GridAlignment::GridPoint> &grid,
    const RC<Texture>                     &gridUVTexture)
{
    // Create high resolution uniform vdm

    const Vector2u highRes = Min(16u * grid.GetSize(), Vector2u(8192, 8192));
    auto highResVDM = BakeUniformVDM(graph, inputMesh, highRes);

    // Resample to grid res

    auto ret = graph->GetDevice()->CreateStatefulTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32B32A32_Float,
        .width = grid.GetWidth(),
        .height = grid.GetHeight(),
        .usage = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderedAccess
    }, "AlignedVDM");

    using Shader = RtrcShader::FADM::BakeAlignedVDM;

    Shader::Pass passData;
    passData.HighResUniformVDM = graph->RegisterTexture(highResVDM);
    passData.GridUVTexture     = gridUVTexture;
    passData.VDM               = graph->RegisterTexture(ret);
    passData.uniformRes        = highRes.To<float>();
    passData.res               = ret->GetSize();
    passData.gridLower         = inputMesh.gridLower;
    passData.gridUpper         = inputMesh.gridUpper;
    RGDispatchWithThreadCount<Shader>(graph, ret->GetSize(), passData);

    return ret;
}
