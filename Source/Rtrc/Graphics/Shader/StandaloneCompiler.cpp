#include <Rtrc/Graphics/Shader/StandaloneCompiler.h>

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

    auto parsedShader = MakeRC<ParsedShader>(ParseShader(envir, &dxc_, source, "UnknownShader", "UnknownFile"));
    auto &variant = parsedShader->variants.front();
    assert(parsedShader->keywords.empty());

    CompilableShader compilableShader;
    compilableShader.envir                   = std::move(envir);
    compilableShader.name                    = parsedShader->name;
    compilableShader.source                  = "_rtrc_generated_shader_prefix " + source;
    compilableShader.sourceFilename          = parsedShader->sourceFilename;
    compilableShader.vertexEntry             = variant.vertexEntry;
    compilableShader.fragmentEntry           = variant.fragmentEntry;
    compilableShader.computeEntry            = variant.computeEntry;
    compilableShader.taskEntry               = variant.taskEntry;
    compilableShader.meshEntry               = variant.meshEntry;
    compilableShader.isRayTracingShader      = variant.vertexEntry.empty() &&
                                               variant.fragmentEntry.empty() &&
                                               variant.computeEntry.empty() &&
                                               variant.workGraphEntryNodes.empty();
    compilableShader.rayTracingEntryGroups   = variant.rayTracingEntryGroups;
    compilableShader.bindingGroups           = variant.bindingGroups;
    compilableShader.ungroupedBindings       = variant.ungroupedBindings;
    compilableShader.aliases                 = variant.aliases;
    compilableShader.inlineSamplerDescs      = variant.inlineSamplerDescs;
    compilableShader.inlineSamplerNameToDesc = variant.inlineSamplerNameToDesc;
    compilableShader.pushConstantRanges      = variant.pushConstantRanges;
    compilableShader.originalParsedShader    = std::move(parsedShader);
    compilableShader.originalVariantIndex    = 0;
    
    return compiler_.Compile(compilableShader, debug);
}

RTRC_END
