# 项目初始化指引

## 硬件配置

目前本项目能在下面两种平台上完成编译与运行：

- x86_64（Raptor Lake） + Windows 11 + Visual Studio（Clang）
- Apple Silicon（M4） + MacOS Tahoe + Clang

本项目代码量达到150W行级别（本体），若加上各类依赖则可逼近千万级，因此需要较为强大的硬件配置来完成编译。

## 开发环境

- 编译器只支持Clang20+，暂时未针对gcc和msvc做适配
- 需要Vulkan开发环境以及支持Vulkan的GPU
- Vcpkg

详见BUILD.md

## 克隆仓库

## 拉取子项目

```bash
git submodule update --init --recursive
```

## 修改agent文档

修改 CLAUDE.md，把里面的路径改为你自己机器上相关资源的路径，以供ai参考。
