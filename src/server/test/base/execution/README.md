# 测试执行预算

## 目录结构

```text
execution/
└── GameTestExecutionPlan.hpp  # 稳定分片、批次预算与 tick 期限
```

## 内部模块关系

`GameTestExecutionPlan` 根据测试声明的执行成本分片，并为宿主计算有限的执行期限。

## 上下游外部依赖关系

依赖基础类型和标准库；由 `facade/GameTestServer` 调用，不访问世界或测试实例。

## 容易踩的坑

普通 tick 最多两秒。启动和声明同步区块生成的测试允许使用批次期限，但不能超过整轮期限。同步生成测试须使用独立批次，避免影响普通测试的卡死检测。
