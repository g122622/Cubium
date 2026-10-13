# 测试标签

## 目录结构

```text
tags/
└── GameTestTags.hpp  # 套件筛选与执行特性标签
```

## 内部模块关系

标签是无状态字符串常量，由注册器保存，宿主在运行期读取。

## 上下游外部依赖关系

只依赖标准库；供原生、脚本注册器和 `facade/GameTestServer` 使用。

## 容易踩的坑

`execution:sync-worldgen` 声明测试在 tick 中同步生成区块，必须放入独立批次。它只改变单 tick 预算，批次和整轮硬期限仍然生效。
