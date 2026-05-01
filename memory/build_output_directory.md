---
name: build_output_directory
description: 项目构建产出目录位置
type: reference
---

项目的构建产出在 `./build/bin/RelWithDebInfo/` 目录下，而非 `./build/bin/`。

运行测试时使用：`./build/bin/RelWithDebInfo/mc_tests --gtest_filter="..."`
