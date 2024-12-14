# vulkan learn code

## 关于macos无法使用的问题

```text
创建实例时添加扩展名VK_KHR_portability_enumeration
添加flags设置 ::vk::InstanceCreateFlags flags{VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};
instanceCreateInfo.setFlags(flags);
创建逻辑设备时添加扩展VK_KHR_portability_subset，这个和swapchan扩展在同一个列表中
```

## 关于窗口缩放

```text
当窗口缩放的时候需要重新创建swapchain，framebuffer
swapchain重新创建时需要传入旧的swachain
```

## imgui

```text
immgui相关代码主要在src/g_app.cpp下, 当前使用的docking分支
```

## format

```text
clang-format version 16.0.5
```

## 依赖

```text
本项目优先使用vcpkg的包
VulkanSDK-1.3.290.0
```

## 关于cmake preset

```text
如果不使用cmake preset需要在vs code setting.json中加入以下配置
"cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "D:/Program/vcpkg/scripts/buildsystems/vcpkg.cmake"
}
如果在Windows下使用mingw64需要设置加入
"VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"
```

### cmake preset一个备份demo

```json
{
    "version": 2,
    "configurePresets": [
        {
            "name": "mingw-gcc-debug",
            "displayName": "GCC 14.2.0 x86_64 (mingw64 debug)",
            "description": "使用编译器: C = D:\\Program\\msys64\\mingw64\\bin\\gcc.exe, CXX = D:\\Program\\msys64\\mingw64\\bin\\g++.exe",
            "inherits": "vcpkg-debug",
            "environment": {
                "VCPKG_ROOT": "D:\\Program\\vcpkg"
            },
            "cacheVariables": {
                "CMAKE_C_COMPILER": "D:/Program/msys64/mingw64/bin/gcc.exe",
                "CMAKE_CXX_COMPILER": "D:/Program/msys64/mingw64/bin/g++.exe",
                "VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"
            }
        },
        {
            "name": "mingw-gcc-release",
            "displayName": "GCC 14.2.0 x86_64(mingw64 release)",
            "description": "使用编译器: C = D:\\Program\\msys64\\mingw64\\bin\\gcc.exe, CXX = D:\\Program\\msys64\\mingw64\\bin\\g++.exe",
            "inherits": "vcpkg-release",
            "environment": {
                "VCPKG_ROOT": "D:\\Program\\vcpkg"
            },
            "cacheVariables": {
                "CMAKE_C_COMPILER": "D:/Program/msys64/mingw64/bin/gcc.exe",
                "CMAKE_CXX_COMPILER": "D:/Program/msys64/mingw64/bin/g++.exe",
                "VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"
            }
        },
        {
            "name": "msvc-debug",
            "displayName": "Visual Studio 2022 - amd64(debug)",
            "description": "将编译器用于 Visual Studio 17 2022 (x64 体系结构)",
            "inherits": "vcpkg-debug",
            "environment": {
                "VCPKG_ROOT": "D:\\Program\\vcpkg"
            },
            "cacheVariables": {
                "CMAKE_C_COMPILER": "cl.exe",
                "CMAKE_CXX_COMPILER": "cl.exe"
            }
        },
        {
            "name": "msvc-release",
            "displayName": "Visual Studio 2022 - amd64(release)",
            "description": "将编译器用于 Visual Studio 17 2022 (x64 体系结构)",
            "inherits": "vcpkg-release",
            "environment": {
                "VCPKG_ROOT": "D:\\Program\\vcpkg"
            },
            "cacheVariables": {
                "CMAKE_C_COMPILER": "cl.exe",
                "CMAKE_CXX_COMPILER": "cl.exe"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "mingw-gcc-debug",
            "description": "",
            "displayName": "Debug",
            "verbose": true,
            "configurePreset": "mingw-gcc-debug"
        },
        {
            "name": "mingw-gcc-release",
            "description": "",
            "displayName": "Release",
            "verbose": true,
            "configurePreset": "mingw-gcc-release"
        },
        {
            "name": "msvc-debug",
            "displayName": "Debug",
            "verbose": true,
            "configurePreset": "msvc-debug"
        },
        {
            "name": "msvc-release",
            "displayName": "Release",
            "verbose": true,
            "configurePreset": "msvc-release",
            "configuration": "Release"
        }
    ]
}
```
