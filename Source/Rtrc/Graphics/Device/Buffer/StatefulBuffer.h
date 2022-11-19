#pragma once

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>

RTRC_BEGIN

struct BufferState
{
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;

    BufferState() = default;
    BufferState(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses);
};

class StatefulBuffer : public Buffer
{
public:

    static RC<StatefulBuffer> FromBuffer(RC<Buffer> buffer);

    void SetState(const BufferState &state);
    void SetState(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses);
    const BufferState &GetState() const;

private:

    BufferState state_;
};

class WrappedStatefulBuffer : public StatefulBuffer
{
public:
    
    explicit WrappedStatefulBuffer(RC<Buffer> buffer = {}, const BufferState &state = {});

private:

    RC<Buffer> buffer_;
};

inline BufferState::BufferState(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
    : stages(stages), accesses(accesses)
{
    
}

inline RC<StatefulBuffer> StatefulBuffer::FromBuffer(RC<Buffer> buffer)
{
    return MakeRC<WrappedStatefulBuffer>(std::move(buffer));
}

inline void StatefulBuffer::SetState(const BufferState &state)
{
    state_ = state;
}

inline void StatefulBuffer::SetState(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
{
    state_ = { stages, accesses };
}

inline const BufferState &StatefulBuffer::GetState() const
{
    return state_;
}

inline WrappedStatefulBuffer::WrappedStatefulBuffer(RC<Buffer> buffer, const BufferState &state)
    : buffer_(std::move(buffer))
{
    rhiBuffer_ = buffer_->GetRHIObject();
    size_ = buffer_->GetSize();
    SetState(state);
}

RTRC_END
