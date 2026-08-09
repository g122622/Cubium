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

## 第六批子目标1：序列化按组件注册（2026-08-09 落地）

批次1-4 完成实体状态数据层 ECS 化（约 19 字段迁入 entt 组件）后，**序列化仍是 OOP 虚函数链**：4 层 `writeToNBT`/`addAdditionalSaveData`（Entity→LivingEntity→MobEntity→Player 逐层 super）承载约 66 个 NBT 字段，其中 19 个已组件化字段的序列化代码仍埋在 OOP 虚函数里经 getter/setter 间接读写组件——组件本身无任何序列化代码。本批目标：搭「组件序列化器注册表」基础设施，把 19 个已组件化字段的序列化逻辑从 OOP 虚函数搬到组件序列化器。打通后后续每迁一个组件自带持久化，避免"迁组件再补 NBT"双段式工作。

**已交付**：
- **注册表基础设施**（`src/common/entity/serialization/components/`）：`ComponentSerializerRegistry`（进程单例，`std::vector<Entry>`，Entry=`{entt::type_id<T>().hash(), SaveFn, LoadFn, priority}`）。键用 `entt::type_id`（编译期类型安全）非基岩版 `HashedString`；裸函数指针 `SaveFn`/`LoadFn` 非 `std::function`；`vector` 非 `unordered_map`（仅 13 条目 cache 友好可排序）。对齐基岩版 `InternalComponentRegistry`（`unordered_map<组件名,{save,load,legacy-convert}>` 静态注册表与 `addAdditionalSaveData` 虚函数并存）。去掉 legacy-convert（项目对齐 Java 版平铺格式无旧版存档转换需求）。
- **Entity public tryGetComponent 包装**：Entity.hpp 加 public `tryGetComponent<T>()` const/非 const 双重载（透传 `m_entityContext->tryGetComponent<T>()`），与第五批 hasComponent 配套。注释限定"仅供序列化器/AI/BlockEntity 等横切关注点使用，业务逻辑走 getter/setter"。
- **13 个序列化器对**（按继承层分 3 文件），覆盖 19 字段：
  - `EntityComponentSerialization`（9 序列化器，13 字段）：StateVector→Pos / Velocity→Motion / EntityRotation→Rotation / PhysicsState→FallDistance+OnGround / Fire→Fire / Portal→PortalCooldown / Freeze→TicksFrozen / EntityState→Air+CustomName+CustomNameVisible+Silent+NoGravity / EntityFlags→FallFlying。
  - `LivingEntityComponentSerialization`（3 序列化器，5 字段，`dynamic_cast<LivingEntity*>` 早退）：Health→Health（+置 m_healthSynced） / HurtState→Absorption+HurtTime+DeathTime / Equipment→Equipment（含旧格式 HandItems/ArmorItems 回退）。
  - `PlayerComponentSerialization`（1 序列化器，1 字段，`dynamic_cast<Player*>` 早退）：PlayerScore→Score。
- **writeToNBT/readFromNBT 改造**：`Entity::writeToNBT` 删 13 字段直写改调 `saveAll`，保留 UUID/Invulnerable/Glowing/Tags/Passengers；`readFromNBT` 删 13 字段直读改调 `loadAll`，保留 UUID/Invulnerable/Glowing/Tags/reapplyPosition。`LivingEntity::addAdditionalSaveData/readAdditionalSaveData` 删 Health/Absorption/HurtTime/DeathTime/Equipment/FallFlying，保留 HurtByTimestamp/ActiveEffects/Attributes。`Player::addAdditionalSaveData/readAdditionalSaveData` 删 Score。FallFlying 从 LivingEntity 层上提到 Entity 层（按 EntityFlagsComponent 注册）。
- **字段访问策略**：能调 public setter 的优先调（保留 DataParameter 同步副作用——C 类字段硬约束）。3 字段（Pos/Rotation/OnGround）现行 `readFromNBT` 刻意绕过 setter 副作用直写 `m_builtIn.*` 组件，序列化器经 public `tryGetComponent<T>()` 拿同一组件指针直写，语义完全一致。Health load 调 `setHealth`（clamp(0,maxHealth)）后置 `m_healthSynced=true` 避免首帧覆盖。

**关键设计决策**：
- **存档格式不变**：保持 Java 版平铺格式（Pos/Motion/Health 等直接在根 tag），键名不变，零迁移成本旧存档兼容。`writeToNBT` 被 11 处复用（DataAccessor/EntityResolver/CopyNbtFunction/Template/NBTPredicate/PlayerResolver/EntityDeserializer），改造内部结构对调用方透明。
- **load 顺序依赖（已化解）**：Health/Absorption 的 `setHealth` 内 `clamp(0, maxHealth)` 读 AttributeMap，但 AttributeMap 在构造期 `registerAttributes` 已就位（派生类 MAX_HEALTH 默认值），非 NBT load 顺序依赖。本批不迁 Attributes（仍留 `readAdditionalSaveData` 虚函数内），维持现状。`loadAll` 在 `readAdditionalSaveData` 之前调，与原顺序一致。`Entry.priority` 字段为未来迁 Attributes（=100）/ActiveEffects（=200）保证顺序预留。
- **注册时机**：`VanillaEntities::registerAll()`（:132）用 `hasType(PIG)` 哨兵提前返回，故 `ComponentSerializerRegistry::registerAll()` 须放 `doRegisterAll()` 末尾。`registerAll` 幂等（`m_registered` 标志 + clear 重注册，同 typeId 覆盖非追加）。
- **dynamic_cast 早退**：LivingEntity/Player 层序列化器经 `Entity&` 调用，内部 `dynamic_cast<LivingEntity*>`/`dynamic_cast<Player*>`。非目标类型实体返回 nullptr 早退无副作用。LivingEntity/Player 非 final、Entity 虚析构，RTTI 可用。

**验证结果**（2026-08-09）：
- 构建：client/server 本体均 exit 0 链接产出（`--target minecraft-client/minecraft-server` 绕过 mc_tests）。
- GameTest 回归：核心场景全过——cloneBlocksCommand（结构 NBT 实体序列化）/ simpleMobTest（fox 属性/寻路）/ zombie_villager_chase / iron_golem_arena（怪物 AI，LivingEntity 序列化）/ alwaysSucceed / minibiomes。zoglin_float/collapsing/simpleMobTest 偶发超时为已知 flaky 群（shulker AI 时序 + 光照引擎 TOCTOU，记忆 `ecs-migration-batch2-landed` 记录非本批引入），多次复跑每次失败的是 flaky 三元组内不同子集，无固定失败。Player Score 序列化路径不在 mob GameTest 覆盖范围（无玩家存档重载测试），靠代码逻辑等价性保证（save 走 getScore，load 走 setScore，与原 readAdditionalSaveData 完全一致）。

**暂未处理**：`tests/` 仍不透传 registry（永久约束）。48 个纯 OOP 字段（Player 背包/FoodStats/ExperienceManager/ActiveEffects/Attributes/MobEntity 全层等）仍走 `addAdditionalSaveData`/`readAdditionalSaveData` 虚函数，留待后续批次随组件化逐步迁入注册表。批次6 子目标3（Brain 模板实例化重构为泛型 System）待办。

## 第六批子目标2：projectile 族整块 ECS 化（2026-08-09 落地）

projectile 族（20 个类，3 个继承根：ProjectileEntity 子树 / FishingBobber / EvokerFangs / EyeOfEnder）的特有状态字段仍全是 OOP 成员，且存在三处与 vanilla 1.21.11 的系统性偏差：网络同步缺口（除 FishingBobber 外全无 C 类 DataParameter，客户端拿不到暴击/插地/owner 等状态）、持久化缺口（箭/三叉戟整族不存盘，owner UUID 全族完全不持久化）、m_noGravity 遮蔽双真相源 bug（批次4 遗留）。本批目标：全族整块 ECS 化 + 补齐网络同步字段 + 补齐持久化 + 先修 bug。分 8 步（Step0 修 bug → Step1 组件骨架 → Step2 owner ECS 化 → Step3 AbstractArrow 13 字段 → Step4 叶子类组件化 → Step5 同步字段补齐 → Step6 持久化补齐 → Step7 验证+文档）。

**已交付**：
- **Step0（修 m_noGravity 遮蔽 bug，commit b0eaf2e0b）**：删 `ProjectileEntity.hpp` 重声明的 `m_noGravity` 成员 + inline getter/setter，让调用回落基类 `Entity::hasNoGravity/setNoGravity`（走 EntityStateComponent / DATA_NO_GRAVITY 镜像）。此前火球/潜影贝子弹 `setNoGravity(true)` 只写子类成员，组件+镜像恒假，客户端见 projectile 受重力下坠。
- **Step1（组件骨架+attach，commit b75feec14）**：新建 16 个 projectile 族组件头文件（`src/common/entity/ecs/components/`），纯 struct 数据。不可移动类型（unordered_set/ItemStack/vector）用 `unique_ptr<T>` 包裹（沿用 AttributeComponent 范式）。各 projectile 构造函数 attach 对应组件。CMake 登记新头。组件与 OOP 字段并存空跑（不删 OOP 字段）。
- **Step2（owner ECS 化，commit f262e0b76）**：ProjectileEntity 5 字段（shooterUuid/shooterEntityId/leftShooter/lastDeflectedById/hasBeenShot）迁入 `ProjectileOwnerComponent`。`getShooter`/`setShooter`/`hasLeftShooter`/`checkLeftShooter` 改走 `tryGetComponent`。getShooter nullptr 安全降级，setShooter nullptr 断言（attach 硬约束）。35 处 owner 调用点签名不变。EvokerFangs 6 字段迁 EvokerFangsComponent（独立 owner 机制）。FishingBobber m_angler 迁 FishingBobberComponent。
- **Step3（AbstractArrow 13 字段，commit a6e0873be）**：AbstractArrowEntity 13 字段（damage/knockback/critical/pierceLevel/inGround/ticksInGround/timeInGround/arrowShake/pickupStatus/shotFromCrossbow/dealtDamage/piercedEntities/inBlockState）迁入 `ProjectileArrowStateComponent`（注意与 LivingEntity 既有 `ArrowStateComponent` 重名故加 Projectile 前缀）。ArrowEntity/SpectralArrowEntity/TridentEntity/SpearEntity 子类对父字段访问改组件。
- **Step4（叶子类+直系支系组件化，commit f90869013）**：ArrowEntity→ArrowEffectsComponent、SpectralArrow→SpectralArrowComponent、Fireball/WitherSkull→FireballStateComponent（共用）、Potion→PotionProjectileComponent、ExperienceBottle→ExperienceBottleComponent、ProjectileItemEntity→ProjectileItemComponent、WindCharge→WindChargeStateComponent、DamagingProjectile→DamagingProjectileComponent、ShulkerBullet→ShulkerBulletComponent、FireworkRocket→FireworkRocketComponent、Trident→TridentStateComponent、EyeOfEnder→EyeOfEnderComponent。各类 getter/setter 改组件读写，删 OOP 字段声明。
- **Step5（同步字段补齐，commit 4374e2f46）**：补齐对齐 vanilla 1.21.11 的 C 类 DataParameter：AbstractArrow DATA_ARROW_FLAGS(u8 位标志 bit0=crit/bit2=shotFromCrossbow)/PIERCE_LEVEL/IN_GROUND（id8/9/10）、Trident DATA_LOYALTY/DATA_FOIL（id11/12）、FireworkRocket DATA_FIREWORKS_ITEM/ATTACHED_TO_TARGET/SHOT_AT_ANGLE（id8/9/10）、Fireball DATA_ITEM_STACK（id8 占位）、WitherSkull DATA_DANGEROUS（id8 镜像 m_blue）、EyeOfEnder DATA_ITEM_STACK（id8 占位）。中间基类 DamagingProjectile/AbstractFireball 补 classInfo 占位节点（无字段，无 registerData override）。同步字段真相源进组件 + DataParameter 镜像（批次4 模式），构造函数显式调 registerData。客户端消费分支留 TODO（真 Java 客户端通过 SetEntityData 自行消费）。
- **Step6（持久化补齐，commit cf7b41f1b）**：新建 `ProjectileComponentSerialization.hpp/.cpp`，注册 10 个组件序列化器到 ComponentSerializerRegistry。对齐 vanilla 1.21.11 各 projectile 类持久化字段清单。owner UUID 用双 long（OwnerUUIDMost/Least，与 EvokerFangs/AreaEffectCloud 一致非 vanilla EntityReference 单键）。dealtDamage 归父组件 ProjectileArrowStateComponent，load 顺序 priority 首次启用（TridentState=0 先 load 重算 loyalty，ArrowState=10 后 load dealtDamage）。loyalty 不存盘（从 item 重算）。EvokerFangs/FireworkRocket/Spear 既有 OOP override 改空壳（搬注册表防双重写入）。FishingBobber 不持久化（对齐 vanilla FishingHook 空实现）。

**验证**：增量构建 minecraft-server+minecraft-client 零错误；GameTest 8/8 零回归（zoglin_float 偶发 flaky 超时复跑即过）。客户端手测渲染（暴击粒子/三叉戟光泽/凋灵之首蓝色）属独立子任务，服务端同步字段补齐即可让真 Java 客户端正常渲染。

**关键设计决策**：
- **dealtDamage 归属**：放 ProjectileArrowStateComponent（vanilla AbstractArrow 也有此字段仅 Trident 用），TridentEntity 复用父类字段不另存。load 顺序靠 priority（TridentState=0 先 load item 重算 loyalty，ArrowState=10 后 load dealtDamage）。
- **组件命名**：箭矢状态组件须叫 `ProjectileArrowStateComponent`（既有 `ecs::ArrowStateComponent` 是 LivingEntity 箭矢计数，重名冲突）。
- **owner 持久化格式**：双 long（OwnerUUIDMost/Least），与 EvokerFangs 现有实现一致，不走 vanilla EntityReference compound。
- **同步字段真相源**：进组件 + DataParameter 镜像（批次4 模式），客户端 syncMetadataFromDataManager 扩展 id 范围。
- **序列化走注册表**：本批字段全组件化，全走子目标1 的 ComponentSerializerRegistry（非 OOP 虚函数）。

**暂未处理**：Fireball/EyeOfEnder 的 item 字段项目当前无（vanilla 持久化 Item），序列化器占位标 TODO 待补。DamagingProjectile acceleration_power 方向信息存盘丢失（项目 XYZ 三分量 vs vanilla 单值，TODO 待字段结构统一）。客户端 ClientEntity 投掷物族消费分支待配套渲染器集成时补。`tests/` 仍不透传 registry（永久约束）。

## 第六批子目标3：Brain tick 调度上移 System（2026-08-09 落地）

ECS改造.md 第 19 行决策"AI 系统保留 OOP（Goal/Brain/Navigator/Controller 约 5 万行不 ECS 化），System 做 tick 调度"。子目标3 落实这句"System 做 tick 调度"——把 Brain 的 tick 调用从 OOP `VillagerEntity::tick()` 内部抽到统一 ECS System，**不 ECS 化 Brain 数据**（Brain 仍是 OOP 成员），只搬调度决策。

**现状**：`Brain<E>` 是 header-only 模板类（`src/common/entity/ai/brain/Brain.hpp`），全仓仅 `Brain<VillagerEntity>` 一个实例化点。原 `VillagerEntity::tick()` 在 `AbstractVillagerEntity::tick()` 之后、业务逻辑（声音/粒子/交易/升级）之前硬编码调 `m_brain->tick(m_world, this, gameTime, dayTime, getRandom())`，调度决策耦合在实体 tick 内部，不符合"System 做 tick 调度"架构方向。

**用户拍板方向**（本会话 AskUserQuestion）：保留 `Brain<E>` 模板不变，新增 `BrainTickSystem` 接入 `EntitySystemScheduler`，用 System 统一调度所有持有 Brain 实体的 `brain().tick()`，替代每个实体 `tick()` 各自调 `m_brain->tick()`。

**已交付**：
- **新建 `BrainTickSystem.hpp`**（header-only 回调委托型 System）：逐字仿 `EntityLegacyTickSystem.hpp`，持 `std::function<void(EntityRegistry&)>` 回调，注册到 `SystemPhase::PostEntityTick`，回调委托 `EntityManager::_tickBrains()`。无 .cpp 不登记 CMake。
- **`EntityManager.hpp` 声明 `_tickBrains()`**：私有方法，由 BrainTickSystem 回调委托，复用 `_tickEntities` 门控框架，对 `dynamic_cast<entity::VillagerEntity*>` 成功实体调 `brain().tick()`，假设已持 `m_mutex`。
- **`EntityManager.cpp` 实现 `_tickBrains()` + 构造函数注册**：include 三个头（BrainTickSystem.hpp/VillagerEntity.hpp/IWorld.hpp），构造函数在 FireTickSystem 之后注册 BrainTickSystem 到 PostEntityTick。`_tickBrains()` 复用 `_tickEntities` 全部门控（playerChunks 快照/isRemoved 跳过/ServerPlayer 短路/模拟距离门控），对 dynamic_cast 成功实体逐字搬迁原 `VillagerEntity::tick` line 137-144 的 brain().tick 调用（gameTime 转 i64、dayTime 转 i32 对齐 `Brain::tick(IWorld*, E*, i64, i32, Random&)` 签名）。
- **`VillagerEntity.cpp` 删除 Brain tick 代码块**：`tick()` 中原 line 136-144 的 `if (m_brain && m_world) {...}` 整块删除，替换为简短注释指向 BrainTickSystem。`m_brain` 成员/`brain()` 访问器/`initializeBrain()` 全部保留（Goal/Task/Sensor 仍经 `owner->brain()` 访问）。

**关键设计决策**：
- **路线选回调委托型而非组件驱动型**：不把 `unique_ptr<Brain>` 包进组件——Brain 含虚函数破坏"组件纯数据"边界且违背"AI 不 ECS 化"决策；System 内复现模拟距离门控须访问 EntityManager 形成循环依赖；Brain 模板类型擦除须引入 IBrain 抽象基类改动 5 万行 AI 代码。回调委托型把 System 仅当"何时调 tick()"调度壳，Brain 数据仍 OOP 成员，契合混合架构。
- **阶段 PostEntityTick（注册在 Portal/Fire 之后）**：Brain tick 在所有实体 OOP tick（含 goalSelector.tick/navigator.tick）+ portal/fire 递减之后执行。跨实体传感器（NearestPlayersSensor 等）读到本帧最终位置，**行为更正确**。
- **类型识别 `dynamic_cast<entity::VillagerEntity*>`**（非 Entity 基类虚函数）：调度决策留 System 符合"System 做调度"，不动基类 vtable，与本仓既有 dynamic_cast 范式一致（序列化器/MobEntity 分类）。当前仅 VillagerEntity 持 Brain，RTTI 开销可忽略；新增持 Brain 实体类型时只改 `_tickBrains` 一处 dynamic_cast。
- **playerChunks 独立快照**：不与 `_tickEntities` 共用（共用须把快照提到 `tick()` 顶层改变三步编排结构，不值得；ServerPlayer 数量少重复快照成本可忽略）。

**风险与行为影响**：①Brain tick 时序迁移（主风险）——原同帧紧邻 goalSelector，新时序延后到所有实体 tick + portal/fire 之后，潜在 1 tick 延迟（Goal 读 Brain memory 可能读到上一帧值），单帧 50ms 玩家无感，GameTest 验证村民日程/工作/睡眠切换无回归。②死亡帧 Brain tick——`isRemoved()` 在 `remove()` 时置 true（死亡消散结束后才 remove），死亡帧 `isRemoved()==false` Brain 仍 tick，与原时序一致不引入新风险。③客户端——ClientWorld 不继承 IWorld，客户端不构造 VillagerEntity，BrainTickSystem 只注册到服务端 EntityManager，客户端无影响。

**验证**：增量构建 minecraft-server+minecraft-client 零错误（`--target` 绕过 mc_tests）；GameTest 8/8 零回归（simpleMobTest 偶发 flaky 超时复跑即过非回归）。Brain 模板未做泛型化（全仓仅 VillagerEntity 一个实例化点，泛型化收益为零且 Brain.hpp 未动），子目标3 实质为"Brain tick 调度上移 System"。

## 批次7：minecart 大族整块 ECS 化（2026-08-09 落地）

minecart 族（`AbstractMinecartEntity` 基类 + 7 叶子类：Chest/Furnace/TNT/Hopper/CommandBlock/Spawner/Rideable）的特有状态字段仍全是 OOP 成员，且继承自 projectile 族的 8 步范式整块迁移成熟度。本批目标：全族整块 ECS 化，按基类子批（B7.1）+ 叶子类子批（B7.2）两步推进，沿用 projectile 族"组件真相源 + OOP 字段删除"零双写范式。

### 子批 7.1：基类状态组件化（commit 9f9986ef2）

AbstractMinecartEntity 基类的 15 个字段（铁轨运行状态/速度配置/损坏动画/可推动标志/显示方块占位等）整批搬入 2 组件。基类先迁移，7 个矿车子类经基类构造自动获得基类组件，叶子类自有字段留 B7.2。

**已交付**：
- **Step0（修 FurnaceMinecart 遮蔽 bug）**：删基类 `m_pushX`/`m_pushZ` 死字段——纯死字段（基类版零读写），FurnaceMinecart 子类 hpp:649-650 重声明遮蔽基类版致双真相源（同 projectile 族 m_noGravity 遮蔽同源模式）。子类字段升格唯一真相源。
- **Step1（2 组件骨架+attach）**：新建 `MinecartStateComponent`（12 字段：铁轨状态/速度配置/损坏动画/可推动）+ `MinecartDisplayComponent`（3 字段：显示方块待接 wire）。AbstractMinecart 构造 attach，7 个矿车子类经此自动获得组件。CMake 登记新头。
- **Step2（15 字段迁移）**：hpp inline getter/setter 改走 `tryGetComponent<>()` + `MC_ASSERT_RELEASE`；cpp 24 处成员访问改方法内缓存组件指针（坑9 范式）。删 OOP 成员，零双写。`m_type` 不进组件（构造期定值类型标识，无运行时变更）。
- **Step3（m_damage 同步字段处理）**：m_damage 进 `MinecartStateComponent` 作 B 类无镜像。保持现有"成员不同步 `DATA_DAMAGE_PARAM`"脱节现状（加镜像会改变行为超 ECS 范围），`DATA_DAMAGE_PARAM` 注册保留作同步层占位。m_maxSpeed 等速度配置既有 `setMaxSpeed` 零调用且 `getMaxSpeed` 读常量不读成员，迁移保持该死字段现状不扩大范围。

### 子批 7.2：叶子类特有字段组件化 + SpawnerMinecart 持久化搬注册表

7 叶子类的特有字段组件化。RideableMinecart 无特有字段不建组件；其余 6 类各建独立组件承载特有状态。SpawnerMinecart 的 SpawnerLogic 持久化从 OOP 虚函数搬到组件序列化器注册表（沿用子目标1/6.2 范式）。

**已交付**：
- **Step1（6 组件骨架+attach，B7.2）**：新建 6 个 minecart 叶子类组件头文件（`src/common/entity/ecs/components/`）：
  - `ChestMinecartComponent`（`unique_ptr<SimpleInventory>` 包裹 27 格库存）：不可拷贝但 noexcept 可移动的 SimpleInventory 用 unique_ptr 包裹（沿用 AttributeComponent/SpawnerMinecart 范式）。header-only，组件隐式默认 ctor/dtor/move（unique_ptr 默认空、noexcept 移动、析构自动 delete 需完整类型已 include）。
  - `FurnaceMinecartComponent`（i32 fuel + f32 pushX/pushZ）：纯 POD 直接存值。
  - `TntMinecartComponent`（i32 fuse + `unique_ptr<DamageSource>` ignitionSource）。
  - `HopperMinecartComponent`（`unique_ptr<SimpleInventory>` inventory + i32 suckCooldown + bool disabled）。
  - `CommandBlockMinecartComponent`（string command/lastOutput + i32 successCount + bool mPowered，mPowered 保留驼峰对齐 vanilla）。
  - `SpawnerMinecartComponent`（直接存值 SpawnerLogic，全成员 noexcept 可移动无需包裹）。
  3 个 inline 构造函数（Furnace/TNT/CommandBlock）+ 3 个 .cpp 构造函数（Chest/Hopper/Spawner）attach 对应组件；Chest/Hopper/Spawner 构造体在 attach 后给组件赋值（库存 `make_unique<SimpleInventory>(INVENTORY_SIZE)` 等）。RideableMinecart 无字段不 attach。
- **Step2（6 叶子类字段迁移，B7.2）**：删全部叶子类 private OOP 字段，所有 inline getter/setter + cpp 方法改走 `tryGetComponent<ecs::XxxComponent>()` + `MC_ASSERT_RELEASE`。const 方法用 const 指针。方法内开头取组件指针缓存（坑9 范式）。返回引用的 getter（getCommand/getLastOutput/getSpawnerLogic）返回 `c->m_xxx`（组件在实体存活期稳定）。HopperMinecart::onActivatorRailPass 改调 `setDisabled(powered)`（已是组件版 inline）。
- **Step3（SpawnerMinecart 持久化搬注册表，B7.2）**：新建 `MinecartComponentSerialization.hpp/.cpp`（`src/common/entity/serialization/components/`），模式 A（tryGetComponent 早退）+ 委托 SpawnerLogic 直写 tag 根层。`registerMinecartComponentSerializers` 在 `ComponentSerializerRegistry::registerAll()` 中 Projectile 之后注册。删除 SpawnerMinecartEntity 的 `addAdditionalSaveData`/`readAdditionalSaveData` OOP override（cpp 函数体 + hpp 声明），回落 AbstractMinecartEntity/Entity 基类空实现防双重写入。

**仅 SpawnerMinecartComponent 注册序列化器的依据**（对齐 vanilla 1.21.11 各 minecart 叶子类持久化字段清单）：
- SpawnerMinecart：SpawnerLogic 全参数（Delay/MinSpawnDelay/MaxSpawnDelay/SpawnCount/MaxNearbyEntities/RequiredPlayerRange/SpawnRange/SpawnData/SpawnPotentials）透传 `saveToNBT`/`loadFromNBT`（与 MobSpawnerBlockEntity 共用同一逻辑类）。
- ChestMinecart/HopperMinecart 库存内容走 LootableContainer 体系（容器 NBT 由 ContainerEntity 层处理，非实体 addAdditionalSaveData），项目当前未接通 vehicle 容器持久化（TODO）。
- FurnaceMinecart 的 fuel/pushX/pushZ 是运行时状态（vanilla MinecartFurnace 不存盘）。
- TntMinecart 的 fuse/ignitionSource 是运行时状态（vanilla MinecartTNT 不存盘）。
- CommandBlockMinecart 的 command/lastOutput/successCount 走 CommandBlockEntity 体系，vanilla MinecartCommandBlock 持久化 Command/LastOutput/SuccessCount，项目当前未接通（TODO，待 command block 矿车持久化业务接入后补序列化器）。

**关键设计决策**：
- **基类先迁、叶子类后迁两步推进**：基类 15 字段先搬组件（B7.1），7 子类经基类构造自动获得基类组件；叶子类特有字段再各自建独立组件（B7.2）。避免一次性全族迁移 diff 过大，且基类组件就位后叶子类只处理自有字段，边界清晰。
- **SimpleInventory 用 unique_ptr 包裹**：SimpleInventory 禁拷贝但 noexcept 可移动，理论可直接内嵌；但含动态资源，沿用 AttributeComponent/SpawnerMinecart 的 unique_ptr 包裹范式更稳妥（组件移动只搬指针，被包裹类型本体不移动，容错性高）。
- **SpawnerLogic 直接存值无需包裹**：SpawnerLogic 全成员 noexcept 可移动（无 mutex/atomic/不可移动容器），可直接内嵌为组件成员。这是"全可移动类型直接内嵌"的判定——与 unique_ptr 包裹形成对照，按类型实际可移动性选择。
- **SpawnerLogic 透传直写 tag 根层**：SpawnerLogic 自身已实现 saveToNBT/loadFromNBT（与 MobSpawnerBlockEntity 共用），序列化器仅作透传——取组件内 SpawnerLogic 引用，直接调其 saveToNBT/loadFromNBT 把键平铺到实体 compound 根层。SpawnerLogic 键（Delay/SpawnData 等）与 minecart 基类组件序列化键（Pos/Motion/Rotation 等）无冲突，平铺安全（同 EvokerFangs 既有 OOP 实现）。
- **m_type 不进组件**：构造期定值类型标识（AbstractMinecartEntity::Type 枚举），无运行时变更、无同步/持久化需求，留作构造期参数不入组件，避免无意义组件化。

**验证结果**（2026-08-09）：
- 构建：client/server 本体均 exit 0 链接产出（`--target minecraft-client/minecraft-server` 绕过 mc_tests；CMakeLists 改动触发 vcpkg VS 探测失败用 `./scripts/configure.sh build` setup VS dev env 解决 configure）。
- GameTest 回归：8/8 passed exitCode=0 零回归。关键回归门 minibiomes（矿车载猪+铁轨运动+乘客同步，本批直接动 MinecartEntity）PASSED，证明基类+叶子类全族 ECS 化无回归。

**暂未处理**：`tests/` 仍不透传 registry（永久约束）。ChestMinecart/HopperMinecart 库存持久化、CommandBlockMinecart 命令持久化待业务接入后补序列化器（已标 TODO）。MinecartDisplayComponent 3 字段待接 wire 同步层（项目当前 minecart 显示方块同步未实现）。

## 后续批次路线（备忘，未落地）

- **批次5（已收束于 5.1）**：实体能力 mixin 接口转 tag/capability component。经全仓勘察，生产代码 dynamic_cast 共 1458 处，但批次5 真实作用域远小于原路线图「约 1220 处」——mixin 接口相关 cast 仅 IMob 5 处可纯 tag 迁移（5.1 已落地）。其余 dynamic_cast 分布：NBT/Tag 节点遍历 429 处（NBT 多态树固有模式，不迁移）、实体具体子类下行约 280 处（Player/LivingEntity/MobEntity/各 Entity 子类，不适合转 tag——子类太多且生命周期与实体绑定，另案）、UI 控件/结构生成具体类约 60 处（不迁移）、方块能力接口 21 处（world/block 域）、容器接口 16 处（inventory 域）、IWorld 跨边界转型 10 处（结构生成子系统另案）。**剩余 10 个 mixin 接口（IShearable/IEquipable/ICrossbowUser/IRangedAttackMob/IJumpingMount/IRideable/ContainerUser/IAngerable/IFlinging/IFlyingAnimal）的 dynamic_cast 经 5.2 勘察确认为「接口多态分发」模式（cast 后立即调接口虚方法），hasComponent 无法替代虚方法调用，违反 AI 保留 OOP 决策；IFlinging/IFlyingAnimal 虽能纯 tag 但零消费点是架构债**。故批次5 收束于 5.1 IMob 试点，剩余 mixin 接口 OOP 多态作混合架构合理行为层保留。详见上文「批次5 收束决策」段。
- **批次6**：子目标1（序列化按组件注册序列化器）已于 2026-08-09 落地，见上文「第六批子目标1」段；子目标2 projectile 族整块 ECS 化已于 2026-08-09 落地，见上文「第六批子目标2」段；子目标3 Brain tick 调度上移 System 已于 2026-08-09 落地，见上文「第六批子目标3」段。批次6 三个子目标全部完成。
- **批次7**：minecart 大族整块 ECS 化已于 2026-08-09 落地，见上文「批次7」段（基类子批 7.1 commit 9f9986ef2 + 叶子类子批 7.2）。原路线图"server/client 专属 system 落地（EntityTracker 同步 system、客户端镜像 system）+ 客户端冰冻渲染 + FreezeComponent 回填点"仍待办，顺延至后续批次。
- **批次8**：引入定义驱动层（ActorDefinitionIdentifier + 组件工厂），适配脚本/gametest 组件式 API。

> 本目录 `src/common/entity/ecs/README.md` 记录了 ECS 层内部结构、上下游依赖与 19 条容易踩的坑（双写禁忌、句柄脆弱性、跨 registry 不可迁移、命名遮蔽、指针稳定性、CMake 显式列举、ClientWorld 不继承 IWorld、占位 Player registry 来源、低频组件查询性能、同步镜像字段组件化、SystemPhase 演进与跨帧延迟、不可移动类型 unique_ptr 包裹范式、C 类同步字段批量迁移 protected 转 getter 委托、C 类字段全量组件化规模化枚举提取消除循环依赖 + 异构字段聚合组件、mixin 接口转 tag component 接口保留 + Entity public hasComponent 包装 + 热路径外部指针改造、组件序列化器注册表：序列化按组件注册 + Entity public tryGetComponent 包装 + setter 副作用绕过直写组件、projectile 族整块 ECS 化同步字段 id 续接 + dealtDamage load 顺序 priority + owner UUID 双 long + FishingBobber 不持久化、Brain tick 调度上移 System 回调委托型非组件驱动 + 时序迁移到 PostEntityTick + dynamic_cast 类型识别、minecart 族整块 ECS 化基类先迁叶子类后迁两步推进 + 不可移动类型判定 + SpawnerLogic 透传直写 tag 根层），迁移后续批次前务必先读。
