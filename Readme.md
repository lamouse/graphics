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

## format

```text
clang-format version 16.0.5
```

## clang-tidy

```text
因为一些原因clang-tidy检查有些问题，或者确认是没有问题的地方需要屏蔽
在行后    //NOLINT(modernize-use-trailing-return-type)
下一行 // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
```

## 依赖

```text
本项目优先使用vcpkg的包
vcpkg安装ktx的时候需要注意windows11系统需要开启开发者模式并将区域设置中的使用Unicode打上对勾，
但是这样对仅支持gbk的东西，需要注意
```

 ## 代码规范

1. 出现原始指针时一律代表没有该指针的所有权

## 关于cmake preset

```text
如果不使用cmake preset需要在vs code setting.json中加入以下配置
"cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "D:/Program/vcpkg/scripts/buildsystems/vcpkg.cmake"
}
如果在Windows下使用mingw64需要设置加入
"VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"

现在改为设置环境变量VCPKG_ROOT, 使用mingw需要在project前面设置CMAKE_TOOLCHAIN_FILE
```

### cmake preset一个备份demo

```text
虽然现在推荐Presets，但是配置完挺麻烦，先预留一个样本吧
Windows还是少使用mingw
详情查看文件CMakeUserPresets-demo.json
$penv{path}获取系统的环境变量
```

### 添加了一个功能，将yaml的配置信息生成一个c++结构体使用

```text
使用的时候需要在字段后面加入注释#{type:bool}，在项目配置的时候会根据type:后面的类型进行类型确认,下面提供一个demo
    1. 支持基础类型
    2. 支持结构体
    3. 支持vector
```

### 编译器问题

```text
在windows上使用clang的时候会提示unknown argument: '-ignore:4221'
解决办法是去vcpkg的installed\x64-windows\share\absl把abslTargets.cmake “;-ignore:4221”替换为空
去掉以后就可以正常用clang编译了，如果懒得改用clang-cl编译也可以，不行就用msvc
由于使用的c++20的module所以mingw的gcc无法编译因为mingw没有正确设置模块相关代码，但是在Linux中可以编译
```

## 需要安装一些python库

```python
pip install xxhash
pip install sphinx
pip install sphinx-rtd-theme
```

## vs code lldb启动配置

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lldb",
            "request": "launch",
            "name": "LLDB",
            "args": [],
            "cwd": "${command:cmake.launchTargetDirectory}",
            "program": "${command:cmake.launchTargetPath}",
            "env": {
                "PATH": "${env:PATH}:${command:cmake.launchTargetDirectory}"
            }
        }
    ]
}
```

## 学习vulkan的一些记录

 1. 有些教学还在设置逻辑设备的验证层，其实不需要了[说明](https://www.lunarg.com/wp-content/uploads/2019/04/UberLayer_V3.pdf)
