本项目目前的实体系统代码量大概20W行，基于纯OOP（和java版mc非常类似），这不仅性能低，且不适配脚本系统和gametest系统中提供的某些api能力，因此想要全量迁移到ecs。

entt已经通过vcpkg安装，源码和文档在 D:\MiscProjects\entt 可供参考。

其中一个难点：ECS抛弃了OOP的继承体系，导致原本依赖继承的逻辑需要重构为组合式的组件系统，是否不利于代码复用且可能导致代码膨胀、不不易维护？

先看看本地MC基岩版的逆向代码，看看其是如何实现ecs的。由于我对这个需求还没有太多具体的想法，因此请你探索相关内容后，使用askuserquestion工具，与我进行多轮充分讨论，以便细化需求和明确难点。

---

## 决策结论（2026-08，多轮讨论后拍板）

经探索基岩版逆向代码（`E:\dev\MC\LeviLamina` 下 `mc/deps/ecs/`、`mc/world/actor/`、`mc/entity/components/`）与多轮讨论，确认**基岩版本身就是 ECS + OOP 混合架构，并非纯 ECS**：`Actor` 内嵌 `EntityContext`（ECS 数据/组件层），`EnderMan→Monster→Mob→Actor` 继承链靠 vftable 作 OOP 行为封装层（末影人搬方块/激怒等特化逻辑走子类虚函数）。因此最终架构定为**基岩版式混合**，不走纯 ECS 推翻继承壳的路线。

### 七项根本决策

1. **架构路线**：基岩版式混合（非纯 ECS）。保留 `Entity→LivingEntity→MobEntity→Player` 继承壳作句柄与行为层；状态数据逐步搬入 entt 组件；`Entity` 内嵌 `EntityContext`，`EntityOwnerComponent` 反向持 `unique_ptr<Entity>`。AI/序列化等低频逻辑暂留 OOP。
2. **迁移策略**：渐进并存。ECS 骨架与 OOP 并存，按子系统逐块迁移，每步可编译可运行，旧代码在该子系统迁移完才删。
3. **AI 系统**：保留 OOP（Goal/Brain/Navigator/Controller 约 5 万行不 ECS 化）。仅用 `AIComponent` 承载 `GoalSelector`，System 做 tick 调度。
4. **首要驱动**：性能 + 脚本/gametest API 适配 + 代码维护性，三者并重不排序。
5. **时序编排**：显式命名阶段（PreMovement/Movement/PostMovement/AiStep/Reset 等）编码业务时序；**阶段内**用 `entt::organizer` 做组件读写冲突检测与可选并行。OOP tick 包装成 `EntityLegacyTickSystem` 塞进 ECS 阶段序列。
6. **句柄体系**：`entt::entity`（自定义 `EntityId`，64 位 entity_type：entity 32 + version 32）作运行时句柄；网络/存档/跨 registry 持久场景用 `EntityUniqueIDComponent`（u64）作持久化投影。过渡期保留现有 `EntityInstanceId`(u64 永不复用)+graveyard 兼容。
7. **同步体系**：基岩版 SynchedData 中间层——组件字段 + 独立 SynchedData 中间层，解耦组件内存布局与网络协议。**首批不触动同步体系**，留待后续批次。

> 关于"OOP 继承重构为组合是否不利复用/导致膨胀"的疑虑：混合架构下继承壳保留，特化逻辑仍走虚函数，复用模式不变；ECS 只接管**状态数据层**（高频字段），行为层不强行拆 system，故不会膨胀。`m_builtIn.velocity->m_velocity.x` 这类长链访问是基岩版标准缓存指针访问（对齐 `Actor::getPosition()` 读 `mStateVectorComponent->mPos`），是终态而非过渡形态。

## 首批范围（已落地，2026-08-08 验证通过）

首批只搭 ECS 骨架 + 迁移 Entity 基类最高频 4 字段（position/velocity/aabb/rotation）到组件，并打通 OOP↔ECS 双向桥接。后续子系统迁移按末尾路线分批进行。

**已交付**：
- **ECS 骨架**（`src/common/entity/ecs/context/`）：`EntityId`（64 位句柄 + `EntityIdTraits` 特化扩 version 位）、`EntityRegistry`（entt 薄包装，每 IWorld 一个）、`EntityContext`（三元组组件查询入口）。
- **6 组件**（`src/common/entity/ecs/components/`）：`StateVectorComponent`（pos/posPrev/posDelta）、`VelocityComponent`、`AABBShapeComponent`（aabb/bbDim）、`EntityRotationComponent`（rot/rotPrev）、`EntityOwnerComponent`（unique_ptr<Entity> 反向桥接）、`EntityUniqueIDComponent`（u64 持久投影）。另有 `BuiltInEntityComponents`（4 高频组件裸指针缓存，对齐基岩版 `BuiltInActorComponents`）。
- **系统层**（`src/common/entity/ecs/systems/`）：`ISystem`/`ITickingSystem` 接口、`SystemPhase` 阶段枚举、`EntitySystemScheduler` 阶段编排器、`EntityLegacyTickSystem`（包装 `Entity::tick()` 虚函数链）。
- **OOP↔ECS 桥接**：`Entity` 构造签名透传 `ecs::EntityRegistry&`、删 8 字段（m_position/m_prevPosition/m_velocity/m_yaw/m_pitch/m_prevYaw/m_prevPitch/m_boundingBox）、`m_builtIn` 缓存 4 裸指针、getter/setter 切组件指针读写（零双写）；`EntityManager` 持 `ecs::EntityRegistry&`；`IWorld` 加 `entityRegistry()` 虚方法；`ServerWorld`/`ClientWorld` 各持 `ecs::EntityRegistry` 成员。
- **全量透传**：继承链全部中间基类 + 全部叶子类构造透传 registry（Creature/Ageable/Animal/Monster/.../AbstractPiglin 等中间层 + 全部叶子）；B 类特殊支系（Effect/Hanging/Boat/Minecart/ItemEntity/ExperienceOrb 等）构造加 registry 形参；全部叶子工厂 `create(IWorld*)` → `create(IWorld*, ecs::EntityRegistry&)`；全部生产调用点（约 34 处 `entityType->create` + 约 57 处 `make_unique`）补 registry。

**验证结果**（2026-08-08）：
- 构建：`cmake --build --preset windows-clang-relwithdebinfo --target minecraft-client` / `--target minecraft-server` 均 exit 0，两 exe 链接产出。
- 冒烟：`minecraft-client --quick-play-new` 完整启动→世界加载→内置服务端 20 TPS→玩家 Steve(PlayerId=1, EntityInstanceId=3) joined the game→区块发送→干净退出，无崩溃。证明 OOP↔ECS 桥接未破坏现有行为。
- GameTest 回归：`minecraft-server --gametest` 8/8 passed（simpleMobTest/zombie_villager_chase/iron_golem_arena/zoglin_float/cloneBlocksCommand 等），覆盖实体生成/寻路/AI/位置同步/tick 全链路，高频4字段迁移后零回归。

**暂未处理（用户决策）**：`tests/` 目录约 692+ 处 Entity 子类构造 + 45 个测试桩需透传 registry，当前 `mc_tests` target 编译失败但不影响 client/server 本体（用 `--target minecraft-client/minecraft-server` 绕过）。测试改造留待后续（需测试 fixture 持本地 `ecs::EntityRegistry` 成员 + 桩构造透传）。

## 第二批范围（已落地，2026-08-08 验证通过）

首批留下两个缺口：① `EntitySystemScheduler` 是孤儿骨架（`EntityManager::tick()` 直接遍历调 `entity->tick()`，从未接入 scheduler）；② 组件覆盖面仅 4 字段，继承链大量中频状态数据仍是 OOP 成员。本批目标"增加组件和系统的丰富度"：打通 scheduler 接入 + 新增 5 数据组件 + 2 真实业务 System。

**已交付**：
- **scheduler 接入 + 阶段重命名**：`EntityManager::tick()` 改委托 `EntitySystemScheduler`（首批为孤儿骨架）；阶段枚举 `LegacyTick` → `EntityTick`，新增 `PostEntityTick`；抽 `_tickEntities()` 私有方法承载原遍历逻辑。
- **5 数据组件**（`src/common/entity/ecs/components/`）：`PortalComponent`（inPortal/portalTime/portalCooldown/portalPos）、`FireComponent`（fire 燃烧/免疫计时）、`PhysicsStateComponent`（onGround/fallDistance/collided*）、`HurtStateComponent`（absorption/hurtTime/maxHurtTime/deathTime，仅 LivingEntity）、`FreezeComponent`（ticksFrozen/isInPowderSnow）。Entity 构造 attach 8 组件，LivingEntity 续接 attach HurtState。
- **2 真实 System**（`src/common/entity/ecs/systems/`）：`PortalTickSystem`（portal 计时/冷却/传送触发，从 baseTick/tick 抽出）、`FireTickSystem`（fire 递减/燃烧伤害/雨中扑灭，从 baseTick 抽出），注册入 `PostEntityTick` 阶段。这是项目首批非桥接真实业务 System，标志 ECS 行为层启动。System 经 `EntityOwnerComponent` 反查 OOP 句柄调虚函数（canTeleport/isInWater/hurt 等）保留多态。
- **字段迁移**（零双写硬约束，删原字段 getter/setter 切组件）：Portal 4 字段、Fire 1 字段、PhysicsState 4 字段（B 类强时序逻辑留 OOP 不抽 System，破例进 m_builtIn 高频缓存）、HurtState 4 字段（LivingEntity 层，低频 try_get）、Freeze 2 字段（m_ticksFrozen 同步真相源，DATA_TICKS_FROZEN_PARAM 退为镜像，消除首批 setTicksFrozen 双写）。全仓 60+ 直接访问点切组件读写，NBT 存读走 getter/setter。
- **BuiltIn 缓存扩展**：`m_builtIn` 从首批 4 指针增至 5（+PhysicsState 高频破例）；Portal/Fire/Freeze/HurtState 走 `tryGetComponent` 低频查询。

**字段三分类**（迁移策略依据）：
- **A 类**（纯本地无同步，本批抽 System）：portal 组、fire 组。
- **B 类**（强时序，数据组件化但逻辑留 OOP）：PhysicsState 组（与 move/checkOnGround/updateFallDistance 强耦合，拆 System 破坏碰撞→清速→落地即时序）。
- **C 类**（同步字段，本批仅 m_ticksFrozen）：组件为真相源，DataParameter 退镜像。其余 C 类（air/health/noGravity/pose/flags 等）本批不动。

**验证结果**（2026-08-08）：
- 构建：client/server 全量构建均 exit 0 链接产出。
- GameTest 回归：8/8 passed 零回归（zoglin_float 为已知时序敏感 flaky——shulker AI 在 maxTicks=210 内攻击 zoglin 偶发超时，非本批引入，首批基准即 6/8）。
- 跨帧延迟（用户已接受）：fire/portal 抽到 PostEntityTick 后递减结果下帧 baseTick 才读到，单帧 50ms 玩家无感。

**暂未处理**：`tests/` 仍不透传 registry（永久约束）。客户端 FreezeComponent 回填点暂不存在（ClientEntity 独立类不继承 mc::Entity、不消费 ticksFrozen），未来实现客户端冰冻渲染时需补。

## 第三批：mob 基础数据组件化（2026-08-09 落地）

延续第二批数据层迁移势头，将 LivingEntity 的 health/attribute/equipment/arrows 四组中频字段搬入 entt 组件。第二批已确立的"组件真相源 + DataParameter 镜像"模式在本批规模化复用，模式成熟风险可控。

**新增 4 组件**（均仅 LivingEntity attach，普通 Entity 不持有）：
- `HealthComponent`（m_health/m_lastHealth/m_healthSynced）：m_health 为同步真相源，`DATA_HEALTH_PARAM` 退为镜像。延续第二批 freeze 的同步镜像模式。
- `EquipmentComponent`（m_equipment[8]/m_lastEquipment[8]/m_lastEquipmentInitialized）：装备无独立 DataParameter（走实体追踪器路径），setEquipment 单写组件，无镜像，最简单的组件化场景。
- `ArrowStateComponent`（m_arrowCount/m_stingerCount/m_arrowHitTimer）：arrowCount/stingerCount 同步真相源。箭矢/蜂针不持久化（无 NBT），仅运行时同步。
- `AttributeComponent`（`unique_ptr<AttributeMap>` 包裹）：承载 LivingEntity::m_attributes。AttributeMap 含 `mutable std::mutex` 不可移动/拷贝，用 unique_ptr 包裹使组件可移动（不引入 pinned type 契约风险）。

**关键设计决策**：
- AttributeComponent 须在 `registerAttributes()` 之前 attach——后者经 `attributes()` getter 取组件填充默认属性，时序颠倒则 getter 的 `MC_ASSERT_RELEASE` 断言失败。
- 原 `protected m_attributes` 成员被子类约 70 文件 320 处直接访问（registerAttributes 重写），删除成员后子类改调 `public attributes()` getter 委托（`m_attributes.xxx` → `attributes().xxx`）。getter 提供 const + 非 const 双重载。
- 构造期虚函数时序：LivingEntity 构造函数调 registerAttributes() 时 vtable 仍是 LivingEntity 的（仅注册基类 7 属性），叶子类构造体显式再调 registerAttributes() 经继承链逐层向上注册专属属性（如 chicken=4.0/fox=10.0）。组件化后该时序不变——叶子类构造体执行时基类已 attach AttributeComponent，`attributes()` 可取到组件。
- health 初始值同步至 maxHealth 在 tick() 首帧兜底执行（HealthComponent.m_healthSynced 标志），因派生类 registerAttributes 时序晚于 LivingEntity 构造，构造期 setHealth 无法拿到派生类 maxHealth。

**验证结果**（2026-08-09）：
- 构建：client/server 本体均 exit 0 链接产出（`--target minecraft-client/minecraft-server` 绕过 mc_tests）。
- GameTest 回归：8/8 passed exitCode=0 零回归（simpleMobTest 含 fox 属性/寻路、zombie_villager_chase、iron_golem_arena、zoglin_float 等均通过，证明属性系统迁移无回归）。
- 修复本批引入的 IWYU 违规：LivingEntity.hpp 的 inline `attributes()` getter 用 `MC_ASSERT_RELEASE` 未 include `AssertMacros.hpp`（基类 Entity.hpp 未传递，tests/ 包含时暴露），补 include。

**暂未处理**：`tests/` 741 处测试构造仍不透传 registry（永久约束）。Player 子类重写 getMutableEquipment/setEquipment 委托到 PlayerInventory，不读写 EquipmentComponent，本组件仅承载非 Player 的 LivingEntity 装备。

## 第四批：C 类字段全量组件真相源化（2026-08-09 落地）

批次4 原计划「用基岩版 SynchedData 中间层替换 EntityDataManager」，但深度勘察发现 **EntityDataManager 已是对齐 vanilla ClassTreeIdRegistry 的成熟 SynchedEntityData 等价物**（ID 继承链分配 / wire 格式 1.21.11 / dirty 机制完整），并非待替换的旧物。故本批真实目标改为：**把剩余 C 类同步字段全部改成组件真相源 + DataParameter 镜像模式**，与已完成的 health/arrows/ticksFrozen 同模式规模化，统一 ECS 数据层。wire 格式不变，网络层零改动。

对 EntityDataManager 全部 22 个 DataParameter 逐字段勘察（成员字段/setter 双写/syncMetadata 回填/NBT/客户端消费），分三类：
- **迁移 9 字段**：Entity 层 7 个（flags/air/customName/customNameVisible/silent/noGravity/pose）+ Player score + Player absorption 下发。
- **不迁移 13 个**：6 个纯占位（effect 粒子/睡眠位/主手/肩膀鹦鹉）+ DATA_LIVING_FLAGS 半残 + DATA_MOB_FLAGS 已符合 + DATA_PLAYER_MODE_CUSTOMISATION 收益低留后续。

**新增 3 组件 + 1 枚举头提取**：
- `EntityFlagsComponent`（m_flags）：Entity 层，所有实体 attach。DATA_FLAGS_PARAM(id0) 退镜像。
- `EntityStateComponent`（m_air/m_customName/m_customNameVisible/m_silent/m_noGravity/m_pose 6 字段）：Entity 层，所有实体 attach。聚合轻量同步元数据避免组件爆炸。m_customName 用 `unique_ptr<ITextComponent>` 承接原类型（保留样式/颜色），DataParameter 仅存 OptionalComponentValue 作有损同步投影。
- `PlayerScoreComponent`（m_score）：仅 Player attach。DATA_PLAYER_SCORE_PARAM(id18) 退镜像。
- `EntityFlags.hpp`：把原内联于 Entity.hpp 的 `EntityFlags` 枚举 + 位运算符提取为独立头（参照 EquipmentSlot 提取先例），消除 EntityFlagsComponent 对 Entity.hpp 的循环依赖。

**Player absorption 特殊处理（无新组件）**：真相源已在 HurtStateComponent.m_absorption（第三批）。缺陷是 LivingEntity::setAbsorptionAmount 非虚、不下发 DATA_PLAYER_ABSORPTION_PARAM。修复：把 LivingEntity::setAbsorptionAmount 改 `virtual`，Player `override` 重写——调基类写 HurtStateComponent（含 clamp）后，再 `m_dataManager.set(DATA_PLAYER_ABSORPTION_PARAM, absorptionAmount())` 下发 clamp 后值。基类 actuallyHurt 内调用经虚派发到 Player 版本，同步链路完整。

**关键设计决策**：
- syncMetadataFromDataManager 删除全部 7 字段回填行，函数体空保留骨架 + 注释（组件为真相源，不从镜像回填）。延续第二批 freeze 模式。
- setPose 迁移后**保留 refreshDimensions() 副作用**（潜行变矮/游泳变扁的碰撞箱更新）。syncMetadata 末尾原 refreshDimensions（服务 pose 回填）随回填行删除，但 setPose 内的 refreshDimensions 保留。
- customName 一致性：setCustomNameComponent 内先写组件 `c->m_customName = std::move(name)`，再从组件取 text 写 DataParameter，确保组件与镜像一致。
- registerData 字面量默认值不变（构造期组件虽已 attach 但 registerParam 不读组件，与现状一致）。
- inline getter（pose/flags/air 等）走 tryGetComponent + MC_ASSERT_RELEASE 守卫，与第三批 attributes() 同模式。Entity.hpp 补 include AssertMacros.hpp。

**子类直接访问修复（7 处）**：原 protected 成员被子类直接访问，删成员后改 getter/setter：LivingEntity.cpp:1703（m_pose→pose()）、Player.cpp height/eyeHeight/_canFitPose/水浮力（m_pose→pose()、m_noGravity→hasNoGravity()）、FishingBobberEntity/EyeOfEnderEntity 构造（m_noGravity=false→setNoGravity(false)）。ShulkerBulletEntity 访问的是 ProjectileEntity 自己的 m_noGravity（合法未动）。

**验证结果**（2026-08-09）：
- 构建：client/server 本体均 exit 0 链接产出（`--target minecraft-client/minecraft-server` 绕过 mc_tests）。
- GameTest 回归：8/8 passed exitCode=0 零回归（simpleMobTest 首跑偶发超时为已知 flaky——fox AI 时序敏感 + 光照引擎 TOCTOU，非本批引入，复跑通过）。
- 修复构建错误：EntityStateComponent.hpp 用 `EntityPose` 未限定 `entity::` 命名空间（组件在 mc::ecs，枚举在 mc::entity）。

**暂未处理**：`tests/` 741 处测试构造仍不透传 registry（永久约束）。DATA_PLAYER_MODE_CUSTOMISATION_PARAM 留后续批次。

## 第五批：实体能力 mixin 接口转 tag/capability component（5.1 IMob 试点落地，2026-08-09 收束于 5.1）

批次1-4 完成实体**状态数据层** ECS 化后，批次5 转向**类型标识层**：把实体能力 mixin 接口（IMob/IShearable/IRideable 等 11 个）转成 tag/capability component。经全仓勘察，生产代码 dynamic_cast 共 1458 处，但批次5 真实作用域远小于原路线图「约 1220 处」：实体能力 mixin 接口仅 5 处生产 dynamic_cast（原「16 处」含其他子批预估），其余为 NBT 节点 429 处 / 实体子类下行约 280 处 / UI 与结构生成约 60 处 / 方块能力 21 处 / 容器接口 16 处 / IWorld 跨边界 10 处，均不在本批。详见末尾路线图修正条目。

子批 5.1 是 IMob 单接口试点，验证「接口保留 + tag 并存」混合架构模式。

**策略**：接口保留作 OOP 行为层（IMob.hpp 不动，MonsterEntity 仍 `public IMob`），tag component 作 ECS 类型标记层，dynamic_cast 改 `hasComponent<T>()`，二者并存对齐基岩版混合架构，tests/ 零改动。

**已交付**：
- **新组件** `MobFlagComponent`（空 struct tag，`src/common/entity/ecs/components/`）：承载 IMob 类型标记语义。MonsterEntity 构造 attach 一次，20+ 怪物子类（Zombie/Skeleton/Creeper/Shulker/Enderman/Blaze 等）经 MonsterEntity 间接获得 tag。header-only 不改 CMake。
- **Entity public hasComponent 包装**：Entity.hpp 加 `template<class T> bool hasComponent() const` 透传 `m_entityContext->hasComponent<T>()`。这是 4 处外部指针改造的前置条件——`m_entityContext` 是 protected 成员，AI goal/sensor、BlockEntity 等继承体系外的调用方无法直接访问。包装只暴露布尔查询不暴露整个 EntityContext，是 ECS 混合架构的外部查询统一入口，后续 5.2/5.3/5.4 复用。
- **5 处生产 dynamic_cast<IMob*> 改造**：Sensors.cpp:615（AvoidEntitySensor 避险判定）、AvoidHostileGoal.cpp:136（村民避敌谓词）、ConduitEntity.cpp:293（潮涌核心攻击敌对生物）、ShulkerGoals.cpp:226（潜影贝防御攻击谓词）、MobEntity.cpp:744（canBeLeashed 拴绳判定）。前 4 处用 `other->hasComponent<MobFlagComponent>()`（外部指针经 Entity public 包装），第 5 处用 `hasComponent<MobFlagComponent>()`（this 场景，派生类内 public 方法）。每处移除无用 `IMob.hpp` include 加 `MobFlagComponent.hpp`。Sensors.cpp:615 中 Player 的 dynamic_cast 保留（实体子类下行，另案）。
- **文档同步**：core/README.md:537 canBeLeashed 描述更新为 hasComponent。

**关键设计决策**：
- **接口保留不删**：tag 只承担类型标记，行为层（虚函数）继续走 vftable。tests/ 9 处 `dynamic_cast<IMob*>`（4 文件：EntityTrackerUuidTest/SensorsTest/MobEntityInteractTest/ShulkerEntityTest）因此零改动——测试走 OOP 接口层，生产走 ECS tag 层，并存即设计意图。
- **Entity public hasComponent（方案 A）**：只暴露布尔查询，不暴露整个 EntityContext（方案 B 权限过大）。const 安全，ShulkerGoals 的 `const LivingEntity*` 与 MobEntity `canBeLeashed() const` 均兼容。
- **MonsterEntity 单点 attach**：IMob 唯一继承点是 MonsterEntity，子类经此间接获得 tag，不在每个子类重复 attach。
- **空 struct tag 零内存开销**：entt 原生支持空 tag（`all_of<EmptyTag>` 编译期类型 id 比较），对齐基岩版 `Is*FlagComponent` 命名约定。

**验证结果**（2026-08-09）：
- 构建：client/server 本体均 exit 0 链接产出（`--target` 绕过 mc_tests）。
- GameTest 回归：核心场景 zombie_villager_chase（村民避险 AvoidHostileGoal/Sensors）/ iron_golem_arena（怪物 AI）/ simpleMobTest 三次复跑均稳定 PASSED，证明 hasComponent 替代 dynamic_cast 语义等价。zoglin_float/collapsing 偶发超时为已知 flaky 群（shulker AI 时序 + 光照引擎 TOCTOU，记忆 `ecs-migration-batch2-landed` 记录非本批引入），三次复跑每次恰好 1 个不同 flaky 测试超时，无固定失败。

**暂未处理**：`tests/` 9 处 dynamic_cast<IMob*> 保留（接口存在即可编译，永久约束）。

## 批次5 收束决策（2026-08-09）

5.2-5.4 取消。全仓勘察剩余 10 个 mixin 接口（`src/common/entity/interfaces/`）后发现，5.1 IMob 是**纯类型标记接口**的特例（仅虚析构无业务虚方法，所有 5 处 cast 后只判 nullptr），其「接口转 tag + dynamic_cast 改 hasComponent」模式不可推广。剩余接口的 dynamic_cast 绝大多数是**接口多态分发**模式（cast 后立即调接口虚方法），hasComponent 无法替代虚方法调用：

- **接口多态分发（11 处生产 cast，cast 后调虚方法）**：IShearable（ShearsItem 调 shear/isShearable）/IEquipable（SaddleItem 调 getEquipment/setEquipment）/ICrossbowUser（RangedAttackGoals 4 处调 setChargingCrossbow/shootCrossbow/getCrossbowChargeTime）/IRangedAttackMob（RangedAttackGoals 调 attackEntityWithRangedAttack）/IJumpingMount（ServerPlayHandler 2 处调 startJumping/stopJumping）/IRideable（OnAStickItem+SaddleItem 调 boost/hasSaddle/setSaddle）/ContainerUser（ChestEntity+CopperGolemGoals 调 hasContainerOpen/getContainerInteractionRange，另含 2 处接口引用参数多态 ChestEntity::startOpen/stopOpen 经 `ContainerUser&` 调 getLivingEntity）。
- **带状态接口（0 生产 cast 但模板静态多态消费）**：IAngerable 经 `ResetAngerGoal<T>` 静态多态调 getRevengeTarget/setAngry/setAngerTime 等带状态虚方法，实现类众多（Tameable/Golem/Enderman/Bee/PolarBear/Piglin），愤怒计时器/复仇目标是状态字段，属 5.3 带状态范畴。
- **零消费点（能纯 tag 但 tag 本身无用）**：IFlinging/IFlyingAnimal 0 cast + 0 接口指针多态消费，虚方法无运行时消费点（IFlinging 的 attackWithFling 是静态方法调用不走多态；IFlyingAnimal 的 isFlying/setFlying 等仅被实现类 override 占位，外部不通过接口分发）。建 tag 是无消费点的架构债。

性能上，剩余 cast 多在低频路径（物品右键 ShearsItem/SaddleItem、骑乘键 IJumpingMount、开容器 ContainerUser），仅 ICrossbowUser/IRangedAttackMob 在 RangedAttackGoals 每 tick 跑但量级是 per-mob（当前攻击中的怪物）非 per-entity 遍历（对比 IMob 是 sensor 遍历范围内所有实体判怪），RTTI 开销可忽略。基岩版混合架构决策（七项根本决策第 1 条：AI/序列化等低频逻辑暂留 OOP）本就允许 OOP 行为层保留。强行 ECS 化要么半吊子（capability component 持接口指针委托，没消除多态只省 RTTI 字符串比较，且与基岩版 EntityContext 查询模式不对齐）要么破坏 AI 保留 OOP 决策（把虚方法分发本身改成系统查询，违反 Goal/Brain/Navigator 约 5 万行不 ECS 化的决策）。故**批次5 收束于 5.1**，剩余 mixin 接口 OOP 多态作为混合架构的合理行为层保留，转向批次6。

## 后续批次路线（备忘，未落地）

- **批次5（已收束于 5.1）**：实体能力 mixin 接口转 tag/capability component。经全仓勘察，生产代码 dynamic_cast 共 1458 处，但批次5 真实作用域远小于原路线图「约 1220 处」——mixin 接口相关 cast 仅 IMob 5 处可纯 tag 迁移（5.1 已落地）。其余 dynamic_cast 分布：NBT/Tag 节点遍历 429 处（NBT 多态树固有模式，不迁移）、实体具体子类下行约 280 处（Player/LivingEntity/MobEntity/各 Entity 子类，不适合转 tag——子类太多且生命周期与实体绑定，另案）、UI 控件/结构生成具体类约 60 处（不迁移）、方块能力接口 21 处（world/block 域）、容器接口 16 处（inventory 域）、IWorld 跨边界转型 10 处（结构生成子系统另案）。**剩余 10 个 mixin 接口（IShearable/IEquipable/ICrossbowUser/IRangedAttackMob/IJumpingMount/IRideable/ContainerUser/IAngerable/IFlinging/IFlyingAnimal）的 dynamic_cast 经 5.2 勘察确认为「接口多态分发」模式（cast 后立即调接口虚方法），hasComponent 无法替代虚方法调用，违反 AI 保留 OOP 决策；IFlinging/IFlyingAnimal 虽能纯 tag 但零消费点是架构债**。故批次5 收束于 5.1 IMob 试点，剩余 mixin 接口 OOP 多态作混合架构合理行为层保留。详见上文「批次5 收束决策」段。
- **批次6**：序列化按组件注册序列化器；Brain 模板实例化重构为泛型 System。projectile 族整块 ECS 化（验证创建→tick→同步→销毁全链路，原批次2 试点延后至此）。
- **批次7**：server/client 专属 system 落地（EntityTracker 同步 system、客户端镜像 system）。客户端冰冻渲染 + FreezeComponent 回填点在此批补。
- **批次8**：引入定义驱动层（ActorDefinitionIdentifier + 组件工厂），适配脚本/gametest 组件式 API。

> 本目录 `src/common/entity/ecs/README.md` 记录了 ECS 层内部结构、上下游依赖与 15 条容易踩的坑（双写禁忌、句柄脆弱性、跨 registry 不可迁移、命名遮蔽、指针稳定性、CMake 显式列举、ClientWorld 不继承 IWorld、占位 Player registry 来源、低频组件查询性能、同步镜像字段组件化、SystemPhase 演进与跨帧延迟、不可移动类型 unique_ptr 包裹范式、C 类同步字段批量迁移 protected 转 getter 委托、C 类字段全量组件化规模化枚举提取消除循环依赖 + 异构字段聚合组件、mixin 接口转 tag component 接口保留 + Entity public hasComponent 包装 + 热路径外部指针改造），迁移后续批次前务必先读。
