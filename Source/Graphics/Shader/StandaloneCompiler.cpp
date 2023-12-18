#include <Graphics/Shader/StandaloneCompiler.h>

RTRC_BEGIN

void StandaloneShaderCompiler::SetDevice(Ref<Device> device)
{
    compiler_.SetDevice(device);
}

RC<Shader> StandaloneShaderCompiler::Compile(
    const std::string                        &source,
    const std::map<std::string, std::string> &macros,
    bool                                      debug) const
{
    ShaderCompileEnvironment envir;
    envir.macros = macros;

    auto parsedShader = ParseShader(envir, &dxc_, source, "UnknownShader", "UnknownFile");
    auto &variant = parsedShader.variants.front();
    assert(parsedShader.keywords.empty());

    CompilableShader compilableShader;
    compilableShader.name                    = parsedShader.name;
    compilableShader.source                  = variant.source;
    compilableShader.sourceFilename          = parsedShader.sourceFilename;
    compilableShader.vertexEntry             = variant.vertexEntry;
    compilableShader.fragmentEntry           = variant.fragmentEntry;
    compilableShader.computeEntry            = variant.computeEntry;
    compilableShader.isRayTracingShader      = variant.vertexEntry.empty() &&
                                               variant.fragmentEntry.empty() &&
                                               variant.computeEntry.empty();
    compilableShader.entryGroups             = variant.entryGroups;
    compilableShader.bindingGroups           = variant.bindingGroups;
    compilableShader.ungroupedBindings       = variant.ungroupedBindings;
    compilableShader.aliases                 = variant.aliases;
    compilableShader.inlineSamplerDescs      = variant.inlineSamplerDescs;
    compilableShader.inlineSamplerNameToDesc = variant.inlineSamplerNameToDesc;
    compilableShader.pushConstantRanges      = variant.pushConstantRanges;
    
    return compiler_.Compile(envir, compilableShader, debug);
}

RTRC_END
