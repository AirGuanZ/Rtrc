#pragma once

#include <sigslot/signal.hpp>

#include <Rtrc/Utils/Uncopyable.h>

RTRC_BEGIN

// Simple wrapper of sigslot

template<typename...Args>
class Signal;

class Connection
{
public:

    void Disconnect()
    {
        impl_.disconnect();
    }

private:

    template<typename...Args>
    friend class Signal;

    sigslot::connection impl_;
};

template<typename...Args>
class Signal : public Uncopyable
{
public:

    template<typename...SlotArgs>
    auto Connect(SlotArgs&&...slotArgs)
    {
        Connection ret;
        ret.impl_ = impl_.connect(std::forward<SlotArgs>(slotArgs)...);
        return ret;
    }

    void Emit(const Args...args)
    {
        impl_(args...);
    }

private:

    sigslot::signal<Args...> impl_;
};

RTRC_END
