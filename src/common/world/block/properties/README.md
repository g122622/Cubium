# 方块注册属性构建与状态谓词

## 目录结构树

```text
properties/
├── BlockProperties.cpp  # 根据材质获取默认的声音类型
├── BlockProperties.hpp  # 方块模型偏移类型
└── StatePredicate.hpp  # 求值。
```

## 内部模块关系

BlockProperties 在构造阶段组合材质、声音、掉落与物理参数；StatePredicate 表达共享的状态判定。偏移和乐器枚举独立放在 base/types。

## 上下游外部依赖关系

注册代码构造属性并传给 Block；本目录依赖方块基础材质、声音和地图颜色类型。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。
