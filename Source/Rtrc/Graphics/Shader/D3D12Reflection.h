#pragma once

#include <Rtrc/Graphics/Shader/ShaderReflection.h>

struct IDxcUtils;

RTRC_BEGIN

class D3D12Reflection : public ShaderReflection, public Uncopyable
{
public:

    // bindingNameToLocation: name -> (set, slot)
    D3D12Reflection(
        IDxcUtils                                        *dxcUtils,
        Span<std::byte>                                   code,
        const std::map<std::string, std::pair<int, int>> &bindingNameToLocation);

    std::vector<ShaderIOVar> GetInputVariables() const override { return inputVariables_; }
    Vector3i GetComputeShaderThreadGroupSize() const override { return threadGroupSize_; }
    std::vector<RHI::RawShaderEntry> GetEntries() const override;
    bool IsBindingUsed(std::string_view name) const override;

private:

    std::vector<ShaderIOVar>           inputVariables_;
    Vector3i                           threadGroupSize_;
    std::set<std::string, std::less<>> usedBindings_;
};

RTRC_END
