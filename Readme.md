# vulkan learn code

## 关于macos无法使用的问题

    创建实例时添加扩展名VK_KHR_portability_enumeration
    添加flags设置 ::vk::InstanceCreateFlags flags{VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};
    instanceCreateInfo.setFlags(flags);
    创建逻辑设备时添加扩展VK_KHR_portability_subset，这个和swapchan扩展在同一个列表中

## 关于窗口缩放

    当窗口缩放的时候需要重新创建swapchain，framebuffer
    swapchain重新创建时需要传入旧的swachain

## imgui

    immgui相关代码主要在src/g_app.cpp下

## format

    clang-format version 16.0.5
