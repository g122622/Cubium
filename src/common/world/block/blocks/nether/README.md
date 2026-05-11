# 下界方块模块 (Nether Blocks)

下界方块模块提供下界相关方块的实现。

## 目录结构

```
nether/
├── README.md           # 本文档
├── FireBlock.hpp/cpp   # 火、灵魂火、下界传送门、下界疣
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `FireBlock` | 普通火焰，可蔓延 | AGE_0_15, NORTH/SOUTH/EAST/WEST/UP |
| `SoulFireBlock` | 灵魂火焰（蓝色，更高伤害） | 同 FireBlock |
| `NetherPortalBlock` | 下界传送门 | HORIZONTAL_AXIS |
| `NetherWartBlock` | 下界疣（可生长） | AGE_0_3 |

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
1. 火焰有年龄（AGE_0_15）
2. 年龄越大越稳定，越不容易熄灭
3. 可以蔓延到周围可燃方块
4. 检查周围是否有可燃物

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
