#pragma once

#include <sigslot/signal.hpp>

#include <Core/Uncopyable.h>

RTRC_BEGIN

// Simple wrapper of sigslot

enum class SignalThreadPolicy
{
    SingleThread,
    SpinLock,
    Mutex
};

template<SignalThreadPolicy ThreadPolicy, typename...Args>
class Signal;

class Connection
{
public:

    void Disconnect()
    {
        impl_.disconnect();
    }

private:

    template<SignalThreadPolicy ThreadPolicy, typename...Args>
    friend class Signal;

    sigslot::connection impl_;
};

template<SignalThreadPolicy ThreadPolicy, typename...Args>
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

    using Lockable = std::conditional_t<
        ThreadPolicy == SignalThreadPolicy::SingleThread,
        sigslot::detail::null_mutex,
        std::conditional_t<
            ThreadPolicy == SignalThreadPolicy::SpinLock,
            sigslot::detail::spin_mutex,
            std::mutex>>;

    sigslot::signal_base<Lockable, Args...> impl_;
};

RTRC_END
