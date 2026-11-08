# 方块族的不可变声音及交互规则

## 目录结构树

```text
family/
├── BlockSetType.cpp  # 模块接口与实现
├── BlockSetType.hpp  # 同一方块族共享的声音与激活规则；实例在进程期间保持不变。
├── WoodType.cpp  # 模块接口与实现
└── WoodType.hpp  # 木材族描述；告示牌与栅栏门共用同一份定义。
```

## 内部模块关系

BlockSetType 组合门、活板门、按钮和压力板的激活规则及声音；WoodType 关联方块族，并增加告示牌与栅栏门的声音。各方块共享 const 描述，注册时显式传入。

## 上下游外部依赖关系

依赖 common/sound/SoundEvents 和本领域声音组合；原版注册、门类、按钮、压力板和告示牌消费这些基础描述。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。
