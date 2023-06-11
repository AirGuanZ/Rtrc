#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12RawShader, RawShader)
{
public:

    DirectX12RawShader(
        std::vector<std::byte>      shaderByteCode,
        std::vector<RawShaderEntry> entries)
        : rawShaderByteCode_(std::move(shaderByteCode)), byteCode_{}, entries_(std::move(entries))
    {
        byteCode_.pShaderBytecode = rawShaderByteCode_.data();
        byteCode_.BytecodeLength = rawShaderByteCode_.size();
    }

    const D3D12_SHADER_BYTECODE &_internalGetShaderByteCode() const
    {
        return byteCode_;
    }

private:

    std::vector<std::byte>      rawShaderByteCode_;
    D3D12_SHADER_BYTECODE       byteCode_;
    std::vector<RawShaderEntry> entries_;
};

RTRC_RHI_D3D12_END
