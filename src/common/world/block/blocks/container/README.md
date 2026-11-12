# 存储与运输方块行为

## 目录结构树

```text
container/
├── ChestBlock.cpp  # 模块接口与实现
├── ChestBlock.hpp  # 箱子方块
├── CopperChestBlock.cpp  # 铜箱子基类（未涂蜡 Unaffected 等级）
├── CopperChestBlock.hpp  # 铜箱子基类声明（双箱合并、铜块→铜箱子映射）
├── WeatheringCopperChestBlock.cpp  # 可氧化铜箱子的随机刻氧化
├── WeatheringCopperChestBlock.hpp  # 可氧化铜箱子（Exposed/Weathered/Oxidized）
├── WaxedCopperChestBlock.hpp  # 涂蜡铜箱子（isWaxed 恒为 true）
├── HopperBlock.cpp  # 模块接口与实现
├── HopperBlock.hpp  # 漏斗方块
├── ShelfBlock.cpp  # 模块接口与实现
├── ShelfBlock.hpp  # 书架方块
├── ShulkerBoxBlock.cpp  # 模块接口与实现
├── ShulkerBoxBlock.hpp  # 潜影盒方块
├── TrappedChestBlock.cpp  # 模块接口与实现
└── TrappedChestBlock.hpp  # 陷阱箱方块
```

## 内部模块关系

Block 定义共享行为入口，BlockState 维护不可变状态；BlockProperties 与方块族描述在注册时绑定。注册表持有方块和状态，原版装配位于 registry/vanilla。

## 上下游外部依赖关系

实体、物品、区块和服务端世界消费方块接口；几何运算使用 common/physics，状态属性使用 common/util/property。

## 容易踩的坑

- 方块和状态由注册表持有，发放的引用必须保持稳定。
- 形状工厂显式区分 box 的归一化坐标和 fromPixelBox 的像素坐标。
- 声音事件标识、方块族规则、LevelEventId 与模拟 GameEvent 分别管理。
- 原版地图颜色策略在注册时绑定，公共模型不直接引用原版快照。
