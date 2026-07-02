# 装饰性方块模块

提供玻璃板、地毯、灯笼、染色玻璃等装饰性方块的实现。

## 目录结构

```
decorative/
├── PaneBlock.hpp/cpp           # 玻璃板/铁栏杆基类（连接逻辑、含水支持、skipRendering面剔除）
├── StainedGlassBlock.hpp/cpp   # 染色玻璃（信标光束颜色提供者）
├── CarpetBlock.hpp/cpp         # 地毯（单层高度、需支撑）
├── GlazedTerracottaBlock.hpp/cpp # 釉面陶瓦（可旋转、不可被活塞拉动）
├── FlowerPotBlock.hpp/cpp      # 花盆（可容纳植物内容）
├── LanternBlock.hpp/cpp        # 灯笼（悬挂/站立、含水支持）
├── ChainBlock.hpp/cpp          # 锁链（轴向放置、含水支持，MC 1.21+ 注册为 iron_chain）
├── LadderBlock.hpp/cpp         # 梯子（攀爬、需背面支撑）
├── ScaffoldingBlock.hpp/cpp    # 脚手架（攀爬、距离支撑计算、含水支持）
├── CampfireBlock.hpp/cpp       # 营火（烹饪、光照、信号火、含水支持）
├── BannerBlock.hpp/cpp         # 旗帜（站立式+墙壁式、含水支持）
├── TorchBlock.hpp/cpp          # 火把（地面放置、火焰/烟雾粒子）
├── WallTorchBlock.hpp/cpp      # 墙上火把（墙面附着、方向碰撞箱、偏移粒子）
├── AbstractCandleBlock.hpp/cpp # 蜡烛抽象基类（点燃/熄灭、投掷物交互、粒子动画）
├── CandleBlock.hpp/cpp         # 蜡烛（1-4根堆叠、含水支持、亮度随数量增长）
└── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      Block (基类)                            │
└─────────────────────────────────────────────────────────────┘
                              │
       ┌──────────────────────┼──────────────────────┐
       │                      │                      │
       ▼                      ▼                      ▼
IWaterLoggable        IBeaconBeamColorProvider   (无接口)
       │                      │                      │
       ├──────────────────────┼──────────────────────┤
       │                      │                      │
       ▼                      ▼                      ▼
┌─────────────┐      ┌─────────────────┐    ┌───────────────┐
│ PaneBlock   │      │ StainedGlassBlock│   │ CarpetBlock   │
│ LanternBlock│      └─────────────────┘    │ FlowerPotBlock│
│ ChainBlock  │                               │ GlazedTerraco │
│ LadderBlock │                               │ ttaBlock      │
│ ScaffoldBlock│                              └───────────────┘
│ CampfireBlock│
│ BannerBlock  │
│ CandleBlock  │─── IWaterLoggable
└──────┬───────┘
       │
┌──────┴──────────────────────────────────────────────────────┐
│ AbstractCandleBlock ← Block（抽象基类：点燃/熄灭、投掷物、粒子）│
│ CandleBlock ← AbstractCandleBlock + IWaterLoggable            │
└─────────────────────────────────────────────────────────────┘
```

- **PaneBlock**: 玻璃板/铁栏杆共享连接逻辑，形状按4位连接掩码缓存为16种组合；重写 `skipRendering` 实现同类方块和 BARS 标签方块之间的面剔除
- **BannerBlock**: 抽象基类 `AbstractBannerBlock` 派生 `StandingBannerBlock`（16方向旋转）和 `WallBannerBlock`（4方向水平朝向）
- **CampfireBlock**: 派生 `SoulCampfireBlock`（灵魂营火，光照10 vs 普通15）
- **TorchBlock/WallTorchBlock**: 继承关系，WallTorchBlock 继承 TorchBlock 添加 HORIZONTAL_FACING 属性；普通火把用 Flame 粒子，灵魂火把用 SoulFireFlame 粒子
- **AbstractCandleBlock/CandleBlock**: 继承关系，AbstractCandleBlock 提供点燃/熄灭（extinguish/setLit）、投掷物点燃（onProjectileHit）、粒子动画（animateTick）等共享逻辑；CandleBlock 继承 AbstractCandleBlock 并实现 IWaterLoggable，增加 CANDLES(1-4)/LIT/WATERLOGGED 三种状态属性，支持堆叠放置和含水自动熄灭

## 上下游外部依赖关系

### 上游依赖（本模块依赖）
- `world/block/Block.hpp` - 方块基类
- `world/block/BlockState.hpp` - 方块状态
- `world/block/BlockProperties.hpp` - 方块属性构建器
- `world/block/BlockStateProperties.hpp` - 标准方块属性（FACING, WATERLOGGED, LIT 等）
- `world/block/IWaterLoggable.hpp` - 含水接口
- `world/block/IBeaconBeamColorProvider.hpp` - 信标光束颜色接口
- `world/block/Material.hpp` - 材料定义
- `world/block/VanillaBlocks.hpp` - 空气方块常量
- `physics/collision/CollisionShape.hpp` - 碰撞形状
- `util/property/Properties.hpp` - 属性系统
- `util/color/DyeColor.hpp` - 染料颜色枚举
- `blockentity/BlockEntity.hpp` - 方块实体基类
- `blockentity/interactive/BannerEntity.hpp` - 旗帜方块实体
- `entity/utils/ItemDropHelper.hpp` - 物品掉落工具（ScaffoldingBlock 使用）

### 下游依赖（依赖本模块）
- `world/block/VanillaBlocks.hpp` - 注册所有方块实例
- `world/block/BlockRegistry.hpp` - 方块注册表
- `blockentity/BlockEntityRegistry.hpp` - 方块实体注册（Banner、Campfire）
- `item/Item.hpp` - 对应物品（BannerItem 等处理站立/墙壁放置）

## 容易踩的坑

### ScaffoldingBlock 距离计算
- **坑**: `distance == 7` 时脚手架掉落，但需要区分"新变成7"和"已经是7"两种情况
- **解**: 新变成7时创建 `FallingBlockEntity`，已经是7时直接掉落物品
- **参考**: MC 1.16.5 `ScaffoldingBlock.tick()`

### CampfireBlock 熄灭逻辑
- **坑**: 营火 **没有** AGE 属性，也 **不会** 因雨天而熄灭
- **解**: 熄灭方式只有三种：水接触、铲子右键、喷溅型水瓶
- **参考**: MC 1.16.5 `CampfireBlock`

### BannerBlock 放置逻辑
- **坑**: 站立式旗帜根据玩家yaw计算旋转值：`floor((180 + yaw) * 16 / 360 + 0.5) & 15`
- **解**: 不是简单的yaw/22.5，需要正确的四舍五入和取模

### PaneBlock 连接判定与面剔除
- **坑**: 连接判定不仅检查同类Pane，还要检查墙方块和有实体面的方块
- **解**: 使用 `shouldConnectTo()` 方法统一处理连接逻辑
- **面剔除**: `skipRendering()` 实现 BARS 标签方块间的面剔除逻辑——同类方块垂直方向始终跳过，水平方向仅双方都连接时跳过；不同 BARS 方块（如铁栏杆↔铜栏杆）水平方向双向连接时跳过

### LanternBlock 支撑检测
- **坑**: 悬挂状态检查上方方块的实体底面，站立状态检查下方方块的实体顶面
- **解**: 支撑方块移除时通过 `updatePostPlacement()` 自动变为空气

### StainedGlassBlock 光束混合
- **坑**: 信标光束穿过染色玻璃时，颜色是**平均混合**而非覆盖
- **解**: 混合算法 `newColor = (currentColor + blockColor) / 2.0`

### IWaterLoggable 实现
- **坑**: 所有含水方块必须在 `updatePostPlacement` 中处理水流体状态变化
- **解**: 调用 `getFluidState()` 返回水状态，并在邻居更新时调度水 tick

### TorchBlock/WallTorchBlock 放置与粒子
- **坑**: 墙上火把的粒子位置需要根据 FACING 偏移，否则粒子出现在方块中心
- **解**: 粒子 X/Z 偏移 `0.27 * oppositeDir.stepX/stepZ`，Y 偏移 `0.22`；火把放置验证用 `isSolidSide` 检查附着面
- **坑**: TorchBlock 的 `isValidPosition` 检查下方方块的上表面是否坚固，`updatePostPlacement` 仅在 `Direction::Down` 变化时触发

### AbstractCandleBlock/CandleBlock 蜡烛堆叠与含水

- **坑**: CandleBlock 的 `isReplaceable` 逻辑是堆叠放置的核心——玩家未潜行且手持同类蜡烛物品时，`isReplaceable` 返回 true 允许在已有蜡烛上堆叠；潜行时返回 false，正常放置新方块
- **坑**: 含水蜡烛无法点燃（`canBeLit` 检查 `!WATERLOGGED`）；含水蜡烛被水 tick 时自动熄灭（`tick` 中调用 `extinguish`）
- **坑**: `getStateForPlacement` 中堆叠逻辑需要检查目标位置已有方块是否为同类型 `CandleBlock`，而非只检查 `isReplaceable`——两个条件缺一不可
- **坑**: `getParticleOffsets` 返回的偏移位置列表必须与蜡烛数量对应（1-4个偏移位置），否则点燃粒子位置错误
- **坑**: CandleBlock 亮度公式 `3 * CANDLES`（1根=3, 2根=6, 3根=9, 4根=12），不是固定亮度
- **参考**: MC 1.17 `CandleBlock`
