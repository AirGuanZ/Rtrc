#pragma once

#include <Rtrc/ShaderCommon/Reflection/ShaderReflection.h>

RTRC_BEGIN

class SPIRVReflection : public ShaderReflection, public Uncopyable
{
public:

    SPIRVReflection(Span<unsigned char> code, std::string entryPoint);

    std::vector<ShaderIOVar> GetInputVariables() const override;
    Vector3i GetComputeShaderThreadGroupSize() const override;
    std::vector<RHI::RawShaderEntry> GetEntries() const override;
    bool IsBindingUsed(std::string_view name) const override;

private:

    struct DeleteShaderModule
    {
        void operator()(SpvReflectShaderModule *ptr) const;
    };

    std::string                                                 entry_;
    std::set<std::string, std::less<>>                          usedBindings_;
    std::unique_ptr<SpvReflectShaderModule, DeleteShaderModule> shaderModule_;
};

RTRC_END
