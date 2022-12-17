#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Utility/AnonymousName.h>

RTRC_BEGIN

class ScopedGPUDebugEventGuard : public Uncopyable
{
public:

    explicit ScopedGPUDebugEventGuard(
        CommandBuffer &cmd, std::string name, const std::optional<Vector4f> &color = std::nullopt)
        : cmd_(cmd)
    {
        cmd_.BeginDebugEvent(std::move(name), color);
    }

    ~ScopedGPUDebugEventGuard()
    {
        cmd_.EndDebugEvent();
    }

private:

    CommandBuffer &cmd_;
};

#define RTRC_SCOPED_GPU_EVENT(CMD, NAME, ...) \
    ::Rtrc::ScopedGPUDebugEventGuard RTRC_ANONYMOUS_NAME(gpuEventGuard){ CMD, NAME, __VA_ARGS__ }

RTRC_END
