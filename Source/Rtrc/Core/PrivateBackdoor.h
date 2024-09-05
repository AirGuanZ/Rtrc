#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

// Inspired by https://gist.github.com/schellingb/5506678176948fa82494d128b95b9876

#define RTRC_DEFINE_PRIVATE_BACKDOOR(NAME, CLASS, MEMBER, MEM_PTR) \
    template<MEMBER CLASS::*MemberPointer>                         \
    struct PrivateBackdoorHelper##NAME                             \
    {                                                              \
        friend MEMBER &NAME(CLASS &object)                         \
        {                                                          \
            return object.*MemberPointer;                          \
        }                                                          \
        friend const MEMBER &NAME(const CLASS &object)             \
        {                                                          \
            return object.*MemberPointer;                          \
        }                                                          \
    };                                                             \
    template struct PrivateBackdoorHelper##NAME<MEM_PTR>;          \
    MEMBER &NAME(CLASS&);                                          \
    const MEMBER &NAME(const CLASS&)

RTRC_END
