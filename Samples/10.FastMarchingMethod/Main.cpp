#include <Rtrc/Rtrc.h>

#include "Visualize.shader.outh"

using namespace Rtrc;

// Assumes a constant speed of 1
void FMM2D(const std::vector<Vector2i> &sources, Image<float> &output)
{
    // Initialize initial fixed points

    Image<bool> isFixed(output.GetSize());
    std::fill(isFixed.begin(), isFixed.end(), false);

    std::fill(output.begin(), output.end(), FLT_MAX);
    for(auto &s : sources)
    {
        output(s.x, s.y) = 0;
        isFixed(s.x, s.y) = true;
    }

    // Initialize initial active points

    auto Compare = [&](const Vector2i &lhs, const Vector2i &rhs)
    {
        const float distLhs = output(lhs.x, lhs.y);
        const float distRhs = output(rhs.x, rhs.y);
        return distLhs != distRhs ? distLhs < distRhs : lhs < rhs;
    };
    std::set<Vector2i, decltype(Compare)> A(Compare);

    auto ForEachNeighbors = [&]<typename F>(const Vector2i &p, const F &f)
    {
        if(p.x > 0) { f(Vector2i(p.x - 1, p.y)); }
        if(p.y > 0) { f(Vector2i(p.x, p.y - 1)); }
        if(p.x + 1 < output.GetSWidth()) { f(Vector2i(p.x + 1, p.y)); }
        if(p.y + 1 < output.GetSHeight()) { f(Vector2i(p.x, p.y + 1)); }
    };

    for(auto &s : sources)
    {
        ForEachNeighbors(s, [&](const Vector2i &p)
        {
            if(output(p.x, p.y) == FLT_MAX)
            {
                output(p.x, p.y) = 1;
                A.insert(p);
            }
        });
    }

    // Main loop

    auto UpdateT = [&](const Vector2i &p)
    {
        float nx = FLT_MAX;
        if(p.x > 0) { nx = (std::min)(nx, output(p.x - 1, p.y)); }
        if(p.x + 1 < output.GetSWidth()) { nx = (std::min)(nx, output(p.x + 1, p.y)); }
        float ny = FLT_MAX;
        if(p.y > 0) { ny = (std::min)(ny, output(p.x, p.y - 1)); }
        if(p.y + 1 < output.GetSHeight()) { ny = (std::min)(ny, output(p.x, p.y + 1)); }
        assert(nx != FLT_MAX || ny != FLT_MAX);

        if(nx != FLT_MAX)
        {
            if(nx + 1 <= ny)
            {
                return nx + 1;
            }
        }
        assert(!(nx != FLT_MAX && ny == FLT_MAX));

        if(ny != FLT_MAX)
        {
            if(ny + 1 <= nx)
            {
                return ny + 1;
            }
        }
        assert(!(nx == FLT_MAX && ny != FLT_MAX));
        assert(nx != FLT_MAX && ny != FLT_MAX);

        // (t-x)^2 + (t-y)^2 = 1
        // => 2t^2 + (-2x-2y)t + (x^2+y^2-1) = 0
        const float a = 2, b = -2 * nx - 2 * ny, c = nx * nx + ny * ny - 1;
        // Since |nx - ny| <= 1, b^2-4ac must be positive.
        const float sqrtDelta = std::sqrt(std::max(b * b - 4 * a * c, 0.0f));
        const float t = (-b + sqrtDelta) / (2 * a);
        return t;
    };

    while(!A.empty())
    {
        const Vector2i p = *A.begin();
        A.erase(A.begin());
        isFixed(p.x, p.y) = true;

        ForEachNeighbors(p, [&](const Vector2i &n)
        {
            if(isFixed(n.x, n.y))
            {
                return;
            }
            const float oldT = output(n.x, n.y);
            const float newT = UpdateT(n);
            if(newT < oldT)
            {
                if(oldT != FLT_MAX)
                {
                    A.erase(n);
                }
                output(n.x, n.y) = newT;
                A.insert(n);
            }
        });
    }
}

class FMMDemo : public SimpleApplication
{
    RC<Texture> sourceTexture;
    RC<Texture> timeTexture;
    float maxT_ = 0;

    GraphicsPipelineCache pipelineCache_;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        LoadSourceImage("Asset/Sample/10.FastMarchingMethod/0.png");
        pipelineCache_.SetDevice(device_);
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto swapchainTexture = graph->RegisterSwapchainTexture(device_->GetSwapchain());
        RGClearColor(graph, "ClearSwapchainTexture", swapchainTexture, { 0, 1, 1, 0 });

        const Vector2f fbSize = swapchainTexture->GetSize().To<float>();
        const Vector2f texSize = sourceTexture->GetSize().To<float>();
        const Vector2f drawSize = { texSize.x * 2.1f, texSize.y };
        const Vector2f relDrawSize = drawSize / fbSize;

        float sx, sy;
        if(relDrawSize.x >= relDrawSize.y)
        {
            sx = 0.8f;
            sy = sx * fbSize.x * drawSize.y / drawSize.x / fbSize.y;
        }
        else
        {
            sy = 0.8f;
            sx = sy * fbSize.y * drawSize.x / drawSize.y / fbSize.x;
        }

        const float sx0 = -sx;
        const float sx3 = +sx;
        const float sx1 = sx0 + 1.0f / 2.1f * (sx3 - sx0);
        const float sx2 = sx1 + 0.1f / 2.1f * (sx3 - sx0);

        const float sy0 = -sy;
        const float sy1 = +sy;

        DrawQuad(graph, swapchainTexture, sourceTexture, 1.0f, { sx0, sy0 }, { sx1, sy1 });
        DrawQuad(graph, swapchainTexture, timeTexture, 1 / maxT_, { sx2, sy0 }, { sx3, sy1 });
    }

    void LoadSourceImage(const std::string &filename)
    {
        auto image = Image<uint8_t>::Load(filename);
        sourceTexture = device_->CreateAndUploadTexture(RHI::TextureDesc
        {
            .format = RHI::Format::R8_UNorm,
            .width = image.GetWidth(),
            .height = image.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource
        }, image.GetData(), RHI::TextureLayout::ShaderTexture);
        sourceTexture->SetName("SourceTexture");

        std::vector<Vector2i> sources;
        for(int y = 0; y < image.GetSHeight(); ++y)
        {
            for(int x = 0; x < image.GetSWidth(); ++x)
            {
                if(image(x, y))
                {
                    sources.push_back({ x, y });
                }
            }
        }

        Image<float> T(image.GetSize());
        FMM2D(sources, T);
        timeTexture = device_->CreateAndUploadTexture(RHI::TextureDesc
        {
            .format = RHI::Format::R32_Float,
            .width = T.GetWidth(),
            .height = T.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource
        }, T.GetData(), RHI::TextureLayout::ShaderTexture);
        timeTexture->SetName("TimeTexture");

        maxT_ = 0;
        for(float t : T)
        {
            maxT_ = (std::max)(maxT_, t);
        }
    }

    void DrawQuad(
        GraphRef graph, RGTexture framebuffer,
        const RC<Texture> &tex, float scale,
        const Vector2f &lower, const Vector2f &upper)
    {
        RGAddRenderPass<true>(graph, "DrawQuad", RGColorAttachment
        {
            .rtv = framebuffer->GetRtv(),
            .isWriteOnly = true
        }, [framebuffer, tex, lower, upper, scale, this]
        {
            using Shader = RtrcShader::FMM::Visualize;
            Shader::Pass passData;
            passData.Texture = tex;
            passData.lower = lower;
            passData.upper = upper;
            passData.scale = scale;
            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

            auto pipeline = pipelineCache_.Get(GraphicsPipelineDesc
            {
                .shader = device_->GetShader<Shader::Name>(),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { framebuffer->GetFormat() }
                }
            });

            auto &commandBuffer = RGGetCommandBuffer();
            commandBuffer.BindGraphicsPipeline(pipeline);
            commandBuffer.BindGraphicsGroups(passGroup);
            commandBuffer.Draw(6, 1, 0, 0);
        });
    }
};

RTRC_APPLICATION_MAIN(
    FMMDemo,
    .title             = "Rtrc Sample: Fast Marching Methods",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
