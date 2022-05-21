#pragma once

#include <Rtrc/Utils/AnonymousName.h>
#include <Rtrc/Utils/Uncopyable.h>

RTRC_BEGIN

template<typename T, typename = std::enable_if_t<std::is_invocable_v<T>>>
class ScopeGuard : public Uncopyable
{
    bool call_ = true;

    T func_;

public:

    explicit ScopeGuard(const T &func)
        : func_(func)
    {

    }

    explicit ScopeGuard(T &&func)
        : func_(std::move(func))
    {

    }

    ~ScopeGuard()
    {
        if(call_)
            func_();
    }

    void Dismiss()
    {
        call_ = false;
    }
};

template<typename F, bool ExecuteOnException>
class ExceptionScopeGuard : public Uncopyable
{
    F func_;

    int exceptions_;

public:

    explicit ExceptionScopeGuard(const F &func)
        : func_(func), exceptions_(std::uncaught_exceptions())
    {

    }

    explicit ExceptionScopeGuard(F &&func)
        : func_(std::move(func)), exceptions_(std::uncaught_exceptions())
    {

    }

    ~ExceptionScopeGuard()
    {
        const int now_exceptions = std::uncaught_exceptions();
        if((now_exceptions > exceptions_) == ExecuteOnException)
            func_();
    }
};

struct ScopeGuardBuilder {};
struct ScopeGuardOnFailBuilder {};
struct ScopeGuardOnSuccessBuilder {};

template<typename Func>
auto operator+(ScopeGuardBuilder, Func &&f)
{
    return ScopeGuard<std::decay_t<Func>>(std::forward<Func>(f));
}

template<typename Func>
auto operator+(ScopeGuardOnFailBuilder, Func &&f)
{
    return ExceptionScopeGuard<std::decay_t<Func>, true>(std::forward<Func>(f));
}

template<typename Func>
auto operator+(ScopeGuardOnSuccessBuilder, Func &&f)
{
    return ExceptionScopeGuard<std::decay_t<Func>, false>(std::forward<Func>(f));
}

#define RTRC_SCOPE_EXIT auto RTRC_ANONYMOUS_NAME(_rtrcScopeExit) = ::Rtrc::ScopeGuardBuilder{} + [&]
#define RTRC_SCOPE_FAIL auto RTRC_ANONYMOUS_NAME(_rtrcScopeFail) = ::Rtrc::ScopeGuardOnFailBuilder{} + [&]
#define RTRC_SCOPE_SUCCESS auto RTRC_ANONYMOUS_NAME(_rtrcScopeSuccess) = ::Rtrc::ScopeGuardOnSuccessBuilder{} + [&]

RTRC_END
