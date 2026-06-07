# 海洋方块模块 (Ocean Blocks)

海洋方块模块提供水下植物和装饰方块的实现。

## 目录结构

```
ocean/
├── SeaPickleBlock.hpp/cpp    # 海泡菜方块（可堆叠1-4个，水下发光）
├── KelpBlock.hpp/cpp         # 海带方块（可生长的水下植物，AGE_0_25）
├── SeagrassBlock.hpp/cpp     # 海草方块（单格水下植物，实现IGrowable接口）
├── TallSeagrassBlock.hpp/cpp # 高海草方块（双格水下植物，HALF属性）
├── BubbleColumnBlock.hpp/cpp # 气泡柱方块（推动实体，DRAG属性）
├── DriedKelpBlock.hpp/cpp    # 干海带块（装饰性方块，可作为燃料）
└── ConduitBlock.hpp/cpp      # 潮涌核心（水下信标，需要框架激活）
```

## 内部模块关系

```
┌─────────────────┐
│  SeagrassBlock  │ ──骨粉催熟──▶ ┌───────────────────┐
│   (单格海草)    │              │ TallSeagrassBlock │
└─────────────────┘              │    (双格海草)     │
        │                        └───────────────────┘
        │ 实现
        ▼
   IGrowable 接口

┌─────────────────┐  产生上推   ┌───────────────────┐
│    灵魂沙       │ ─────────▶ │ BubbleColumnBlock │
└─────────────────┘            │   (DRAG=false)    │
                               │                   │
┌─────────────────┐  产生下拖   │   向上传播机制    │
│    岩浆块       │ ─────────▶ │   (tick方法)      │
└─────────────────┘            └───────────────────┘
```

- **SeagrassBlock ↔ TallSeagrassBlock**：海草通过骨粉催熟变成高海草，两者通过 `VanillaBlocks::TALL_SEAGRASS` 关联
- **BubbleColumnBlock**：独立方块，由灵魂沙/岩浆块触发，通过静态方法 `placeBubbleColumn()` 和 `getDrag()` 与其他方块交互

## 上下游外部依赖关系

### 上游依赖

| 依赖模块 | 用途 |
|----------|------|
| `world/block/Block` | 方块基类 |
| `world/block/IGrowable` | 生长接口（SeagrassBlock） |
| `world/block/IWaterLoggable` | 含水接口（SeaPickleBlock） |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `world/fluid/FluidRegistry` | 流体注册表（获取水流体） |
| `world/fluid/FluidTags` | 流体标签（检查是否为水） |
| `util/property/Properties` | 方块属性（PICKLES_1_4, AGE_0_25, DRAG, DOUBLE_BLOCK_HALF等） |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `block/registry/VanillaBlocks` | 原版方块注册表（获取TALL_SEAGRASS等） |

### 下游依赖

| 依赖模块 | 用途 |
|----------|------|
| `block/registry/VanillaBlocks` | 注册海洋方块实例 |
| `world/gen/feature` | 世界生成特性（海带、海草生成） |
| `entity/Entity` | 实体交互（气泡柱推动实体） |
| `block/blocks/MagmaBlock` | 岩浆块触发气泡柱 |
| `block/blocks/SoulSandBlock` | 灵魂沙触发气泡柱 |

## 容易踩的坑

### 1. 水源方块检测

**问题**：海草、高海草放置时需要检测水源方块，容易误用流体等级判断。

**解决方案**：必须检查流体等级是否为 8（完整水源），同时检查流体标签是否为水：
```cpp
// 正确做法
if (fluidState->getLevel() != 8) return false;
if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) return false;
```

### 2. 高海草上下半部分关联

**问题**：高海草由上下两个方块组成，破坏下半部分时需要同时处理上半部分。

**解决方案**：`updatePostPlacement()` 中检查邻居状态，当下半部分失去上方支撑或上半部分失去下方连接时，正确处理方块移除。

### 3. 气泡柱 DRAG 状态继承

**问题**：气泡柱向上延伸时需要正确继承 DRAG 状态。

**解决方案**：使用 `BubbleColumnBlock::getDrag()` 静态方法，它会递归检查下方方块类型：
- 岩浆块 → `true`（下拖）
- 灵魂沙 → `false`（上推）
- 气泡柱 → 继承其 DRAG 状态

### 4. VanillaBlocks 依赖时序

**问题**：`SeagrassBlock::grow()` 中访问 `VanillaBlocks::TALL_SEAGRASS`，可能在方块注册完成前被调用。

**解决方案**：使用前检查指针是否为空：
```cpp
if (VanillaBlocks::TALL_SEAGRASS == nullptr) {
    return;
}
```

### 5. 海泡菜发光等级

**问题**：海泡菜只有在水中才发光，离开水不发光。

**解决方案**：`getLightLevel()` 需要检查 `WATERLOGGED` 属性，而非仅检查数量：
- 检查 `isWaterlogged(state)` 是否为 true
- 根据数量返回对应亮度（1=6, 2=9, 3=12, 4=15）

### 6. 海带生长上限

**问题**：海带通过 `AGE_0_25` 属性控制生长，最大值为 25，超出会导致状态无效。

**解决方案**：在 `randomTick()` 中检查年龄是否已达到上限再决定是否生长。
