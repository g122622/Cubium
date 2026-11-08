# 按主要行为组织的具体方块

## 目录结构树

```text
blocks/
├── agricultural/  # 该领域的子模块
├── air/  # 空气方块
├── base/  # 本领域共享的基础类型与定义
├── building/  # 该领域的子模块
├── cauldron/  # 炼药锅与内容物交互
├── cave/  # 该领域的子模块
├── container/  # 存储与运输方块行为
├── copper/  # 该领域的子模块
├── coral/  # 该领域的子模块
├── decorative/  # 该领域的子模块
├── dirt/  # 该领域的子模块
├── door/  # 双格门行为与铜门变种
├── enchanting/  # 附魔台与书架行为
├── end/  # 该领域的子模块
├── falling/  # 下落方块的专用行为
├── fencegate/  # 栅栏门交互与连接
├── fluid/  # 流体方块
├── functional/  # 该领域的子模块
├── furnace/  # 熔炉及冶炼方块行为
├── garden/  # 该领域的子模块
├── growing_plant/  # 该领域的子模块
├── ice/  # 该领域的子模块
├── mangrove/  # 该领域的子模块
├── mob/  # 该领域的子模块
├── multiface/  # 多面附着与扩散
├── nether/  # 该领域的子模块
├── ocean/  # 该领域的子模块
├── pale_garden/  # 该领域的子模块
├── redstone/  # 该领域的子模块
├── sculk/  # 该领域的子模块
├── sign/  # 普通及悬挂告示牌行为
├── special/  # 该领域的子模块
├── trapdoor/  # 活板门行为与铜变种
├── trial/  # 该领域的子模块
└── vegetation/  # 该领域的子模块
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

### 1. 液体等级映射错误

方块 LEVEL（0-15）与流体 LEVEL（1-8）的映射关系容易混淆。始终使用静态转换方法：

```cpp
i32 fluidLevel = LiquidBlock::blockLevelToFluidLevel(blockLevel);
i32 blockLevel = LiquidBlock::fluidLevelToBlockLevel(fluidLevel, falling);
```

### 2. 状态缓存生命周期

`LiquidBlock::m_fluidStateCache` 存储 `FluidState` 对象而非指针，因为 FluidState 可能被修改。不要缓存返回的 `FluidState*` 指针，每次都调用 `getFluidState()`。

### 3. AirBlock 的特殊性

AirBlock 的 `isSolid()` 返回 `false`，可能导致意外的碰撞检测通过。在碰撞检测时同时检查 `isAir()`：

```cpp
if (!state.isAir() && state.isSolid()) {
    // 执行碰撞检测
}
```

### 4. RotatedPillarBlock 的默认轴向

默认轴向是 `Axis::X`（枚举第一个值），但大多数原木默认应该是 `Axis::Y`。在注册时设置默认状态：

```cpp
auto& block = BlockRegistry::instance().registerBlock<RotatedPillarBlock>(...);
block.setDefaultState(block.withAxis(block.defaultState(), Axis::Y));
```

### 5. FallingBlock 火焰穿透机制

`canFallThrough()` 检查火焰标签（`BlockTags::FIRE`），下落过程中 `FireBlock::getCollisionShape()` 返回空碰撞形状，双重保障确保下落方块能穿透火焰。

### 6. 门方块的双方块结构

DoorBlock 使用 `HALF` 属性区分上下半部分，操作时需要同时处理两个方块位置。破坏、放置、红石控制都需要正确处理上下半部分的同步。

### 7. 炼药锅系统（三层架构）

炼药锅系统采用与 MC 原版一致的三类方块架构：

- **CauldronBlock**（空炼药锅）：无水位属性。接收降水（雨5%/雪10%）→ 替换为 LayeredCauldronBlock（水位1）；接收滴石水滴 → 替换为 LayeredCauldronBlock（水位1）；接收滴石岩浆滴 → 替换为 LavaCauldronBlock；水桶交互 → 替换为 LayeredCauldronBlock（水位3）；岩浆桶交互 → 替换为 LavaCauldronBlock；水瓶交互 → 替换为 LayeredCauldronBlock（水位1）。`isEmpty()` 始终返回 true，`isFull()` 始终返回 false。`getEntityInsideCollisionShape()` 返回 `fullCube()`。

- **LayeredCauldronBlock**（分层炼药锅/水炼药锅）：使用 `LEVEL_1_3` 属性存储水位（1-3）。空桶取水（仅满水位）→ 替换为空 CauldronBlock；玻璃瓶取水 → 降低水位（`lowerFillLevel`）；水瓶倒水 → 增加水位；水桶倒水 → 水位变3；皮革/旗帜清洗 → 降低水位；实体灭火 → 降低水位；降水匹配时增加水位；滴石水滴增加水位（仅水炼药锅）。`lowerFillLevel()` 水位降至0时替换为空 CauldronBlock。对应 MC 原版 `LayeredCauldronBlock`。

- **LavaCauldronBlock**（岩浆炼药锅）：始终满，无水位属性。空桶提取岩浆 → 替换为空 CauldronBlock。实体进入受岩浆伤害。`canReceiveStalactiteDrip` 返回 false。

滴石滴水通过 `tick()` → `findStalactiteTipAboveCauldron()` → `getCauldronFillFluidType()` → `canReceiveStalactiteDrip()` → `receiveStalactiteDrip()` 链路填充炼药锅。

### 8. TrappedChestBlock 红石信号与双箱支持

`TrappedChestBlock` 重写了 `createBlockEntity()` 返回 `TrappedChestEntity`（而非继承自 `ChestBlock` 的默认 `ChestEntity`）。红石信号通过 `TrappedChestEntity::getRedstoneSignal(world)` 计算，该方法会自动聚合双箱两侧的打开玩家数，信号强度 = 打开玩家总数（最大15）。`TrappedChestBlock::getWeakPower()` 调用此方法而非直接读取 `openCount`，确保双陷阱箱的红石信号正确。强充能仅从顶面输出（`getStrongPower` 仅 `Direction::Up` 返回有效信号）。

### 9. 附魔台与书架的距离通知机制

附魔台检测书架的范围是水平2格、垂直0-1格（MC的BOOKSHELF_OFFSETS，共30个候选位置）。而 `ServerWorld::setBlockState` 的 `neighborChanged` 只通知1格距离的直接邻居。因此书架放置/移除时，`BookshelfBlock` 必须主动扫描5x3x5范围（dx∈[-2,2], dy∈[-1,1], dz∈[-2,2]）通知附近的附魔台重新计算附魔力量。附魔台自身的 `onBlockAdded` 会调度1tick延迟（因为方块实体在 `onBlockAdded` 之后才创建），在 `tick` 中完成首次附魔力量计算。

### 10. 书架方块使用 BlockTags 而非硬编码

`BookshelfBlock` 和 `EnchantingTableEntity` 使用 `BlockTags::ENCHANTMENT_POWER_PROVIDER` 标签判断书架（而非硬编码 `VanillaBlocks::BOOKSHELF` 指针），使用 `BlockState::canBeReplaced()` 判断中间方块是否可穿透（对应MC的 `ENCHANTMENT_POWER_TRANSMITTER` 标签）。这意味着任何被添加到 `minecraft:enchantment_power_provider` 标签的方块都会增强附魔力量。

### 11. 潜影盒的「打开判定盒」不能与方块本体重叠

`ShulkerBoxBlock::canOpen()` 用 `world.getBlockCollisions(getOpenBoundingBox(pos, facing)).empty()`
判定能否打开，而方块本体一定是实心的——**只要判定盒与本体重叠，`canOpen()` 就恒为 false，
潜影盒永远打不开**（表现为右键无反应且无任何日志，只会播一个「上锁」音效）。
`getOpenBoundingBox` 的语义是「本体朝 facing 方向外扩 0.5 格的那一块」：外扩只能移动朝向那一面，
随后把反方向那一面推到本体另一侧；不要「两侧各外扩 0.5 再收缩 1.0」（收缩量必须扣掉那次外扩，
否则留下 0.5 格重叠）。
