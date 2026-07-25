# 车辆实体

本目录包含可乘坐的车辆实体。

## 目录结构

```
vehicle/
├── BoatEntity.hpp          # 船实体定义（水上交通工具，10种木材变体）
├── BoatEntity.cpp          # 船实体实现（浮力、控制、乘客管理）
├── ChestBoatEntity.hpp     # 箱子船实体定义（27格容器，INamedContainerProvider，战利品表延迟填充）
├── ChestBoatEntity.cpp     # 箱子船实体实现（容器交互、战利品表解包、物品掉落、NBT序列化）
├── MinecartEntity.hpp      # 矿车实体定义（基类 + 7种变体）
├── MinecartEntity.cpp      # 矿车实体实现（铁轨移动、变体逻辑）
└── README.md               # 本文档
```

## 内部模块关系

```
Entity (基类)
    │
    ├── BoatEntity
    │   └── 水上交通工具，不实现 IRideable（船不需要鞍）
    │       └── ChestBoatEntity  # 箱子船（27格容器，实现 INamedContainerProvider，最多1名乘客，支持战利品表延迟填充）
    │
    └── AbstractMinecartEntity
        ├── RideableMinecartEntity    # 普通矿车（可乘坐）
        ├── ChestMinecartEntity       # 箱子矿车（27格库存）
        ├── FurnaceMinecartEntity     # 熔炉矿车（燃料驱动）
        ├── TNTMinecartEntity         # TNT矿车（可点燃爆炸）
        ├── HopperMinecartEntity      # 漏斗矿车（物品收集，实现 IHopper）
        ├── CommandBlockMinecartEntity # 命令方块矿车
        └── SpawnerMinecartEntity     # 刷怪笼矿车（持有 SpawnerLogic，自动生成实体）
```

## 上下游外部依赖

### 上游依赖（本目录依赖的模块）

| 模块 | 用途 |
|------|------|
| `entity/core/Entity.hpp` | 实体基类 |
| `entity/core/BoostHelper.hpp` | 加速辅助类（鞍和加速状态管理） |
| `entity/interfaces/IRideable.hpp` | 可骑乘接口（船不实现，供其他骑乘实体用） |
| `entity/damage/DamageSource.hpp` | 伤害源系统 |
| `world/gamerule/GameRules.hpp` | 游戏规则（tntExplodes 控制 TNT 矿车引爆/爆炸） |
| `world/block/blocks/redstone/AbstractRailBlock.hpp` | 铁轨形状定义 |
| `world/blockentity/transport/IHopper.hpp` | 漏斗接口（HopperMinecartEntity 实现） |
| `world/blockentity/core/SimpleInventory.hpp` | 简单库存实现 |

### 下游依赖（依赖本目录的模块）

| 模块 | 用途 |
|------|------|
| `entity/registry/VanillaEntities.hpp` | 实体类型注册 |
| `item/items/vehicle/BoatItem.hpp` | 放置船的物品 |
| `item/items/vehicle/MinecartItem.hpp` | 放置矿车的物品 |
| `world/block/dispense/` | 发射器行为（放置车辆） |
| `client/renderer/trident/entity/renderer/vehicle/` | 车辆渲染器 |
| `server/network/ServerPlayRouter.cpp` | 骑乘/载具网络包处理（PaddleBoat/MoveVehicle/SteerBoat 分支，替代已删除的 PacketHandler） |

## 容易踩的坑

### 1. 船与 IRideable 接口

**问题**：船不实现 `IRideable` 接口，因为船不需要鞍即可骑乘，也不支持加速功能。

**解决方案**：船直接使用 `Entity` 的乘客系统，通过 `handleInput()` 处理玩家控制。

### 2. 矿车速度限制

**要点**：
- 铁轨最大速度由 `max_minecart_speed` 游戏规则控制（默认值 8，范围 [1, 1000]）
- 实际速度 = 规则值 / 20.0，水中减半。默认 8 / 20.0 = 0.4 方块/刻
- 熔炉矿车：`getMaxSpeed()` 返回 0.2（覆盖铁轨最大速度限制，但实际仍受 `max_minecart_speed` 影响）
- 空中横向速度：0.4 (DEFAULT_MAX_SPEED_AIR_LATERAL)

### 3. TNT矿车伤害处理

**问题**：TNT矿车对不同伤害类型有不同反应，处理逻辑复杂。

**要点**：
- 火焰/爆炸伤害 → 点燃TNT（不掉落物品）
- 普通伤害 + 低速度(<0.01) → 掉落矿车 + TNT方块
- 普通伤害 + 高速度(≥0.01) → 碰撞爆炸
- 燃烧箭矢 → 根据箭矢速度计算爆炸威力
- `_ignite()` 和 `_explode()` 均受 `tntExplodes` 游戏规则控制：当 `tntExplodes=false` 时，`_ignite()` 不点燃（直接返回），`_explode()` 不创建爆炸（若已点燃则丢弃实体）

参考 MC 1.21.11：`MinecartTNT.primeFuse()`、`MinecartTNT.explode()`、`MinecartTNT.damageSourceIgnitesTnt()`

### 3.5 TNT矿车引燃同步与归因

**问题**：TNT矿车引燃时需要记录引爆来源，爆炸伤害需正确归因到引爆者。

**要点**：
- **引燃来源**：`_ignite(const DamageSource* source)` 接受可选的 DamageSource 参数，首次点燃时记录到 `m_ignitionSource`，后续不覆盖
- **归因转换**：`m_ignitionSource` 为 `IndirectEntityDamageSource(Explosion, causeEntity, this)`，其中 `causeEntity` 是原始伤害的造成者，`this` 是 TNT 矿车自身
- **爆炸传递**：`_explode()` 将 `m_ignitionSource` 传递给 `createExplosionWithSource()`，使爆炸伤害正确归因
- **伤害判断**：`_damageSourceIgnitesTnt()` 判断伤害源是否能点燃TNT（着火投射物 / IS_FIRE / IS_EXPLOSION）
- **shouldSourceDestroy**：`hurt()` 中当 `_damageSourceIgnitesTnt()` 返回 true 时，即使伤害未超过阈值也触发 `dropItem()`
- **服务端**：`_ignite()` 设置 `m_fuse = 80`，调用 `broadcastEntityStatus(EatBlock)` 通知客户端，调用 `playSound(ENTITY_TNT_PRIMED)` 播放音效
- **网络**：实体状态经 IR `ir::play::EntityEvent` 传输，`network::EntityStatus::EatBlock(10)` 此状态码被羊吃草和TNT矿车引燃共用
- **客户端**：`ClientPlayVisitor` 的 `onEntityStatus` 回调根据 `entityType() == VanillaEntityTypeKeys::TNT_MINECART` 区分处理：TNT矿车调用 `setFuseTimer(80)`，羊调用 `setEatAnimationTimer(40)`
- `Entity::playSound()` 自动检查 `isSilent()`，无需手动判断
- 修改引燃或新增实体状态处理时，三端（服务端实体、网络包、客户端回调）必须同步更新

### 4. 熔炉矿车掉落逻辑

**问题**：熔炉矿车的掉落逻辑与伤害类型相关。

**要点**：
- 爆炸伤害 → 只掉落矿车
- 非爆炸伤害 → 掉落矿车 + 熔炉方块（通过 `BlockItemRegistry` 查询 `VanillaBlocks::FURNACE` 对应的 BlockItem）

**与 MC 1.21.11 的差异**：MC 1.21.11 `MinecartFurnace` 不覆写 `destroy()`，只掉落 `FURNACE_MINECART` 物品，不再掉落熔炉方块。本项目保留 MC 1.16.5 `FurnaceMinecartEntity.killMinecart()` 的行为（非爆炸时额外掉落熔炉方块），此为有意偏差。

### 5. 漏斗矿车红石禁用

**要点**：
- 漏斗矿车在充能的激活铁轨上暂停工作，`onActivatorRailPass(powered=true)` 设置 `m_disabled=true`
- 离开激活铁轨后保持当前状态不变，必须经过未充能的激活铁轨才重新启用
- 激活铁轨回调由基类 `_moveAlongTrack()` 统一调用，无需子类手动轮询铁轨状态

### 6. 矿车斜坡高度调整

**问题**：矿车在斜坡上的高度调整需要精确计算。

**要点**：
- 斜坡调整值：`SLOPE_ADJUSTMENT = 0.0078125` (1/128)
- 斜坡上升：Y + 0.0625 或 Y + 0.5625

### 7. 伤害源参数传递

**问题**：`dropItem()` 方法现在需要 `DamageSource*` 参数。

**解决方案**：在 `hurt()` 方法中调用 `dropItem(&source)` 传递伤害源，用于判断掉落逻辑。非伤害导致的销毁传 `nullptr`。

### 8. 船的乘客限制

**问题**：船最多承载2名乘客，但水下状态的船不能承载乘客。

**解决方案**：`canAddPassenger()` 覆写检查 `m_passengers.size() < MAX_PASSENGERS && m_status != BoatStatus::UnderWater`。基类 `couldAcceptPassenger()` 未覆写（返回 true），乘客准入由 `canAddPassenger()` 控制。

### 8.5. 船的地面滑度采样

**要点**：
- `getBoatGlide()` 对应 MC Java `AbstractBoat.getGroundFriction()`，实现船在陆地上的摩擦力采样算法
- 采样范围：船底碰撞箱向下扩展 0.001 格的薄碰撞盒区域
- 搜索范围：碰撞盒各方向扩展 1 格，排除角落 4 个柱状区域
- 跳过睡莲方块（`NaturalBlocks::LILY_PAD`），不参与滑度采样
- 仅采样碰撞箱与船底有交集的方块
- 返回值：所有有效方块滑度的平均值（`Block::getSlipperiness()`），无有效方块时返回 0
- 冰（0.98）和蓝冰（0.989）提供更高的滑度，使船在冰面上滑行更远
- `updateMotion()` 中陆地摩擦力减半的条件检查控制乘客是否为 Player（通过 `getControllingPassenger()` + `entityType() == VanillaEntityTypeKeys::PLAYER`）

### 9. 铁轨形状与移动

**问题**：铁轨有10种形状，需要正确处理每种形状的移动方向。

**参考**：`AbstractRailBlock` 定义了所有 `RailShape`，`_getRailDirectionVectors()` 计算方向向量。

### 10. 网络同步

**问题**：骑乘相关网络包需要在正确的时机发送。

**要点**（均走 IR `ir::play::*`）：
- `ir::play::PlayerInput`：客户端→服务端，发送玩家输入（骑乘/转向）
- `ir::play::ServerboundMoveVehicle`：客户端→服务端，发送载具位置
- `ir::play::ClientboundMoveVehicle`：服务端→客户端，校正载具位置
- `ir::play::SetPassengers`：服务端→客户端，同步乘客列表

### 11. 矿车比较器信号

**问题**：部分矿车需要通过 `getComparatorOutput()` 向红石比较器输出信号。

**要点**：
- `ChestMinecartEntity`：重写 `getComparatorOutput()`，使用 `RedstoneHelper::calcRedstoneFromInventory()` 计算容器填充信号（0-15）
- `HopperMinecartEntity`：重写 `getComparatorOutput()`，使用 `RedstoneHelper::calcRedstoneFromInventory()` 计算容器填充信号（0-15）
- `CommandBlockMinecartEntity`：重写 `getComparatorOutput()`，返回 `m_successCount`；通过 `setSuccessCount(i32)` 设置成功计数
- `RideableMinecartEntity`、`FurnaceMinecartEntity`、`TNTMinecartEntity`：不重写，返回默认值 0
- `DetectorRailBlock::getComparatorInputOverride()` 通过 `RedstoneHelper::getEntitySignal()` 查询矿车信号，优先级：CommandBlockMinecart > 容器矿车 > 普通矿车（返回 0）

### 12. 游戏规则 doEntityDrops 对车辆掉落的影响

**问题**：所有车辆实体的 `dropItem()` 方法都受 `GameRuleKeys::DO_ENTITY_DROPS` 游戏规则控制。

**要点**：
- `AbstractMinecartEntity::dropItem()`：当 `DO_ENTITY_DROPS` 为 false 时，直接调用 `remove()` 返回，不产生任何掉落物
- `ChestMinecartEntity::dropItem()`：容器内容物掉落受 `DO_ENTITY_DROPS` 控制，然后调用父类方法（父类也会检查该规则）
- `FurnaceMinecartEntity::dropItem()`：熔炉方块额外掉落受 `DO_ENTITY_DROPS` 控制（且仅非爆炸伤害时掉落）
- `TNTMinecartEntity::dropItem()`：TNT 方块掉落受 `DO_ENTITY_DROPS` 控制
- `HopperMinecartEntity::dropItem()`：容器内容物掉落受 `DO_ENTITY_DROPS` 控制，然后调用父类方法
- `BoatEntity::dropItem()`：当 `DO_ENTITY_DROPS` 为 false 时，直接返回，不掉落船物品

参考 MC 1.21.11：`VehicleEntity.destroy()` 中的 `ENTITY_DROPS` 检查

### 13. 刷怪笼矿车（SpawnerMinecartEntity）

**要点**：
- 持有 `SpawnerLogic` 实例（对应 MC Java 的 `BaseSpawner`），与 `MobSpawnerBlockEntity` 共享生成逻辑
- 刷怪笼矿车被摧毁时**不会掉落任何物品**（既不掉矿车也不掉刷怪笼方块），与 MC Java 一致
- 刷怪笼矿车**没有对应物品**，只能通过 `/summon` 命令生成
- 矿车内部显示刷怪笼方块（`DATA_SHOW_BLOCK_PARAM = true`）
- 服务端 tick 执行生成逻辑，成功生成后通过 `broadcastEntityStatus(id, 1)` 广播粒子事件
- 客户端 tick 更新旋转动画（`SpawnerLogic::clientTick()`）
- 支持 NBT 序列化/反序列化，保存所有刷怪笼参数
- 刷怪笼矿车**不响应激活铁轨**（无 `onActivatorRailPass` 重写）
- 刷怪笼矿车**无比较器输出**（`getComparatorOutput()` 返回默认值 0）

参考 MC 1.21.11：`MinecartSpawner`

### 14. 箱子船战利品表延迟填充（ChestBoatEntity LootTable）

**要点**：
- 箱子船支持战利品表延迟填充：结构生成时设置 `setLootTable(id, seed)`，玩家首次打开容器时解包生成物品
- 对应 MC Java 的 `ContainerEntity.unpackChestVehicleLootTable()` 模式
- **懒解包**：`getInventoryItem()`、`setInventoryItem()`、`removeInventoryItem()`、`removeInventoryItemNoUpdate()`、`clearInventory()` 等容器访问方法会在操作前调用 `unpackLootTable(nullptr)` 确保物品已生成
- **旁观者守卫**：`createMenu()` 中，若玩家为旁观者且存在未解包战利品表，返回 `nullptr`（防止旁观者触发战利品生成）
- **NBT 序列化**：有未解包战利品表时只保存 `LootTable`/`LootTableSeed`，不保存 `Items`；已解包后保存 `Items`，不保存战利品表引用
- **`isInventoryEmpty()`**：有未解包战利品表时返回 `false`（容器可能有物品，但尚未填充），`m_lootFilled` 标志追踪解包状态
- **`unpackLootTable(Player*)`**：从 `LootTableManager` 获取战利品表，使用 `LootParameterSets::chest()` 构建 `LootContext`，支持玩家幸运值和 `THIS_ENTITY` 参数，生成物品后按堆叠优先填充空槽位
- **掉落**：`dropInventoryContents()` 和 `remove()` 在掉落前调用 `unpackLootTable(nullptr)` 确保物品已生成
