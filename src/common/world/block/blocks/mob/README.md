# 生物相关方块模块 (Mob Blocks)

生物相关方块模块提供与生物交互的方块实现。

## 目录结构

```
mob/
├── BeehiveBlock.hpp/cpp      # 蜂巢/蜂箱方块（有方块实体）
├── TurtleEggBlock.hpp/cpp    # 海龟蛋方块（可孵化、可被踩破）
├── InfestedBlock.hpp/cpp     # 被感染方块（破坏时生成蠹虫）
├── SpawnerBlock.hpp/cpp      # 刷怪笼方块（有方块实体）
└── DragonBreathBlock.hpp/cpp # 龙息方块（纯视觉，无碰撞无伤害）
```

## 内部模块关系

```
Block (基类)
    ├── BeehiveBlock      → 需要 BeehiveBlockEntity（待实现蜜蜂存储）
    ├── TurtleEggBlock    → 状态属性: EGGS_1_4, HATCH_0_2
    ├── InfestedBlock     → 静态映射表管理虫蚀方块关系
    ├── SpawnerBlock      → 需要 SpawnerBlockEntity（待实现生物生成）
    └── DragonBreathBlock → 纯视觉方块，无碰撞无伤害；龙息伤害由 AreaEffectCloudEntity 处理
```

## 上下游外部依赖关系

**上游依赖（本模块使用的）：**
- `Block` / `BlockState` / `StateContainer` - 方块基类和状态系统
- `BlockStateProperties` - 属性定义（EGGS_1_4、HATCH_0_2、HONEY_LEVEL_0_5）
- `BlockTags` - 方块标签（SAND 用于海龟蛋放置检测）
- `IWorld` / `IBlockReader` - 世界接口
- `Entity` / `LivingEntity` - 实体基类（海龟蛋、蠹虫生成）
- `GameRules` - 游戏规则（mobGriefing）
- `SoundEvents` - 音效播放

**下游依赖（使用本模块的）：**
- `VanillaBlocks` - 注册所有方块实例
- `BlockRegistry` - 方块注册表
- `ItemStack` / `BlockItem` - 方块物品形式

## 容易踩的坑

### TurtleEggBlock
- **孵化条件**：`randomTick` 只在 `ticksRandomly() == true` 时被调用，确保方块注册时启用了随机 tick
- **沙子检测**：`isValidPosition` 和 `_hasProperHabitat` 都需要检查 `BlockTags::SAND`，不能硬编码方块 ID
- **孵化生成小海龟**：必须调用 `setChild(true)` 设置为幼体，并调用 `setHomePos(pos)` 设置出生位置
- **踩踏检查**：`_canTrample` 需要检查 `mobGriefing` 游戏规则，玩家例外（总是可踩破）
- **僵尸类特殊处理**：Zombie、Husk、Drowned 在 `onFallenUpon` 中不踩破蛋，但仍然调用父类 `Block::onFallenUpon` 施加普通摔落伤害
- **摔落伤害**：`onFallenUpon` 先执行踩破逻辑，再调用父类 `Block::onFallenUpon` 保留普通摔落伤害（对齐 MC 1.21 TurtleEggBlock.fallOn）

### InfestedBlock
- **映射表初始化**：必须在使用 `canContainSilverfish` 或 `infest` 前调用 `initializeMappings()`，由 `VanillaBlocks::initialize()` 自动调用
- **映射注册**：通过 `registerInfestedBlock()` 注册普通方块与虫蚀方块的映射关系
- **服务端检查**：蠹虫只在服务端生成，需要 `world.isClientSide()` 检查
- **蠹虫生成逻辑**：通过 `spawnAfterBreak` 实现，检查 `doTileDrops` 游戏规则和精准采集附魔；使用精准采集工具破坏时不生成蠹虫；爆炸破坏时（无工具）正常生成
- **粒子效果**：蠹虫出现时产生 `ParticleTypeId::Poof` 烟雾粒子效果（调用 `world.addParticle()`），提供视觉反馈
- **调用路径**：`BlockInteractionManager::handleBlockBreak` → `spawnAfterBreak(tool, true)` → `InfestedBlock::spawnAfterBreak`；爆炸路径 `Explosion::_destroyBlocks` → `spawnAfterBreak(nullptr, false)`；实体破坏路径（末影龙/凋灵/掠夺者/蠹虫苏醒/活塞/命令setblock/fill）均调用 `spawnAfterBreak(nullptr, false)`
- **不调用 spawnAfterBreak 的路径**：流体冲刷（WaterFluid/LavaFluid）、火焰烧毁（FireBlock）、/clone move 命令——MC Java 中这些路径不调用 spawnAfterBreak

### DragonBreathBlock
- **纯视觉方块**：不造成伤害，龙息伤害由 `AreaEffectCloudEntity`（由 `DragonFireballEntity` 生成）处理
- **无碰撞体积**：`getShape()` 和 `getCollisionShape()` 均返回空形状
- **非不透明**：`isOpaque()` 返回 `false`，允许光照穿过

### BeehiveBlock / SpawnerBlock
- **方块实体**：`hasBlockEntity()` 返回 `true`，需要配套的 BlockEntity 实现完整功能
