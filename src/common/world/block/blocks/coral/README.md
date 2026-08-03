# 珊瑚方块模块 (Coral Blocks)

水下珊瑚类方块的实现，包括珊瑚、珊瑚扇、墙珊瑚扇和珊瑚块。

## 目录结构

```
coral/
├── README.md           # 本文档
├── CoralBlock.hpp/cpp  # 珊瑚方块和珊瑚扇类定义（CoralBlock、CoralFanBlock、CoralWallFanBlock、CoralBlockBlock）
```

## 内部模块关系

```
CoralColor (枚举)
    ↓ 共享
┌───────────────────────────────────────────────────────────┐
│  CoralBlock          → 活珊瑚（含水，离水死亡）            │
│  CoralFanBlock       → 珊瑚扇（地面放置，需附着面）        │
│  CoralWallFanBlock   → 墙珊瑚扇（墙面放置）                │
│  CoralBlockBlock     → 珊瑚块（固体，不会死亡）            │
└───────────────────────────────────────────────────────────┘
    ↓ 都实现（除CoralBlockBlock）
IWaterLoggable 接口
```

**类继承关系**：
- `CoralBlock` → `Block`, `IWaterLoggable`
- `CoralFanBlock` → `Block`, `IWaterLoggable`
- `CoralWallFanBlock` → `Block`, `IWaterLoggable`
- `CoralBlockBlock` → `Block`（无含水功能）

**珊瑚死亡机制**：
- `CoralBlock`、`CoralFanBlock`、`CoralWallFanBlock` 在 `updatePostPlacement()` 中检测周围是否有水
- 无水时转换为对应的死珊瑚方块（通过 `m_deadBlock` ID 查找）

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/IWaterLoggable` | 含水方块接口 |
| `world/block/Material` | 材质系统（`Material::CORAL`） |
| `world/block/WaterLoggableHelpers` | 含水工具函数（`shouldWaterlogAt`、`scheduleWaterTick`） |
| `util/property/Properties` | 方块属性（`WATERLOGGED`、`HORIZONTAL_FACING`、`FACING`） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/Direction` | 方向枚举 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/NaturalBlocks` | 注册所有珊瑚方块（5种颜色 × 4种类型 = 20种活珊瑚 + 20种死珊瑚） |
| `world/block/BlockTags` | `WALL_CORALS` 标签（10种墙珊瑚扇）、`UNDERWATER_BONEMEALS` 标签（5种珊瑚扇） |
| `item/BoneMealItem` | 水下骨粉可能使用珊瑚扇 |

## 容易踩的坑

### 1. 珊瑚死亡检测需要周围6个方向都有水

`hasNearbyWater()` 检测上下左右前后6个方向的流体状态，只要有一个方向有水就不会死亡。不仅是自身 `WATERLOGGED` 状态。

### 2. 死珊瑚是不同的方块类型

活珊瑚和死珊瑚是不同的方块实例，通过构造函数传入的 `m_deadBlock` ID 关联。珊瑚死亡时返回死珊瑚的 `BlockState`，而不是修改自身状态。

### 3. 珊瑚扇需要有效附着面

`CoralFanBlock` 和 `CoralWallFanBlock` 在 `isValidPosition()` 中检查附着面是否有效。如果附着面失效（如被移除），方块会在 `updatePostPlacement()` 中变成空气。

### 4. 墙珊瑚扇只能附着水平面

`CoralWallFanBlock` 的 `facing` 属性用 `BlockStateProperties::HORIZONTAL_FACING()`
（仅 north/south/east/west 4 向），对齐 vanilla 1.21.11 `coral_wall_fan` 的 8 状态
（4 facing × waterlogged）。容器本身不含 up/down，故 `getStateForPlacement` 对点击
地面/天花板的 up/down 退化为 North（随后 `isValidPosition` 因对面无支撑方块而拒绝放置）。
`isValidPosition` 检查 `facing` 反方向是否有坚固墙面可附着。

**注意**：`HORIZONTAL_FACING` 的 `with()` 对 up/down 会抛 `std::invalid_argument`
（值不在允许集合），故放置逻辑必须先做水平化收窄，不能直接把任意 `getClickedFace()` 写入。

### 5. CoralBlockBlock 不实现含水功能

`CoralBlockBlock` 是固体珊瑚块，不会因缺水死亡，也不需要 `WATERLOGGED` 属性。它只有颜色属性，用于渲染。
