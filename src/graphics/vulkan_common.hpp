#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#ifdef __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#include <vulkan/vulkan_metal.h>
#else
#include <vulkan/vulkan.h>
#endif