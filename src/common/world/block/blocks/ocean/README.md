# 海洋方块模块 (Ocean Blocks)

海洋方块模块提供水下植物和装饰方块的实现。

## 目录结构

```
ocean/
├── README.md              # 本文档
├── SeaPickleBlock.hpp/cpp # 海泡菜方块
├── KelpBlock.hpp/cpp      # 海带方块
├── SeagrassBlock.hpp/cpp  # 海草方块
├── TallSeagrassBlock.hpp/cpp # 高海草方块
├── BubbleColumnBlock.hpp/cpp # 气泡柱方块
└── DriedKelpBlock.hpp/cpp # 干海带块和潮涌核心
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `SeaPickleBlock` | 海泡菜（可堆叠1-4个，水下发光） | PICKLES_1_4, WATERLOGGED |
| `KelpBlock` | 海带（可生长到很高） | AGE_0_25, WATERLOGGED |
| `SeagrassBlock` | 海草（单格水下植物） | 无 |
| `TallSeagrassBlock` | 高海草（双格水下植物） | HALF, WATERLOGGED |
| `BubbleColumnBlock` | 气泡柱（推动实体） | DRAG |
| `DriedKelpBlock` | 干海带块（装饰性方块） | 无 |
| `ConduitBlock` | 潮涌核心（水下信标） | WATERLOGGED, ACTIVE |

## 核心机制

### VanillaBlocks 注册

以下方块已在 `VanillaBlocks::registerNaturalBlocks()` 中注册并可被世界生成直接使用：

- `minecraft:sea_pickle`
- `minecraft:kelp`
- `minecraft:kelp_plant`
- `minecraft:seagrass`
- `minecraft:tall_seagrass`
- `minecraft:bubble_column`
- `minecraft:dried_kelp_block`
- `minecraft:conduit`

### 海草骨粉催熟 (MC 1.16.5 对齐)

`SeagrassBlock` 实现了 `IGrowable` 接口，支持骨粉催熟：

- **canGrow()**: 检查上方是否有水源方块（流体等级=8）
- **canUseBonemeal()**: 总是返回 true
- **grow()**: 将海草变成高海草（双格植物）

```cpp
// 骨粉对海草使用时会变成高海草
SeagrassBlock seagrass(...);
if (seagrass.canGrow(world, pos, state, false)) {
    seagrass.grow(world, random, pos, state);
    // 海草变成高海草（LOWER + UPPER 两部分）
}
```

### 海泡菜发光

- 在水中时发光
- 亮度随数量增加：1个=6, 2个=9, 3个=12, 4个=15
- 离开水不发光

### 海带生长

- 通过随机 tick 生长
- 高度限制基于 AGE_0_25 (最大 25 格)
- 只能在水中生长
- 生长概率约 14%

### 气泡柱 (MC 1.16.5 对齐)

- 灵魂沙产生上推气泡柱 (DRAG=false)
- 岩浆块产生下拖气泡柱 (DRAG=true)
- 推动实体:
  - 上推: 速度 +0.1 Y方向 (灵魂沙)
  - 下拖: 速度 -0.03 Y方向 (岩浆块)
- 重置摔落距离
- tick 传播: 上方是水时转换为气泡柱

## 使用方法

```cpp
// 创建海泡菜
auto seaPickle = std::make_unique<SeaPickleBlock>(
    BlockProperties(Materials::UNDERWATER_PLANT())
        .hardness(0.0f)
        .noCollision()
        .lightLevel(6)  // 基础亮度
);

// 创建海带
auto kelp = std::make_unique<KelpBlock>(
    BlockProperties(Materials::UNDERWATER_PLANT())
        .hardness(0.0f)
        .noCollision()
);

// 创建气泡柱
auto bubbleColumn = std::make_unique<BubbleColumnBlock>(
    BlockProperties(Materials::BUBBLE_COLUMN())
        .hardness(0.0f)
        .noCollision()
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
| `physics/collision/CollisionShape` | 碰撞形状 |
