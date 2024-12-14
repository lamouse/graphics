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

```text
详情查看文件CMakeUserPresets-demo.json
$penv{path}获取系统的环境变量
```
