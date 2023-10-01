#include <RHI/Window/NativeWindow.h>

#if RTRC_IS_WIN32

typedef struct GLFWwindow GLFWwindow;

// Avoid vulkan.h included by glfw3 conflicts with volk when unity build is enabled
#if RTRC_RHI_VULKAN
#include <volk.h>
#endif

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

RTRC_BEGIN

uint64_t GetWin32WindowHandleFromGlfwWindow(void *glfwWindow)
{
    auto handle = glfwGetWin32Window(static_cast<GLFWwindow *>(glfwWindow));
    static_assert(sizeof(handle) == sizeof(uint64_t));
    return reinterpret_cast<uint64_t>(handle);
}

RTRC_END

#endif // #if RTRC_IS_WIN32
