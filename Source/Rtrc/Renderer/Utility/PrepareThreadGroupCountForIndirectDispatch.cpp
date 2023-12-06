#include <Rtrc/Resource/ResourceManager.h>
#include <Rtrc/Renderer/Utility/PrepareThreadGroupCountForIndirectDispatch.h>

#include "PrepareThreadGroupCountForIndirectDispatch.shader.outh"

RTRC_RENDERER_BEGIN

RG::Pass *PrepareThreadGroupCount1D(
    ObserverPtr<ResourceManager> resources,
    ObserverPtr<RG::RenderGraph> renderGraph,
    const RG::BufferResourceSrv &threadCountBuffer,
    const RG::BufferResourceUav &threadGroupCountBuffer,
    int                          threadGroupSize)
{
    constexpr char SHADER[] = "PrepareThreadGroupCountForIndirectDispatch_1D";

    using StaticShader = StaticShaderInfo<SHADER>;
    using ThreadGroupSize = StaticShader::ThreadGroupSize;

    auto Dispatch = [&]<ThreadGroupSize GroupSize>()
    {
        FastKeywordContext keywordContext;
        keywordContext.Set(RTRC_FAST_KEYWORD(ThreadGroupSize), std::to_underlying(GroupSize));
        auto shader = resources
            ->GetShaderManager()
            ->GetShaderTemplate<SHADER>()
            ->GetVariant(keywordContext);

        using Variant = StaticShader::Variant<GroupSize>;
        typename Variant::Pass passData;
        passData.ThreadCountBuffer = threadCountBuffer;
        passData.ThreadGroupCountBuffer = threadGroupCountBuffer;
        if constexpr(GroupSize == ThreadGroupSize::Others)
        {
            passData.threadGroupSize = threadGroupSize;
        }

        return renderGraph->CreateComputePassWithThreadCount(
            SHADER, shader, 1, passData);
    };

    RG::Pass *pass;
    if(threadGroupSize == 32)
    {
        pass = Dispatch.operator()<ThreadGroupSize::s32>();
    }
    else if(threadGroupSize == 64)
    {
        pass = Dispatch.operator()<ThreadGroupSize::s64>();
    }
    else if(threadGroupSize == 128)
    {
        pass = Dispatch.operator()<ThreadGroupSize::s128>();
    }
    else if(threadGroupSize == 256)
    {
        pass = Dispatch.operator()<ThreadGroupSize::s256>();
    }
    else
    {
        pass = Dispatch.operator()<ThreadGroupSize::Others>();
    }
    return pass;
}

RTRC_RENDERER_END
