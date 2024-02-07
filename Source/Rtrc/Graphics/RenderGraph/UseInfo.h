#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

struct RGUseInfo
{
    RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
};

constexpr RGUseInfo operator|(const RGUseInfo &lhs, const RGUseInfo &rhs)
{
    assert(lhs.layout == rhs.layout);
    return RGUseInfo
    {
        .layout   = lhs.layout,
        .stages   = lhs.stages   | rhs.stages,
        .accesses = lhs.accesses | rhs.accesses
    };
}

namespace RG
{

    inline constexpr RGUseInfo ColorAttachment =
    {
        .layout   = RHI::TextureLayout::ColorAttachment,
        .stages   = RHI::PipelineStage::RenderTarget,
        .accesses = RHI::ResourceAccess::RenderTargetWrite | RHI::ResourceAccess::RenderTargetRead
    };

    inline constexpr RGUseInfo ColorAttachmentReadOnly =
    {
        .layout   = RHI::TextureLayout::ColorAttachment,
        .stages   = RHI::PipelineStage::RenderTarget,
        .accesses = RHI::ResourceAccess::RenderTargetRead
    };

    inline constexpr RGUseInfo ColorAttachmentWriteOnly =
    {
        .layout   = RHI::TextureLayout::ColorAttachment,
        .stages   = RHI::PipelineStage::RenderTarget,
        .accesses = RHI::ResourceAccess::RenderTargetWrite
    };

    inline constexpr RGUseInfo DepthStencilAttachment =
    {
        .layout   = RHI::TextureLayout::DepthStencilAttachment,
        .stages   = RHI::PipelineStage::DepthStencil,
        .accesses = RHI::ResourceAccess::DepthStencilRead | RHI::ResourceAccess::DepthStencilWrite
    };

    inline constexpr RGUseInfo DepthStencilAttachmentWriteOnly =
    {
        .layout   = RHI::TextureLayout::DepthStencilAttachment,
        .stages   = RHI::PipelineStage::DepthStencil,
        .accesses = RHI::ResourceAccess::DepthStencilWrite
    };

    inline constexpr RGUseInfo DepthStencilAttachmentReadOnly =
    {
        .layout   = RHI::TextureLayout::DepthStencilReadOnlyAttachment,
        .stages   = RHI::PipelineStage::DepthStencil,
        .accesses = RHI::ResourceAccess::DepthStencilRead
    };

    inline constexpr RGUseInfo ClearColor =
    {
        .layout   = RHI::TextureLayout::ClearDst,
        .stages   = RHI::PipelineStage::Clear,
        .accesses = RHI::ResourceAccess::ClearColorWrite
    };

    inline constexpr RGUseInfo ClearDepthStencil =
    {
        .layout   = RHI::TextureLayout::ClearDst,
        .stages   = RHI::PipelineStage::Clear,
        .accesses = RHI::ResourceAccess::ClearDepthStencilWrite
    };

    inline constexpr RGUseInfo PS_Texture =
    {
        .layout   = RHI::TextureLayout::ShaderTexture,
        .stages   = RHI::PipelineStage::FragmentShader,
        .accesses = RHI::ResourceAccess::TextureRead
    };

    inline constexpr RGUseInfo CS_Texture =
    {
        .layout   = RHI::TextureLayout::ShaderTexture,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::TextureRead
    };

    inline constexpr RGUseInfo CS_RWTexture =
    {
        .layout   = RHI::TextureLayout::ShaderRWTexture,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWTextureRead | RHI::ResourceAccess::RWTextureWrite
    };

    inline constexpr RGUseInfo CS_RWTexture_WriteOnly =
    {
        .layout   = RHI::TextureLayout::ShaderRWTexture,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWTextureWrite
    };

    inline constexpr RGUseInfo CS_Buffer =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::BufferRead
    };

    inline constexpr RGUseInfo CS_RWBuffer =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWBufferRead | RHI::ResourceAccess::RWBufferWrite
    };

    inline constexpr RGUseInfo CS_RWBuffer_WriteOnly =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWBufferWrite
    };

    inline constexpr RGUseInfo CS_StructuredBuffer =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::StructuredBufferRead
    };

    inline constexpr RGUseInfo CS_RWStructuredBuffer =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWStructuredBufferRead | RHI::ResourceAccess::RWStructuredBufferWrite
    };

    inline constexpr RGUseInfo CS_RWStructuredBuffer_WriteOnly =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::RWStructuredBufferWrite
    };

    inline constexpr RGUseInfo CopyDst =
    {
        .layout   = RHI::TextureLayout::CopyDst,
        .stages   = RHI::PipelineStage::Copy,
        .accesses = RHI::ResourceAccess::CopyWrite
    };

    inline constexpr RGUseInfo CopySrc =
    {
        .layout   = RHI::TextureLayout::CopySrc,
        .stages   = RHI::PipelineStage::Copy,
        .accesses = RHI::ResourceAccess::CopyRead
    };

    inline constexpr RGUseInfo BuildAS_Output =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::BuildAS,
        .accesses = RHI::ResourceAccess::WriteAS
    };

    inline constexpr RGUseInfo BuildAS_Scratch =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::BuildAS,
        .accesses = RHI::ResourceAccess::BuildASScratch
    };

    inline constexpr RGUseInfo CS_ReadAS =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::ReadAS
    };

    inline constexpr RGUseInfo IndirectDispatchRead =
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = RHI::PipelineStage::IndirectCommand,
        .accesses = RHI::ResourceAccess::IndirectCommandRead
    };

} // namespace RG

RTRC_END
