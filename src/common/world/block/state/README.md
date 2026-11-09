# 不可变方块状态

## 目录结构树

```text
state/
├── BlockState.cpp  # 模块接口与实现
└── BlockState.hpp  # 方块状态
```

## 内部模块关系

BlockState 使用属性布局和值索引表示不可变状态，缓存静态属性并把上下文查询委派给 Block。3D 多方块匹配位于同级 `../pattern/`。

## 上下游外部依赖关系

依赖 common/util/property、碰撞形状与方块核心；区块调色板、网络状态编码和玩法逻辑持有状态引用。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。
