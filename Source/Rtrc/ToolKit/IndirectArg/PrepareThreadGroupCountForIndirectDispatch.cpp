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
    auto shader = renderGraph->GetDevice()->GetShader<Shader::Name>();
    Shader::Pass passData;
    passData.ThreadCountBuffer = threadCountBuffer;
    passData.ThreadGroupCountBuffer = threadGroupCountBuffer;
    passData.threadGroupSize = threadGroupSize;
    return RGDispatchWithThreadCount(renderGraph, Shader::Name, shader, 1, passData);
}

RTRC_RENDERER_END
