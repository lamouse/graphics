#pragma once
#include <vulkan/vulkan.h>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>

#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan_wayland.h>
#endif

#ifdef __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#include <vulkan/vulkan_metal.h>
#elif defined(_WIN32)
#include <vulkan/vulkan_win32.h>
#else

#endif