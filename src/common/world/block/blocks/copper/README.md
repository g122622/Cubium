# 铜方块模块 (Copper Blocks)

本目录实现了 Minecraft 铜方块的氧化系统，包括所有可氧化铜变体和涂蜡铜变体。

## 架构概览

### 氧化接口

```
IOxidizableBlock (接口)
├── WeatheringCopperBlock (铜方块基类)
│   ├── WeatheringCopperGrateBlock (铜格栅)
│   ├── WeatheringCopperChainBlock (铜锁链)
│   ├── WeatheringCopperBarsBlock (铜栏杆)
│   ├── WeatheringCopperLanternBlock (铜灯笼)
│   └── CopperBulbBlock (铜灯)
├── WeatheringCopperStairBlock (铜楼梯)
├── WeatheringCopperSlabBlock (铜台阶)
├── WeatheringCopperDoorBlock (铜门)
├── WeatheringCopperTrapDoorBlock (铜活板门)
└── WeatheringLightningRodBlock (可氧化避雷针)
```

### IOxidizableBlock 接口

所有具有氧化等级的方块都实现 `IOxidizableBlock` 接口，提供统一的氧化识别和概率计算。

**核心方法：**

| 方法 | 说明 |
|------|------|
| `getOxidationLevel()` | 获取当前氧化等级 (Unaffected/Exposed/Weathered/Oxidized) |
| `getNextOxidationBlock()` | 获取下一氧化等级对应的方块，Oxidized 返回 nullptr |
| `getPreviousOxidationBlock()` | 获取上一氧化等级对应的方块，Unaffected 返回 nullptr（用于斧头刮削） |
| `getOxidationChanceModifier()` | 氧化概率修正系数，Unaffected=0.75，其余=1.0 |
| `tryOxidize()` | 执行完整的曼哈顿距离氧化算法 |

**氧化算法流程 (`IOxidizableBlock::tryOxidize()`)：**

1. 若已是最高氧化等级 (Oxidized)，直接返回
2. 获取下一氧化等级方块 (nullptr 则返回)
3. 外层门限概率：0.05688889 (~5.69%)
4. 扫描 4 格曼哈顿距离内的可氧化方块 (`dynamic_cast<IOxidizableBlock>`)
5. 若存在更低等级邻居 → 取消氧化
6. 计算最终概率：`f = (k+1)/(k+j+1)`, `f1 = f² × chanceModifier`
   - k = 更高等级邻居数
   - j = 同等级邻居数
7. 通过概率则替换为下一等级方块，使用 `withPropertiesOf()` 保留共有属性

### 继承方式

- **WeatheringCopperBlock** 继承 `Block` + `IOxidizableBlock`，作为简单铜方块的基类
- **WeatheringCopperStairBlock** 继承 `StairsBlock` + `IOxidizableBlock`
- **WeatheringCopperSlabBlock** 继承 `SlabBlock` + `IOxidizableBlock`
- **WeatheringCopperDoorBlock** 继承 `DoorBlock` + `IOxidizableBlock`（仅下半部分触发氧化，上半部分联动替换）
- **WeatheringCopperTrapDoorBlock** 继承 `TrapDoorBlock` + `IOxidizableBlock`

所有变体类在 `randomTick()` 中调用 `tryOxidize()` 执行氧化算法。Door 变体额外处理上半部分联动。

### 反向氧化链（斧头刮削）

每个可氧化铜方块（Unaffected 除外）都维护一个 `m_previousOxidationBlock` 指针，指向上一氧化等级的方块，形成反向氧化链：

```
Copper ← ExposedCopper ← WeatheredCopper ← OxidizedCopper
```

反向链在 `CopperBlocks.cpp` 注册时通过 `setPreviousOxidationBlock()` 设置，覆盖全部 12 种铜变体：
- 铜方块、切制铜、切制铜楼梯、切制铜台阶
- 铜门、铜活板门、铜格栅、铜灯
- 铜锁链、铜栏杆、凿纹铜、铜灯笼
- 避雷针（MC 1.21+ 新增氧化变种）

斧头刮削交互（`AxeItem::onItemUse`）通过 `IOxidizableBlock::getPreviousOxidationBlock()` 访问反向链，将铜方块降级到上一氧化等级（如 Exposed → Unaffected），并播放 SCRAPE 粒子效果。

### 属性保留

氧化替换方块时，`tryOxidize()` 使用 `StateHolder::withPropertiesOf()` 方法自动保留共有属性（如楼梯朝向、台阶类型、含水状态等），确保氧化不会丢失方块状态。斧头刮削同样使用 `withPropertiesOf()` 保留属性。

### 涂蜡变体

每种铜变体都有对应的涂蜡版本（`Waxed*` 类），涂蜡方块不实现 `IOxidizableBlock`，不会氧化。

## 文件说明

| 文件 | 说明 |
|------|------|
| `IOxidizableBlock.hpp/cpp` | 氧化接口，包含共享的曼哈顿距离氧化算法 |
| `WeatheringCopperBlock.hpp/cpp` | 基础可氧化铜方块 |
| `WeatheringCopperStairBlock.hpp/cpp` | 可氧化铜楼梯 |
| `WeatheringCopperSlabBlock.hpp/cpp` | 可氧化铜台阶 |
| `WeatheringCopperDoorBlock.hpp/cpp` | 可氧化铜门（上下半部分联动） |
| `WeatheringCopperTrapDoorBlock.hpp/cpp` | 可氧化铜活板门 |
| `WeatheringCopperGrateBlock.hpp/cpp` | 可氧化铜格栅（含水） |
| `WeatheringCopperChainBlock.hpp/cpp` | 可氧化铜锁链（含水，轴向放置） |
| `WeatheringCopperBarsBlock.hpp/cpp` | 可氧化铜栏杆（含水，四方向连接，BARS标签，skipRendering面剔除） |
| `WeatheringCopperLanternBlock.hpp/cpp` | 可氧化铜灯笼（含水，悬挂/站立） |
| `CopperBulbBlock.hpp/cpp` | 铜灯（红石控制，LIT/POWERED 状态） |
| `WeatheringLightningRodBlock.hpp/cpp` | 可氧化避雷针（方向性，红石信号输出，闪电吸引，含水支持） |
| `CopperGolemStatueBlock.hpp/cpp` | 铜傀儡雕像（1.21.11，4 种姿态，比较器输出 1-4，方块实体保存 CUSTOM_NAME） |
| `CopperChestBlock.hpp/cpp` | 铜箱子（1.21.11，27 格容器，双箱合并跨氧化等级/涂蜡状态，方块实体保留） |

### 避雷针氧化系统（MC 1.21+）

MC 1.21 为避雷针新增了氧化变种。避雷针的氧化架构与其他铜方块略有不同：

- **基础 `lightning_rod`**：使用普通 `LightningRodBlock`（不实现 `IOxidizableBlock`），相当于 Unaffected 等级，但不参与氧化 tick
- **`exposed_lightning_rod` / `weathered_lightning_rod` / `oxidized_lightning_rod`**：使用 `WeatheringLightningRodBlock`（继承 `LightningRodBlock` + `IOxidizableBlock`），可随机 tick 氧化
- **涂蜡变种**：使用 `WaxedLightningRodBlock`（继承 `LightningRodBlock`），不参与氧化

氧化链：`lightning_rod → exposed_lightning_rod → weathered_lightning_rod → oxidized_lightning_rod`

`WeatheringLightningRodBlock` 在构造函数中重建状态容器，添加 `OXIDATION` 属性到 `LightningRodBlock` 的 `FACING`、`POWERED`、`WATERLOGGED` 之上。涂蜡版本 `WaxedLightningRodBlock` 保持与基础 `LightningRodBlock` 相同的状态属性，不添加 `OXIDATION`。

### 铜傀儡雕像系统（MC 1.21.11）

铜傀儡雕像（`copper_golem_statue`）是 1.21.11 引入的装饰性方块，共 8 个变体（4 氧化等级 + 4 涂蜡）。它的架构与避雷针类似：

- **基础 `copper_golem_statue`**：使用 `CopperGolemStatueBlock`（不实现 `IOxidizableBlock`），处于氧化链的 Unaffected 位置但不参与氧化 tick
- **`exposed_copper_golem_statue` / `weathered_copper_golem_statue` / `oxidized_copper_golem_statue`**：使用 `WeatheringCopperGolemStatueBlock`（继承 `CopperGolemStatueBlock` + `IOxidizableBlock`），可随机 tick 氧化
- **涂蜡变种**：使用 `CopperGolemStatueBlock`（不氧化），与基础类相同但通过 `HoneycombItem` 涂蜡映射关联

**状态属性：**
- `HORIZONTAL_FACING`：朝向（北南东西，与玩家朝向相反）
- `COPPER_GOLEM_POSE`：姿态（Standing/Sitting/Running/Star，玩家右键循环切换）
- `WATERLOGGED`：是否含水
- `OXIDATION`：氧化等级（仅 `WeatheringCopperGolemStatueBlock`）

**核心行为：**
- 右键点击：持有斧头敲击基础（未涂蜡）`copper_golem_statue` 时生成铜傀儡（调用 `CopperGolemStatueBlockEntity::removeStatue()`）；持有斧头敲击涂蜡变体时返回 PASS（委托 `AxeItem::onItemUse` 处理除蜡）；否则循环切换 POSE 并播放 `BLOCK_COPPER_GOLEM_BECOME_STATUE` 音效
- 比较器输出：`POSE.ordinal() + 1`（范围 1-4）
- 碰撞形状：圆柱形 `box(3, 0, 3, 13, 14, 13)`（直径 10 像素，高度 14 像素），对应 MC Java `Block.column(10.0, 0.0, 14.0)`
- 方块实体：`CopperGolemStatueBlockEntity`，保存 `CUSTOM_NAME` 组件（用于铜傀儡转化时保留名称），并提供 `removeStatue()` 接口生成铜傀儡实体

**斧头生成铜傀儡：** 玩家用斧头右键敲击基础 `copper_golem_statue`（Unaffected 等级、未涂蜡）时，`CopperGolemStatueBlock::onBlockActivated` 调用 `CopperGolemStatueBlockEntity::removeStatue(state)` 生成铜傀儡实体（转移 `CUSTOM_NAME`、设置位置与朝向、播放生成音效），然后损坏斧头、将实体加入世界、移除方块。涂蜡变体返回 PASS 由 `AxeItem` 处理除蜡，Exposed+ 变体由 `WeatheringCopperGolemStatueBlock` 继承父类逻辑（涂蜡检测返回 PASS，由 `AxeItem` 处理刮削）。

氧化链：`copper_golem_statue → exposed_copper_golem_statue → weathered_copper_golem_statue → oxidized_copper_golem_statue`

涂蜡映射（4 组）通过 `HoneycombItem::_buildWaxablesMap()` 注册，斧头除蜡通过 `HoneycombItem::getWaxedOff()` 自动反向查找。

### 铜箱子系统（MC 1.21.11）

铜箱子（`copper_chest`）是 1.21.11 引入的容器方块，共 8 个变体（4 氧化等级 + 4 涂蜡）。它的架构与铜傀儡雕像类似但更复杂，因为需要保留方块实体中的物品：

- **基础 `copper_chest`**：使用 `CopperChestBlock`（不实现 `IOxidizableBlock`），处于氧化链的 Unaffected 位置但不参与氧化 tick
- **`exposed_copper_chest` / `weathered_copper_chest` / `oxidized_copper_chest`**：使用 `WeatheringCopperChestBlock`（继承 `CopperChestBlock` + `IOxidizableBlock`），可随机 tick 氧化
- **涂蜡变种**：使用 `WaxedCopperChestBlock`（继承 `CopperChestBlock`，重写 `isWaxed()` 返回 true），不氧化

**与铜傀儡雕像的关键差异：**

1. **方块状态属性**：铜箱子不使用 `OXIDATION` 方块状态属性，每个氧化等级是独立的方块类型（与 MC Java 1.21.11 一致）。`m_oxidationLevel` 成员变量仅用于双箱合并时比较氧化等级。

2. **方块实体保留**：铜箱子复用 `BlockEntityType::Chest` 与 `ChestEntity`（27 格物品存储）。氧化/涂蜡/除蜡/刮削导致方块类型变化时，通过 `shouldChangedStateKeepBlockEntity()` 返回 true 保留旧方块实体，避免物品丢失。`ServerWorld::setBlockState` 在检测到此方法返回 true 时，会迁移旧方块实体到新方块而非创建空实体。

3. **双箱合并跨氧化等级**：`chestCanConnectTo()` 检查 `COPPER_CHESTS` 标签，允许不同氧化等级与涂蜡状态的铜箱子合并为双箱。`getStateForPlacement()` 调用 `getLeastOxidizedChestOfConnectedBlocks`，在合并时取较低氧化等级（且优先未涂蜡）的方块类型作为合并后方块类型。`updatePostPlacement()` 在连接建立时同步方块类型。

4. **随机 tick 氧化条件**：`WeatheringCopperChestBlock.randomTick` 在以下情况不氧化：
   - 当前箱子是双箱的 RIGHT 部分（避免双箱两侧同时氧化导致不同步）
   - 箱子正在被玩家打开（`ChestEntity::getOpenCount() > 0`）

**状态属性：**
- `HORIZONTAL_FACING`：朝向（北南东西，与玩家朝向相反）
- `CHEST_TYPE`：箱子类型（SINGLE/LEFT/RIGHT）
- `WATERLOGGED`：是否含水

**核心行为：**
- 容量 27 格（单箱）/ 54 格（双箱），与普通箱子一致
- 玩家右键打开 GUI（`Generic9x3` 单箱 / `Generic9x6` 双箱），复用 `ChestEntity` 与 `StandaloneServer` 菜单工厂
- 红石比较器输出（与普通箱子一致，按物品占用率计算）
- 双箱合并时跨氧化等级与涂蜡状态连接（`COPPER_CHESTS` 标签）

氧化链：`copper_chest → exposed_copper_chest → weathered_copper_chest → oxidized_copper_chest`

涂蜡映射（4 组）通过 `HoneycombItem::_buildWaxablesMap()` 注册，斧头除蜡通过 `HoneycombItem::getWaxedOff()` 自动反向查找。
