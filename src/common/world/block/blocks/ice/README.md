# 冰系方块模块

`ice/` 目录包含普通冰、浮冰、蓝冰、霜冰和雪层的实现。重点处理冰的融化、雪层融化掉落、挖掘后的替换，以及世界写入顺序相关的回调安全性。

## 目录结构

```text
ice/
├── IceBlock.hpp             # 声明 IceBlock、PackedIceBlock、BlueIceBlock、FrostedIceBlock 四种冰系方块
├── IceBlock.cpp             # 实现冰和霜冰的融化逻辑、玩家破坏后是否留水的判断
├── SnowBlock.hpp            # 声明 SnowBlock 雪层方块（1-8层）
├── SnowBlock.cpp            # 实现雪层融化并掉落雪球的逻辑
└── README.md
```

## 内部模块关系

- `IceBlock` 和 `FrostedIceBlock` 在高光照下会把自己替换成水或空气
- `PackedIceBlock` 与 `BlueIceBlock` 仅保留基础方块行为，不融化
- `SnowBlock` 在光照 > 11 时融化，掉落对应层数的雪球物品

## 上下游外部依赖关系

**本目录依赖：**
- `Block.hpp`、`BlockRegistry.hpp` - 方块基类和注册表
- `Fluid.hpp`、`FluidRegistry.hpp` - 流体系统（获取水源方块状态）
- `Items.hpp`、`ItemStack.hpp`、`ItemDropHelper.hpp` - 物品和掉落工具
- `IWorld.hpp`、`IRandom.hpp` - 世界接口和随机数接口
- `TickManager.hpp`、`TickPriority.hpp` - Tick 调度系统（霜冰使用）

**被依赖：**
- `VanillaBlocks.hpp` - 注册所有原版方块时引用
- 测试文件 - `tests/common/world/block/blocks/IceBlockTest.cpp` 等

## 容易踩的坑

- **不要在冰的随机刻里直接调用 `onBlockRemoved()`**：否则会把"融化"和"破坏后替换"混成同一条路径
- **同一坐标的替换必须先完成区块写入，再进入旧方块回调**：像冰块这种会再次写回自身的逻辑会触发递归。代码中通过 `s_skipIceReplacementCallback` 和 `IceReplacementGuard` 来避免递归
- **挖掘冰时的逻辑和融化时的逻辑不同**：前者要看下方支撑，后者只看维度与光照
- **雪层融化时掉落的雪球数量等于层数（1-8个）**
