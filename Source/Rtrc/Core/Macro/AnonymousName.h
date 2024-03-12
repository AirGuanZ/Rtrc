#pragma once

/*
    Helper macro for generating unique identifiers (in non-global scope)
    Example:
        RTRC_ANONYMOUS_NAME(FOO) expands FOOXXX, where XXX is a unique number in current compilation unit
*/

#define RTRC_ANONYMOUS_NAME_CAT(A, B) RTRC_ANONYMOUS_NAME_CAT_IMPL(A, B)
#define RTRC_ANONYMOUS_NAME_CAT_IMPL(A, B) A##B

#ifdef __COUNTER__
#define RTRC_ANONYMOUS_NAME(STR) RTRC_ANONYMOUS_NAME_CAT(STR, __COUNTER__)
#else
#define RTRC_ANONYMOUS_NAME(STR) RTRC_ANONYMOUS_NAME_CAT(STR, __LINE__)
#endif
