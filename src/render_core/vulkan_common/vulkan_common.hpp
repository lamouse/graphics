#pragma once
#include <vulkan/vulkan.hpp>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
#include <MoltenVK/mvk_vulkan.h>
#include <vulkan/vulkan_metal.h>
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
