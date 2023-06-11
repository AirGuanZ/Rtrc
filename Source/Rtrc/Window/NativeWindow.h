#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

#if RTRC_IS_WIN32
uint64_t GetWin32WindowHandleFromGlfwWindow(void *glfwWindow);
#endif

RTRC_END
