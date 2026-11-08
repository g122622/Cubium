# 服务端世界时间

## 目录结构树

```text
time/
├── TimeManager.hpp/cpp  # 运行时时间管理门面
└── GameTime.hpp/cpp  # 时钟状态与推进实现
```

## 内部模块关系

TimeManager 持有 GameTime，统一提供累计时间、日内时间和推进控制。

## 上下游外部依赖关系

应用装配与命令使用 TimeManager；共享月相和时间线位于 common/world/time。

## 容易踩的坑

- dayTime 保留累计值，只有日内计算才归一化。
- 月相与默认亮度使用共享时间规则，维度实际环境值由属性系统求值。
