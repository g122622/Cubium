#红石方块(Redstone Blocks)

红石方块模块提供所有红石相关方块的实现。

##目录结构

``` redstone /
├── RedstoneBlock.hpp / cpp #红石块（恒定15强度信号源）
├── RedstoneTorchBlock.hpp / cpp #红石火把（信号反转器）
├── RedstoneWallTorchBlock.hpp / cpp #墙上红石火把
├── RedstoneWireBlock.hpp / cpp #红石线（信号传输，每格衰减1）
├── RedstoneDiodeBlock.hpp / cpp #红石二极管基类（单向传输）
├── RedstoneRepeaterBlock.hpp / cpp #红石中继器（增强 + 延迟 +
            锁定）
├── RedstoneComparatorBlock.hpp / cpp #红石比较器（比较 / 减法模式，支持容器和物品展示框检测）
├── ObserverBlock.hpp / cpp #侦测器（方块变化检测，2tick脉冲）
├── AbstractButtonBlock.hpp / cpp #按钮基类（瞬时信号源）
├── StoneButtonBlock.hpp / cpp #石头按钮（10tick脉冲）
├── WoodButtonBlock.hpp / cpp #木按钮（15tick脉冲）
├── LeverBlock.hpp / cpp #拉杆（持久信号源）
├── AbstractPressurePlateBlock.hpp / cpp #压力板基类（实体检测信号源）
├── StonePressurePlateBlock.hpp / cpp #石头压力板（仅生物触发）
├── WoodPressurePlateBlock.hpp / cpp #木压力板（所有实体触发）
├── WeightedPressurePlateBlock.hpp / cpp #测重压力板（实体数量→信号强度）
├── DaylightDetectorBlock.hpp / cpp #日光探测器（天空亮度→信号）
├── PistonBlock.hpp / cpp #活塞（推动 / 拉回方块）
├── PistonStructureHelper.hpp / cpp #活塞推动结构计算器
├── PistonHeadBlock.hpp / cpp #活塞头（存活检查、基座关联、级联销毁）
├── MovingPistonBlock.hpp / cpp #移动中的活塞（动画代理）
├── DispenserBlock.hpp / cpp #发射器（含方块实体支持，红石触发发射物品）
├── DropperBlock.hpp / cpp #投掷器（继承发射器，简化投掷行为，含方块实体支持）
├── TripWireBlock.hpp / cpp #绊线（实体穿越检测，潜行不触发）
├── TripWireHookBlock.hpp / cpp #绊线钩
├── NoteBlock.hpp / cpp #音符盒（16种乐器，25个音高）
├── TNTBlock.hpp / cpp #TNT（红石 / 火焰触发爆炸，受 tntExplodes 游戏规则控制）
├── TargetBlock.hpp / cpp #标靶（箭矢命中→信号）
├── RedstoneLampBlock.hpp / cpp #红石灯（信号控制发光）
├── AbstractRailBlock.hpp / cpp #铁轨基类
├── RailState.hpp / cpp #铁轨连接状态计算器（形状计算、三连接道岔）
├── RailBlock.hpp / cpp #普通铁轨
├── PoweredRailBlock.hpp / cpp #动力铁轨
├── DetectorRailBlock.hpp / cpp #探测铁轨
└── ActivatorRailBlock.hpp /
                cpp #激活铁轨
```

                ##内部模块关系

```mermaid flowchart TB subgraph Core["红石核心"] RS[RedstoneSystem] RP[RedstonePower] RC[RedstoneContext] end

                    subgraph Blocks["红石方块"] Signal["信号源方块"] Process["信号处理方块"] Output["输出方块"] end

                        subgraph World["世界系统"] IWorld[IWorld] Tick[TickSystem] BE[BlockEntity] end

                            Signal-- >
    | "提供信号" | RS Process-- > | "处理信号" | RS RS-- > | "更新状态" | Output RS-- > | "调度tick" | Tick RS-- > |
    "查询" | RP RS-- > | "递归保护" | RC Blocks-- > | "方块状态" | IWorld Output-- > | "创建" |
    BE
```

    ##上下游外部依赖关系

    ## #上游依赖（本目录依赖的模块）

    | 模块 | 用途 | | -- -- --| -- -- --| | `world / redstone / RedstoneSystem` | 红石系统核心，信号传播管理 |
    | `world / redstone / RedstonePower` | 红石信号查询工具 | | `world / redstone / RedstoneContext` | 递归保护上下文 |
    | `world / tick / TickPriority` | tick 优先级 | | `world / blockentity / BlockEntity` | 方块实体基类 |
    | `world / blockentity / redstone / PistonBlockEntity` | 活塞方块实体 | | `world / IWorld` | 世界接口 |
    | `item / BlockItemUseContext` | 放置上下文 | | `util / property / Properties` | 方块属性 |

    ## #下游依赖（依赖本目录的模块）

    | 模块 | 用途 | | -- -- --| -- -- --| | `world / World` | 红石方块的世界交互 | | `block / BlockRegistry` |
    方块注册 | | `blockentity /` | 方块实体创建 |

    ##容易踩的坑

    ## #1. 红石火把无限递归

        红石火把更新可能触发反馈循环，必须使用 `RedstoneContext` 或 `RedstoneSystem` 跟踪更新位置：

```cpp auto& redstone = RedstoneSystem::instance();
if (!redstone.isUpdating(pos)) {
    redstone.beginUpdate(pos);
    redstone.updateNeighbors(world, pos, block);
    redstone.endUpdate(pos);
}
```

    ## #2. 中继器更新顺序

        中继器面向另一个中继器时，更新顺序影响结果，必须使用正确的 `TickPriority`： -
    面向另一个中继器：`TickPriority::ExtremelyHigh` - 当前已充能：`TickPriority::VeryHigh` -
    其他情况：`TickPriority::High`

    ## #3. 红石线连接状态

        忘记更新红石线的连接状态会导致信号传输错误，必须在 `updatePostPlacement` 中重新计算连接。

    ## #4. 强弱信号混淆

    - **强信号**：直接从方块输出（`getStrongPower`），只向输出方向 -
    **弱信号**：通过方块传导（`getWeakPower`），向所有方向

        ## #5. 按钮和拉杆支撑检测

            按钮 /
        拉杆在支撑方块被移除后不会自动掉落，必须在 `neighborChanged` 中检测支撑方块是否还存在。

        ## #6. 侦测器脉冲时长

        侦测器脉冲持续* *
        2 tick**，使用 `PULSE_DURATION` 常量。

        ## #7. 墙上红石火把方向

`HORIZONTAL_FACING` 指向火把朝向的方向，输出信号时需要** 排除该方向**（不向附着面输出）。

        ## #8. 活塞推动链检测

        活塞推动时必须限制最大推动距离为* *
        12 格**，超过则推动失败。

        ## #9. 活塞收回时的方块实体

            活塞收回时方块实体可能丢失，必须使用 `MovingPistonBlock` 作为动画代理。

        ## #9b. 活塞破坏方块时的 spawnAfterBreak

            活塞推出时如果前方的方块无法被推动（如实体方块在推动路径末端），该方块会被破坏。破坏后调用 `spawnAfterBreak(
                world,
                pos,
                *destroyState,
                nullptr,
                false)`，使得虫蚀方块等特殊方块能正确触发生成逻辑（如蠹虫）。调用顺序：先 `setBlockState(nullptr)` 移除方块，再调用 `spawnAfterBreak`，与
        MC Java 行为一致。

        ## #10. 信号源强
        /
        弱信号区分

        按钮、拉杆等信号源必须正确区分：
    - 强信号只向输出方向输出 -
    弱信号向所有方向输出

    ## #11. 绊线潜行玩家

        ** 潜行的玩家不会触发绊线**，使用 `entity
            ->isSneaking()` 检测。

    ## #12. 压力板实体过滤

    压力板检测实体时使用不同的过滤策略，与 MC Java 对齐：

    - **石质/磨制黑石压力板**（MOBS 灵敏度）：使用 `dynamic_cast<LivingEntity*>` 过滤，
      只检测生物实体。同时排除 `doesEntityNotTriggerPressurePlate()` 返回 true 的实体
      （蝙蝠、标记模式盔甲架、不祥物品生成器等）。
    - **木质/铜/铁/金等压力板**（EVERYTHING 灵敏度）：检测所有实体类型，
      但排除 `doesEntityNotTriggerPressurePlate()` 返回 true 的实体。
    - **测重压力板**：与木质压力板相同，检测所有实体并排除不触发的实体。

    `doesEntityNotTriggerPressurePlate()` 覆写情况（对应 MC Java 的 `isIgnoringBlockTriggers()`）：
    - 蝙蝠（BatEntity）：返回 `true`
    - 标记模式盔甲架（ArmorStandEntity, isMarker=true）：返回 `true`
    - 不祥物品生成器（OminousItemSpawnerEntity）：返回 `true`
    - 其他实体默认返回 `false`

    注意：物品实体和投射物**不**覆写此方法。在 MC 原版中，木质/测重压力板可检测
    所有实体（包括物品），石质压力板通过 LivingEntity 类型过滤自动排除非生物实体。

    ## #13. 比较器物品展示框检测

    比较器检测物品展示框时：
    - 物品展示框朝向必须与比较器朝向相同 - 信号强度 = rotation + 1（范围 1 - 8） -
    只有唯一一个物品展示框时才返回信号

    ## #14. 红石线形状缓存

    红石线形状根据连接状态动态计算，使用 `std::unordered_map<u32, const CollisionShape*>` 缓存组合形状。

    ## #15. 压力板形状变化

    压力板有两种形状：
    - 未按下(power = 0)：高度 1 像素 -
    按下(power > 0)：高度 0.5 像素

    ## #15b. 压力板持久化信号模型（对齐 vanilla 1.21.11）

    vanilla 木/石压力板持久化 `powered` 布尔（按下=true/松开=false），
    测重压力板持久化 `power` 0-15。项目基类 `AbstractPressurePlateBlock`
    通过两个虚函数统一这两种持久化模型：

    - `getStoredSignal(state)`：读出持久化信号强度。基类（木/石）返回
      `isPowered(state) ? 15 : 0`；测重子类 `WeightedPressurePlateBlock`
      覆写为读 `POWER_0_15` 的真实 0-15 值。
    - `withStoredSignal(state, signal)`：写入信号。基类写 `POWERED = (signal>0)`；
      测重子类覆写为写 `POWER_0_15 = signal`。

    基类 `tick`/`updateState`/`getWeakPower`/`getShape`/`onEntityCollision` 全部走
    这两个虚函数，因此木/石与测重共用同一份 tick 逻辑而持久化属性不同。
    `WeightedPressurePlateBlock` 构造函数用 `POWER_0_15` 容器覆盖基类的 `POWERED`
    容器（与 `SoulFireBlock` 用空容器覆盖 `FireBlock` 容器同一手法），使测重压力板
    16 状态与 vanilla 一致以通过 `JavaBlockStateIdMap` 映射。

    注意：静态 `isPowered`/`withPowered`（POWERED 布尔）仅基类及木/石子类可用，
    测重子类状态不含 `POWERED`，故一切读写必须走 `getStoredSignal`/`withStoredSignal`。

    ## #16. 绊线形状变化

    绊线根据 ATTACHED 属性有不同形状：
    - 绷紧(ATTACHED = true)：悬浮在空中 -
    松弛(ATTACHED = false)：下垂

    ## #17. 音符盒乐器映射

    音符盒根据下方方块材质决定乐器类型，必须使用 `NoteBlockInstrument.byState` 进行映射。

    ## #18. TNT 方块与 tntExplodes 游戏规则

    TNT 方块的关键方法均受 `tntExplodes` 游戏规则控制：

    - **`prime()`*
        *（静态方法）：对应 MC Java 的 `TntBlock.prime()`，仅生成点燃的 TNT
         实体、播放音效、发出 `PRIME_FUSE` 游戏事件，** 不移除方块**。调用方需自行负责移除方块。当 `tntExplodes =
                                                          false` 时返回 `false`。 -
    **`ignite()`* *：调用 `prime()` 成功后移除 TNT 方块（`setBlockState(pos, nullptr, 11)`）。是 `prime() +
    移除方块` 的便捷组合。返回值为 `[[nodiscard]] bool`，调用方需根据返回值决定后续行为。带 `LivingEntity
        *` 参数的重载版本会将点燃者信息传递给 TNT 实体。
    - **`explode()`**：当 `tntExplodes = false` 时仅移除方块（不创建爆炸效果），与 MC Java 行为一致。 -
    **`onBlockExploded()`**：当 `tntExplodes = false` 时不生成连锁 TNT 实体（对应 MC Java 的 `wasExploded()`）。 -
    **`onBlockActivated()`* *：玩家使用打火石 /
        火焰弹右键点燃
        TNT。先调用 `prime()`，成功后移除方块并消耗物品。打火石消耗耐久度，火焰弹消耗一个物品（创造模式不消耗）。`tntExplodes =
                                                   false` 时显示 action bar 消息 "block.minecraft.tnt.disabled"。 -
    **`onProjectileHit()`*
        *：燃烧投掷物命中 TNT 时点燃。从 ProjectileEntity 获取发射者作为点燃者。先调用 `prime()`，成功后移除方块。
    - **`playerWillDestroy()`**：玩家破坏不稳定 TNT（UNSTABLE = true）时自动点燃，创造模式例外。 *
        *只调用 `prime()`，不移除方块 * *（方块移除由破坏流程处理，避免双重移除）。
    -
    **`canDropFromExplosion()`*
        *：返回 `false`，TNT 被爆炸摧毁时不掉落物品（对应 MC Java 的 `dropFromExplosion` 返回 `false`）。

        * *`prime()` 与 `ignite()` 的区别 * *（对应 MC Java 设计）：
    - `prime()`：仅生成实体 + 音效
    + 事件， * *调用方自行处理方块移除 * *。适用于 `playerWillDestroy`（破坏流程移除方块）。 - `ignite()`：`prime()` +
    移除方块。适用于 `onBlockAdded`、`neighborChanged`、`onBlockActivated`、`onProjectileHit` 等场景。

    此外：
    - `_hasFlammableNeighbor()` 检测相邻火焰 / 熔岩，通过 `onBlockAdded()` 和 `neighborChanged()` 间接调用。
    - `onBlockExploded()` 在 `Explosion::_destroyBlocks()` 中被调用，当其他爆炸摧毁 TNT 方块时生成短引信 TNT
    实体（引信公式：`nextInt(fuse / 4) +
    fuse /
        8`，即[10, 29] ticks）。连锁 TNT 的 owner 通过 `explosion->getIndirectSourceEntity()` 追溯爆炸源链。

        ## #19. TNT 火焰检测

        TNT 需要检测相邻位置的火焰（包括灵魂火）和熔岩，不仅检测红石信号。

        ## #20. 活塞头存活检查与级联销毁

        活塞头（PistonHeadBlock）不是独立方块，必须依赖已伸出的活塞基座才能存活：

    - **存活条件 * *（`isValidPosition`）：反方向有匹配的已伸出活塞基座（类型 + EXTENDED
    + FACING 三重匹配），或反方向是方向匹配的 MOVING_PISTON -
    **自动消失 * *（`updatePostPlacement`）：当活塞基座消失或收回时，活塞头收到更新后返回空气状态 -
    **级联销毁 * *（`onBlockRemoved`）：活塞头被移除时，检查反方向是否有匹配的已伸出活塞基座，如有则销毁基座并生成掉落物
    - **创造模式级联销毁 * *（`playerWillDestroy`）：玩家在创造模式破坏活塞头时，同时销毁匹配的活塞基座但不产生掉落物 -
    **通知转发 * *（`neighborChanged`）：活塞头存活时将邻居变化通知转发到活塞基座方向，确保红石信号能传导到活塞 -
    **推动反应 * *：活塞头的 `getPushReaction` 返回 `Block`，不能被活塞推动 -
    **类型匹配 *
        *：Normal 活塞头对应 PISTON，Sticky 活塞头对应 STICKY_PISTON，类型不匹配则无法存活

    ## #20b. 活塞 type 属性序列化（对齐 vanilla 1.21.11）

    `piston_head` 与 `moving_piston` 的 `type` 属性由 `EnumProperty<PistonHeadBlock::Type>`
    序列化，取值为 `"normal"`/`"sticky"`（vanilla 1.21.11 线格式）。C++ 枚举 `Type::Normal`/
    `Type::Sticky` 仅内部使用，序列化字符串必须用 `normal` 而非 `default`，否则
    `JavaBlockStateIdMap` 构造的 key（`piston_head|facing=...,short=...,type=default`）
    与 vanilla 表（`type=normal`）全部不匹配，导致 24+12 个状态全 miss。`MovingPistonBlock`
    复用 `PistonHeadBlock::getTypeProperty()`，故两方块共用同一份 Traits，修一处同生效。

        ## #21. 铁轨连接系统（RailState）

        铁轨的连接形状计算由 `RailState` 类负责，完整复刻了 MC Java 的 `RailState` 逻辑：

    - **三层级Y轴搜索 *
        *：`hasNeighborRail` 和 `getRail` 检查同层、上方(+1)、下方(
            -1) 三个Y层级，确保斜坡铁轨能正确发现和连接不同Y高度的相邻铁轨
    - ** 双向连接验证**：`removeSoftConnections` 验证对方铁轨是否仍然连接回来，`canConnectTo` 限制每条铁轨最多2个连接
    - ** 三连接道岔切换**：普通铁轨在三连接时根据红石信号选择弯轨方向（T型道岔的核心逻辑）
    - 有红石信号：优先级 SE → SW → NE → NW（最后匹配胜出） - 无红石信号：优先级 NW → NE → SW → SE（反方向）
    - 通过 `RailBlock::updateState` 在邻居信号源变化且铁轨有三连接时触发重算
    - ** 四连接处理**：保持当前形状，红石信号决定弯轨方向
    - ** 直线铁轨**：动力铁轨、探测铁轨、激活铁轨（`isStraight = true`）不支持弯轨，三 / 四连接时保持当前直轨形状 -
    **连接传播 * *：`place()` 中 `updateBlock = true` 模式下会通知相邻铁轨更新连接 -
    **updateBlock 参数 * *： - `true`：放置铁轨时（`onBlockAdded`）使用，直接设置方块状态并传播连接
    - `false`：`updatePostPlacement` 时使用，仅返回计算后的状态，由调用方设置 -
    **放置流程 * *：`getStateForPlacement` 只返回基于玩家朝向的初始形状（南北
        / 东西），真正的连接计算在 `onBlockAdded` 中通过 `updateDir` 触发
    -
    **RailState 内部状态**：`m_state` 存储构造时传入的方块状态，用于 `withRailShape` 计算新状态，避免从世界中读取可能为空的方块状态

## #22. 铁轨含水功能（IWaterLoggable）

所有铁轨方块（RailBlock、PoweredRailBlock、DetectorRailBlock、ActivatorRailBlock）均通过 `AbstractRailBlock` 继承 `IWaterLoggable` 接口，支持在水下放置。这与 MC Java 的 `BaseRailBlock implements SimpleWaterloggedBlock` 行为一致。

### 方块状态属性

| 方块 | 状态属性 | WATERLOGGED 默认值 |
|------|---------|-------------------|
| RailBlock | SHAPE (10值) + WATERLOGGED | false |
| PoweredRailBlock | SHAPE (6值) + POWERED + WATERLOGGED | false |
| DetectorRailBlock | SHAPE (6值) + POWERED + WATERLOGGED | false |
| ActivatorRailBlock | SHAPE (6值) + POWERED + WATERLOGGED | false |

**SHAPE 属性取值集合**（对齐 vanilla 1.21.11）：

- `RailBlock` 用 `RailShapeProperty::create("shape")`，含全部 10 值
  （南北/东西直轨 + 4 斜坡 + 4 弯轨），普通铁轨支持弯轨与三连接道岔。
- `PoweredRailBlock`/`DetectorRailBlock`/`ActivatorRailBlock` 用
  `RailShapeProperty::createStraight("shape")`，仅 6 值（南北/东西直轨 + 4 斜坡），
  不含弯轨。vanilla 矿车铁轨 shape 也只有这 6 值，用 `createStraight` 限制取值集合
  以通过 `JavaBlockStateIdMap` 映射（否则会生成 4 个弯轨幽灵状态导致 miss）。
  枚举序列化（`EnumProperty<RailShape>::Traits`）对所有铁轨共用同一份 10 名表，
  `createStraight` 仅收窄允许值集合，不影响序列化字符串。

### 含水功能实现

- **getStateForPlacement**：检测放置位置是否有水源，如果有则设置 `WATERLOGGED=true`
- **updatePostPlacement**：当 `WATERLOGGED=true` 时调用 `waterloggable::scheduleWaterTick()` 调度流体 tick
- **getFluidState**：含水时返回水的 FluidState，否则返回默认空流体
- **isWaterlogged**：读取 `WATERLOGGED` 属性值
- **canContainFluid / receiveFluid / pickupFluid**：继承自 `IWaterLoggable` 默认实现

### 客户端/服务端区分

- **updateDir**：客户端（`isClientSide() == true`）直接返回原状态，不执行 RailState 计算
- **neighborChanged**：客户端跳过邻居更新处理，避免铁轨形状在客户端与服务端不同步

## #23. 红石交互方块的建造权限检查

红石比较器（RedstoneComparatorBlock）、红石中继器（RedstoneRepeaterBlock）和红石线（RedstoneWireBlock）的 `onBlockActivated` 方法中，已集成 `Player::mayBuild()` 权限检查。当玩家不具备建造权限时（冒险/旁观模式或 `allowEdit=false`），这些方块不会响应右键交互（返回 `ActionResultType::Pass`）。这与 MC Java 中对应方块的 `mayBuild()` 检查逻辑一致。
