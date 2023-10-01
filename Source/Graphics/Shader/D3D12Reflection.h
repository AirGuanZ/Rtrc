#pragma once

#include <Graphics/Shader/ShaderReflection.h>

struct IDxcUtils;

RTRC_BEGIN

class D3D12Reflection : public ShaderReflection, public Uncopyable
{
public:

    D3D12Reflection(IDxcUtils *dxcUtils, Span<std::byte> code, bool isLibrary);

    std::vector<ShaderIOVar> GetInputVariables() const override { return inputVariables_; }
    Vector3i GetComputeShaderThreadGroupSize() const override { return threadGroupSize_; }
    std::vector<RHI::RawShaderEntry> GetEntries() const override;
    bool IsBindingUsed(std::string_view name) const override;

private:

    void InitializeRegularReflection(IDxcUtils *dxcUtils, Span<std::byte> code);
    void InitializeLibraryReflection(IDxcUtils *dxcUtils, Span<std::byte> code);

    std::vector<ShaderIOVar>           inputVariables_;
    Vector3i                           threadGroupSize_;
    std::set<std::string, std::less<>> usedBindings_;
    std::vector<RHI::RawShaderEntry>   entries_; // For library only
};

RTRC_END
