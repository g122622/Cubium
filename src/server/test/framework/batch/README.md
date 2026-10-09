# GameTest 批次

## 目录结构

```text
batch/
├── GameTestBatch.*               # 批次名称、函数集合和环境
├── GameTestBatchListener.hpp     # 批次初始化前与清理后的事件
└── BaseGameTestBatchRunner.*     # 顺序调度和实例所有权
```

## 内部关系与外部依赖

batch runner 按批次创建实例并加入 ticker；全部完成后清理实例，再推进下一批。
依赖函数、实例、环境与 ticker 接口。MinecraftGameTestBatchRunner 提供结构放置，
GameTestRunner 注入结果监听器，GameTestServer 注入期限监听器。

## 容易踩的坑

- 开始事件必须早于 beforeBatch、环境 setup 和结构放置，保证初始化受批次期限约束。
- 完成事件必须晚于环境 teardown、afterBatch 和实例清理，避免清理逃过期限。
- 放置结构可能在监听器挂载前失败，跟踪实例时须补发该结果。
