# Cubium

![](./docs/logo.png)

![Lines of Code](https://raw.githubusercontent.com/g122622/Cubium/image-data/badge.svg)

![](./docs/screenshot1.png)
![](./docs/screenshot2.png)

现代 Minecraft 第三方完整实现，使用 C++20 和 Vulkan 渲染，采用客户端-服务端架构。

## 初始化项目

详细的初始化指引见 docs/SETUP.md

## 构建

详细的构建指南见 [docs/BUILD.md](docs/BUILD.md)，包含环境配置、构建命令、运行方式、着色器编译、VS Code IntelliSense 配置、本地 Sanitizer 构建等。

## 运行服务端

### Windows / Linux

直接运行构建产物：

```bash
./build/bin/RelWithDebInfo/minecraft-server --help
```

### macOS

macOS 上 Vulkan 加载器（vcpkg 打包的 `libvulkan`）默认不会发现 Homebrew 安装的 MoltenVK 驱动，必须通过 `VK_ICD_FILENAMES` 指向 ICD 清单，否则 `vkCreateInstance` 会返回 `-9`（`VK_ERROR_INCOMPATIBLE_DRIVER`）。启动命令：

```bash
# 方式 A：作为命令前缀（仅对本次启动生效）
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json build/bin/RelWithDebInfo/minecraft-server

# 方式 B：先 export 再运行（当前 shell 后续都生效）
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
build/bin/RelWithDebInfo/minecraft-server
```

> 注意：`VAR=value;` 单独一行（带分号、另起一行再运行二进制）只会设置 shell 变量而**不会**导出给子进程，进程仍然读不到该变量，会再次报 `-9`。请用上面两种写法之一。
>
> MoltenVK 经 Homebrew 安装时，ICD 清单路径为 `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json`；若改用 Vulkan SDK 安装，路径见 SDK 目录下的 `share/vulkan/icd.d/`。

> **说明**：项目目前以服务端为主，Linux/macOS 的默认 preset 均设 `MC_BUILD_CLIENT=OFF`。客户端构建已关闭，如需构建客户端请使用 Windows preset（`windows-clang-*`，设 `MC_BUILD_CLIENT=ON`）。

## 测试

项目测试基于 GoogleTest，通过 CTest 编排运行，支持单用例限时（默认 300 秒）、并行、按名筛选。完整指南见 docs/test/UNIT_TEST.md。

## clang-tidy 静态分析

见 docs/TIDY.md

## 依赖

见vcpkg.json
