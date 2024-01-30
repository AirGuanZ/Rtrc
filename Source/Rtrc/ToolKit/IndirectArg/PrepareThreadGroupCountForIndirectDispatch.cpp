#include <Rtrc/ToolKit/Resource/ResourceManager.h>
#include <Rtrc/ToolKit/IndirectArg/PrepareThreadGroupCountForIndirectDispatch.h>

#include "PrepareThreadGroupCountForIndirectDispatch.shader.outh"

RTRC_RENDERER_BEGIN

RGPass PrepareThreadGroupCount1D(
    GraphRef        renderGraph,
    const RGBufSrv &threadCountBuffer,
    const RGBufUav &threadGroupCountBuffer,
    int             threadGroupSize)
{
    using Shader = RtrcShader::Builtin::PrepareThreadGroupCountForIndirectDispatch_1D;
    using ThreadGroupSize = Shader::ThreadGroupSize;

    auto Dispatch = [&]<ThreadGroupSize GroupSize>()
    {
        FastKeywordContext keywordContext;
        keywordContext.Set(RTRC_FAST_KEYWORD(ThreadGroupSize), std::to_underlying(GroupSize));
        auto shader = renderGraph->GetDevice()
            ->GetShaderTemplate<Shader::Name>()
            ->GetVariant(keywordContext);

        using Variant = Shader::Variant<GroupSize>;
        typename Variant::Pass passData;
        passData.ThreadCountBuffer = threadCountBuffer;
        passData.ThreadGroupCountBuffer = threadGroupCountBuffer;
        if constexpr(GroupSize == ThreadGroupSize::Others)
        {
            passData.threadGroupSize = threadGroupSize;
        }

        return DispatchWithThreadCount(renderGraph, Shader::Name, shader, 1, passData);
    };

    RGPassImpl *pass;
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
