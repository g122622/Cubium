# GameTest 实例状态机

## 目录结构

```text
instance/
├── BaseGameTestInstance.* # setup、回调、序列、tick 超时和结果事件
└── GameTestState.hpp     # 未开始、运行、成功、失败、停止状态
```

## 内部关系与外部依赖

实例持有函数、helper、序列和监听器，由 ticker 推进并由 batch runner 持有。
依赖 framework 接口、base 的错误模型和标准时钟；MinecraftGameTestInstance 提供世界绑定，
runner 监听器把状态转发到报告器。

## 容易踩的坑

- 必须先通知开始再执行测试体，否则同步完成的结果会被报告器覆盖。
- setup 使用负 tick；游戏 tick 期限不等于实际时间，独立看门狗由 GameTestServer 安装。
- 实际耗时从实例创建开始，成功/失败后固定结束时刻。
