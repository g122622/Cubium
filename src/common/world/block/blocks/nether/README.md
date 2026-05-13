# 下界方块模块 (Nether Blocks)

下界方块模块提供下界相关方块的实现。

## 目录结构

```
nether/
├── README.md              # 本文档
├── FireBlock.hpp/cpp      # 普通火焰方块
├── SoulFireBlock.hpp/cpp  # 灵魂火焰方块（蓝色火焰）
├── NetherPortalBlock.hpp/cpp  # 下界传送门方块
├── NetherWartBlock.hpp/cpp    # 下界疣方块
├── NyliumBlock.hpp/cpp    # 绯红/诡异菌岩方块
└── MagmaBlock.hpp/cpp     # 岩浆块方块
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `FireBlock` | 普通火焰，可蔓延 | AGE_0_15, NORTH/SOUTH/EAST/WEST/UP |
| `SoulFireBlock` | 灵魂火焰（蓝色，更高伤害） | 同 FireBlock |
| `NetherPortalBlock` | 下界传送门 | HORIZONTAL_AXIS |
| `NetherWartBlock` | 下界疣（可生长） | AGE_0_3 |
| `NyliumBlock` | 绯红/诡异菌岩 | 无 |
| `MagmaBlock` | 岩浆块 | 无 |

## 核心机制

### 火焰碰撞伤害（MC 1.16.5 对齐）

当实体与火焰方块碰撞时，`FireBlock::onEntityCollision` 执行以下逻辑：

1. **火焰免疫检查**：调用 `entity.isImmuneToFire()` 检查实体是否免疫火焰
   - 免疫实体（烈焰人、恶魂、岩浆怪、猪灵等）跳过所有火焰效果

2. **火焰计时器递增**：`entity.forceFireTicks(entity.getFireTimer() + 1)`
   - 每次碰撞 tick 都增加计时器

3. **点燃实体**：当 `fireTimer == 0` 时调用 `entity.setFire(8)`
   - 设置燃烧 8 秒（160 ticks）
   - `setFire` 方法只在当前值较小时更新

4. **造成伤害**：对 `LivingEntity` 造成 `m_fireDamage` 点火焰伤害
   - 普通火焰：1.0 伤害
   - 灵魂火：2.0 伤害（继承自 FireBlock，构造时传入 2）
   - 使用 `DamageSources::inFire()` 创建伤害源

```cpp
void FireBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    if (entity.isImmuneToFire()) {
        return;
    }
    entity.forceFireTicks(entity.getFireTimer() + 1);
    if (entity.getFireTimer() == 0) {
        entity.setFire(8);
    }
    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity != nullptr) {
        livingEntity->hurt(DamageSources::inFire(), static_cast<f32>(m_fireDamage));
    }
}
```

### 火焰蔓延

火焰蔓延机制已完整实现，参考 MC 1.16.5 `FireBlock.tick()` 和 `FireBlock.trySpread()`。

#### 火焰年龄

- 火焰有年龄（AGE_0_15），范围 0-15
- 年龄越大越稳定，越不容易熄灭
- 每次 randomTick 有概率增加年龄

#### 火焰 Tick 逻辑

```cpp
void FireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 1. 检查位置有效性
    // 2. 检查游戏规则 doFireTick
    // 3. 检查是否为无限火源（如下界岩）
    // 4. 下雨熄灭检查
    // 5. 火焰年龄增长
    // 6. 无可燃邻居时检查支撑
    // 7. 尝试蔓延
}
```

#### 蔓延算法

**直接相邻燃烧**（6个方向）：
- 垂直方向：chance = 250 + humidityPenalty
- 水平方向：chance = 300 + humidityPenalty
- 点燃概率：`(flammability / chance) * (5 / (age + 10))`

**远距离蔓延**（3x6 区域）：
- 范围：x: -1~1, z: -1~1, y: -1~4
- 蔓延概率：`(encouragement + 40 + difficulty * 7) / (age + 30)`
- 高度惩罚：每向上一层 +100

**环境因素**：
- 下雨：增加熄灭概率，降低蔓延概率 50%
- 难度：影响蔓延速度（Peaceful=0, Easy=7, Normal=14, Hard=21）
- 高湿度：蔓延概率减半

#### 核心方法

| 方法 | 功能 |
|------|------|
| `canBurn()` | 检查周围是否有可燃方块 |
| `trySpread()` | 尝试蔓延到周围方块 |
| `canDie()` | 检查是否会被雨淋灭 |
| `canCatchFire()` | 检查指定位置是否可被点燃 |
| `tryCatchFire()` | 尝试点燃指定位置 |
| `getNeighborEncouragement()` | 获取周围火焰蔓延加速值 |
| `areNeighborsFlammable()` | 检查周围是否有可燃方块 |

#### 火焰信息注册表

`FireInfoRegistry` 管理所有方块的燃烧参数：

```cpp
struct FireInfo {
    i32 encouragement;  // 火焰蔓延速度
    i32 flammability;   // 可燃性 (0-300)
};
```

部分方块燃烧参数：

| 方块 | encouragement | flammability |
|------|--------------|--------------|
| 木板/栅栏/楼梯 | 5 | 20 |
| 原木 | 5 | 5 |
| 树叶 | 30 | 60 |
| 羊毛 | 30 | 60 |
| TNT | 15 | 100 |
| 藤蔓 | 15 | 100 |
| 草/花 | 60 | 100 |

### 灵魂火系统

**灵魂火特性**：
- 只能在灵魂沙（soul_sand）或灵魂土（soul_soil）上方存在
- 伤害更高（2点，普通火为1点）
- 光照等级较低（10，普通火为15）
- 不会蔓延燃烧其他方块

**放置逻辑**：
1. `FlintAndSteelItem::getFireForPlacement()` 检查目标位置下方方块
2. 如果下方方块在 `BlockTags::SOUL_FIRE_BASE_BLOCKS()` 标签中，返回 `SOUL_FIRE`
3. 否则返回普通 `FIRE`

**灵魂火基座方块**（`SOUL_FIRE_BASE_BLOCKS` 标签）：
- `minecraft:soul_sand` - 灵魂沙
- `minecraft:soul_soil` - 灵魂土

**关键方法**：
```cpp
// 检查方块是否可作为灵魂火基座
bool SoulFireBlock::isSoulFireBase(const Block* block);

// 检查灵魂火是否可以放置在指定位置
bool SoulFireBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const;

// 当下方不再是灵魂基座时移除火焰
BlockState SoulFireBlock::updatePostPlacement(...);
```

### 下界传送门

1. 由黑曜石框架组成
2. 通过点火激活
3. 实体碰撞后传送
4. 水平轴向（X 或 Z）

### 下界疣生长

1. 只能种在灵魂沙上
2. 4个生长阶段（AGE_0_3）
3. 随机 tick 生长

### 菌岩退化

绯红菌岩和诡异菌岩在光照过亮时会退化为下界岩：

```cpp
void NyliumBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    if (!isDarkEnough(world, pos, state)) {
        world.setBlockState(pos, &VanillaBlocks::NETHERRACK->defaultState());
    }
}
```

### 岩浆块

岩浆块在水中会产生气泡柱：

```cpp
void MagmaBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() &&
            fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 生成气泡柱
            // BubbleColumnBlock.placeBubbleColumn(world, abovePos, true);
        }
    }
}
```

## 使用方法

```cpp
// 创建火焰
auto fire = std::make_unique<FireBlock>(
    BlockProperties(Materials::FIRE)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(15)
);

// 创建灵魂火
auto soulFire = std::make_unique<SoulFireBlock>(
    BlockProperties(Materials::FIRE)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(10)
);

// 检查是否可放置灵魂火
const BlockState* belowState = world.getBlockState(pos.down());
if (SoulFireBlock::isSoulFireBase(belowState->getBlock())) {
    // 可以放置灵魂火
    world.setBlockState(pos, &VanillaBlocks::SOUL_FIRE->defaultState(), 11);
}

// 检查方块是否在灵魂火基座标签中
if (BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*belowState)) {
    // 方块是灵魂沙或灵魂土
}

// 创建下界传送门
auto portal = std::make_unique<NetherPortalBlock>(
    BlockProperties(Materials::PORTAL)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(11)
);

// 创建下界疣
auto netherWart = std::make_unique<NetherWartBlock>(
    BlockProperties(Materials::PLANTS)
        .hardness(0.0f)
        .noCollision()
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/block/BlockTags` | 方块标签系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |

## 测试

测试文件：`tests/common/world/block/blocks/SoulFireBlockTest.cpp`

测试覆盖：
- `SOUL_FIRE_BASE_BLOCKS` 标签包含 `soul_sand` 和 `soul_soil`
- `SoulFireBlock::isSoulFireBase()` 方法
- `SoulFireBlock::isValidPosition()` 在不同基座上的行为
- `FIRE` 标签包含普通火和灵魂火
