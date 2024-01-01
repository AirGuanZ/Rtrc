#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_RG_BEGIN

struct UseInfo
{
    RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
};

constexpr UseInfo operator|(const UseInfo &lhs, const UseInfo &rhs)
{
    assert(lhs.layout == rhs.layout);
    return UseInfo
    {
        .layout   = lhs.layout,
        .stages   = lhs.stages   | rhs.stages,
        .accesses = lhs.accesses | rhs.accesses
    };
}

inline constexpr UseInfo ColorAttachment =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite | RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo ColorAttachmentReadOnly =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo ColorAttachmentWriteOnly =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite
};

inline constexpr UseInfo DepthStencilAttachment =
{
    .layout   = RHI::TextureLayout::DepthStencilAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead | RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DepthStencilAttachmentWriteOnly =
{
    .layout   = RHI::TextureLayout::DepthStencilAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DepthStencilAttachmentReadOnly =
{
    .layout   = RHI::TextureLayout::DepthStencilReadOnlyAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead
};

inline constexpr UseInfo ClearDst =
{
    .layout   = RHI::TextureLayout::ClearDst,
    .stages   = RHI::PipelineStage::Clear,
    .accesses = RHI::ResourceAccess::ClearWrite
};

inline constexpr UseInfo PS_Texture =
{
    .layout   = RHI::TextureLayout::ShaderTexture,
    .stages   = RHI::PipelineStage::FragmentShader,
    .accesses = RHI::ResourceAccess::TextureRead
};

inline constexpr UseInfo CS_Texture =
{
    .layout   = RHI::TextureLayout::ShaderTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::TextureRead
};

inline constexpr UseInfo CS_RWTexture =
{
    .layout   = RHI::TextureLayout::ShaderRWTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWTextureRead | RHI::ResourceAccess::RWTextureWrite
};

inline constexpr UseInfo CS_RWTexture_WriteOnly =
{
    .layout   = RHI::TextureLayout::ShaderRWTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWTextureWrite
};

inline constexpr UseInfo CS_Buffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::BufferRead
};

inline constexpr UseInfo CS_RWBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWBufferRead | RHI::ResourceAccess::RWBufferWrite
};

inline constexpr UseInfo CS_RWBuffer_WriteOnly =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWBufferWrite
};

inline constexpr UseInfo CS_StructuredBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::StructuredBufferRead
};

inline constexpr UseInfo CS_RWStructuredBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWStructuredBufferRead | RHI::ResourceAccess::RWStructuredBufferWrite
};

inline constexpr UseInfo CS_RWStructuredBuffer_WriteOnly =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWStructuredBufferWrite
};

inline constexpr UseInfo CopyDst =
{
    .layout   = RHI::TextureLayout::CopyDst,
    .stages   = RHI::PipelineStage::Copy,
    .accesses = RHI::ResourceAccess::CopyWrite
};

inline constexpr UseInfo CopySrc =
{
    .layout   = RHI::TextureLayout::CopySrc,
    .stages   = RHI::PipelineStage::Copy,
    .accesses = RHI::ResourceAccess::CopyRead
};

inline constexpr UseInfo BuildAS_Output =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::BuildAS,
    .accesses = RHI::ResourceAccess::WriteAS
};

inline constexpr UseInfo BuildAS_Scratch =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::BuildAS,
    .accesses = RHI::ResourceAccess::BuildASScratch
};

inline constexpr UseInfo CS_ReadAS =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::ReadAS
};

inline constexpr UseInfo IndirectDispatchRead =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::IndirectCommand,
    .accesses = RHI::ResourceAccess::IndirectCommandRead
};

RTRC_RG_END
