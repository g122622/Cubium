# ECS 层（实体数据组件化）

本目录是实体系统 OOP→ECS 迁移的**数据层**。实体系统整体采用**基岩版式混合架构**：保留 `Entity→LivingEntity→MobEntity→Player` 继承壳作句柄与 OOP 行为层（AI/序列化/特化逻辑暂留 OOP），状态数据搬入本目录的 entt 组件，行为按需抽成 System。设计参考基岩版 `Actor` 内嵌 `EntityContext` + 继承链 vftable 并存的模式。

> 架构决策与七项根本约束见 `docs/iterations/ECS改造.md`。本 README 只讲本目录的内部结构与坑位。

## 目录结构

```
src/common/entity/ecs/
├── context/                         # ECS 核心包装层
│   ├── EntityId.hpp                 # entt 实体句柄 + EntityIdTraits（64位：entity 32 + version 32）
│   ├── EntityRegistry.hpp/cpp       # entt::basic_registry<EntityId> 薄包装，每 IWorld 一个实例
│   └── EntityContext.hpp            # (EntityRegistry&, entt::registry&, EntityId) 三元组，组件查询入口
│
├── components/                      # 数据组件（纯数据，无虚函数）
│   ├── StateVectorComponent.hpp     # m_pos / m_posPrev / m_posDelta（首批，进 m_builtIn 缓存）
│   ├── VelocityComponent.hpp        # m_velocity（首批，进 m_builtIn）
│   ├── AABBShapeComponent.hpp       # m_aabb / m_bbDim（首批，进 m_builtIn）
│   ├── EntityRotationComponent.hpp  # m_rot / m_rotPrev（首批，进 m_builtIn）
│   ├── EntityOwnerComponent.hpp     # unique_ptr<Entity> 反向桥接（ECS→OOP）
│   ├── EntityUniqueIDComponent.hpp  # u64 持久投影（网络/存档/跨 registry）
│   ├── PortalComponent.hpp          # m_inPortal/m_portalTime/m_portalCooldown/m_portalPos（第二批，低频 try_get）
│   ├── FireComponent.hpp            # m_fire 燃烧/免疫计时（第二批，低频 try_get）
│   ├── PhysicsStateComponent.hpp    # m_onGround/m_fallDistance/m_collided*（第二批，高频进 m_builtIn）
│   ├── HurtStateComponent.hpp       # m_absorption/m_hurtTime/m_maxHurtTime/m_deathTime（第二批，仅 LivingEntity，低频 try_get）
│   ├── FreezeComponent.hpp          # m_ticksFrozen/m_isInPowderSnow（第二批，低频 try_get，m_ticksFrozen 同步真相源）
│   ├── HealthComponent.hpp          # m_health/m_lastHealth/m_healthSynced（第三批，仅 LivingEntity，m_health 同步真相源）
│   ├── EquipmentComponent.hpp       # m_equipment/m_lastEquipment/m_lastEquipmentInitialized（第三批，仅 LivingEntity，无同步单写）
│   ├── ArrowStateComponent.hpp      # m_arrowCount/m_stingerCount/m_arrowHitTimer（第三批，仅 LivingEntity，arrowCount/stingerCount 同步真相源）
│   ├── AttributeComponent.hpp       # unique_ptr<AttributeMap> 包裹属性表（第三批，仅 LivingEntity，不可移动类型包装范式）
│   ├── EntityFlagsComponent.hpp     # m_flags 位掩码（第四批，所有实体，DATA_FLAGS 退镜像）
│   ├── EntityStateComponent.hpp     # m_air/m_customName/m_customNameVisible/m_silent/m_noGravity/m_pose 6 字段聚合（第四批，所有实体，unique_ptr<ITextComponent> 包裹）
│   ├── PlayerScoreComponent.hpp     # m_score（第四批，仅 Player，DATA_PLAYER_SCORE 退镜像）
│   ├── MobFlagComponent.hpp         # 空 struct tag（第五批 5.1，仅 MonsterEntity attach，IMob 接口 tag 层）
│   ├── ProjectileOwnerComponent.hpp         # shooterUuid/shooterEntityId/leftShooter/lastDeflectedById/hasBeenShot（第六批 6.2，ProjectileEntity 基类 attach，全投掷物族）
│   ├── ProjectileArrowStateComponent.hpp    # AbstractArrow 13 字段含 dealtDamage（第六批 6.2，Arrow/SpectralArrow/Trident/Spear 共用；与 LivingEntity ArrowStateComponent 重名故加 Projectile 前缀）
│   ├── TridentStateComponent.hpp            # tridentStack(unique_ptr)/hitBlock/returning/hitBlockPos/loyaltyLevel/returningTicks（第六批 6.2，仅 Trident；dealtDamage 复用父类 ProjectileArrowStateComponent）
│   ├── DamagingProjectileComponent.hpp      # accelerationXYZ/damage（第六批 6.2，DamagingProjectile 子树=火球族）
│   ├── FireballStateComponent.hpp           # explosionPower/blue（第六批 6.2，Fireball+WitherSkull 共用）
│   ├── ArrowEffectsComponent.hpp            # color/glowing/effects(unique_ptr<vector>)（第六批 6.2，仅 ArrowEntity 药水箭）
│   ├── SpectralArrowComponent.hpp           # glowDuration（第六批 6.2，仅 SpectralArrow）
│   ├── ProjectileItemComponent.hpp          # itemStack(unique_ptr)（第六批 6.2，ProjectileItemEntity 子树+Spear 共用）
│   ├── PotionProjectileComponent.hpp        # lingering（第六批 6.2，仅 Potion）
│   ├── ExperienceBottleComponent.hpp        # experience（第六批 6.2，仅 ExperienceBottle）
│   ├── WindChargeStateComponent.hpp         # hasBurst/burstCenter/hasBurstCenter（第六批 6.2，仅 WindCharge）
│   ├── ShulkerBulletComponent.hpp           # target(指针)/targetUuid/direction/flightSteps/targetDelta（第六批 6.2，仅 ShulkerBullet）
│   ├── FireworkRocketComponent.hpp          # fireworkItem(unique_ptr)/flightTime/lifetime/lifeTime/shotFromCrossbow（第六批 6.2，仅 FireworkRocket）
│   ├── FishingBobberComponent.hpp           # 13 字段含 angler/caughtEntity/state 等（第六批 6.2，仅 FishingBobber，直接继承 Entity 无 ProjectileOwnerComponent）
│   ├── EvokerFangsComponent.hpp             # owner(指针)/ownerUuid/warmupDelay/sentAttackEvent/lifeTicks/clientSideAttackStarted（第六批 6.2，仅 EvokerFangs，直接继承 Entity 独立 owner 机制）
│   └── BuiltInEntityComponents.hpp  # 高频组件裸指针缓存（首批4+第二批PhysicsState=5指针，非组件，是 Entity 内缓存结构）
│
└── systems/                         # 系统层
    ├── ISystem.hpp                  # 系统接口
    ├── ITickingSystem.hpp           # 每 tick 系统接口（virtual void tick(EntityRegistry&)）
    ├── SystemPhase.hpp              # 命名阶段枚举（EntityTick / PostEntityTick）
    ├── EntitySystemScheduler.hpp/cpp # 阶段化编排器（EntityManager::tick 委托，阶段内预留 organizer 钩子）
    ├── EntityLegacyTickSystem.hpp   # 包装 Entity::tick() 虚函数链为系统，注册入 EntityTick 阶段
    ├── BrainTickSystem.hpp          # 第六批 6.3 回调委托型 System：VillagerEntity 的 brain().tick() 上移调度，注册入 PostEntityTick
    ├── PortalTickSystem.hpp/cpp     # 第二批真实 System：portal 计时/冷却/传送触发，注册入 PostEntityTick
    └── FireTickSystem.hpp/cpp       # 第二批真实 System：fire 递减/燃烧伤害/雨中扑灭，注册入 PostEntityTick
```

## 内部模块关系

```
IWorld（ServerWorld / ClientWorld）
  └─ 持有 ecs::EntityRegistry 成员            ← entityRegistry() 访问器
       │
       ├─ EntityManager::tick() 委托 scheduler ← 第二批接入（首批为孤儿骨架）
       │    └─ EntitySystemScheduler.tick(registry)
       │         ├─ EntityTick 阶段:  EntityLegacyTickSystem → _tickEntities() 遍历调 entity->tick()
       │         └─ PostEntityTick 阶段: PortalTickSystem / FireTickSystem / BrainTickSystem（真实业务 System，BrainTickSystem 回调委托 _tickBrains()）
       │
       └─ entt::basic_registry<EntityId>        ← 组件池存储
            │
            ├─ Entity 构造时（透传 registry&）：
            │    registry.create() → EntityId
            │    emplace<StateVector/Velocity/AABBShape/Rotation/Portal/Fire/PhysicsState/Freeze>()
            │    缓存 5 裸指针到 m_builtIn（首批4 + PhysicsState）
            │
            └─ EntityContext（Entity 内嵌）
                 └─ tryGetComponent<T>() / getOrAddComponent<T>() / ...
```

**双向桥接**：
- OOP→ECS：`Entity` 持 `unique_ptr<EntityContext>`，getter/setter 读写组件——高频走 `m_builtIn.*->m_*` 裸指针（首批4+PhysicsState），低频走 `m_entityContext->tryGetComponent<T>()`（Portal/Fire/Freeze/HurtState）。
- ECS→OOP：`EntityOwnerComponent` 持 `unique_ptr<Entity>`，System 遍历组件时反查 OOP 句柄调虚函数（PortalTickSystem 调 `canTeleport`/`onPortalTriggered`，FireTickSystem 调 `isInWater`/`hurt` 等）。

## 上下游依赖

- **上游（依赖）**：`entt`（vcpkg 安装，`#include <entt/entt.hpp>`）、`common/core/Types.hpp`（u64/u32/Vec3）、`common/util/AxisAlignedBB.hpp`、`common/util/math/Vector3.hpp`。
- **下游（被依赖）**：
  - `common/entity/core/Entity.hpp` — 内嵌 `EntityContext` + `BuiltInEntityComponents m_builtIn`。
  - `common/world/IWorld.hpp` — `virtual ecs::EntityRegistry* entityRegistry()`。
  - `common/world/entity/EntityManager.hpp` — 持 `ecs::EntityRegistry&` + `registry()` 访问器。
  - `server/world/ServerWorld.hpp` / `client/world/ClientWorld.hpp` — 各持 `ecs::EntityRegistry` 成员。
- **不依赖**：本目录不依赖 `ai/`、`registry/`、`serialization/`（OOP 层），保持数据层纯净。

## 组件工厂

未建独立 `factory/` 目录。组件 attach 内联在构造链内：
- **Entity 构造**：透传 `ecs::EntityRegistry&`，attach 10 组件（首批 4 高频 + 第二批 Portal/Fire/PhysicsState/Freeze + 第四批 EntityFlags/EntityState），缓存 5 高频裸指针到 `m_builtIn`。
- **LivingEntity 构造**：续接 attach 5 组件（第二批 HurtState + 第三批 Health/Equipment/ArrowState/Attribute）。**AttributeComponent 须在 `registerAttributes()` 之前 attach**——后者经 `attributes()` getter 取组件填充默认属性，时序颠倒则 getter 断言失败。
- **Player 构造**：续接 attach PlayerScoreComponent（第四批），须在 `registerData()` 之前——后者经 `getScore()` 取组件填默认值。Player 另重写 `setAbsorptionAmount`（基类改 virtual）下发 DATA_PLAYER_ABSORPTION_PARAM，无新组件（复用 HurtStateComponent）。
- **叶子类工厂** `EntityType::create(IWorld*, ecs::EntityRegistry&)`：构造 Entity 子类 → 返回 `unique_ptr<Entity>`。

差异化组件的工厂注入留待后续批次（见 `docs/iterations/ECS改造.md` 路线）。

## 容易踩的坑

### 1. 双写时序 bug（最高危，硬约束）

同一字段若同时存在于组件和 Entity 成员且都可写，会产生"同帧读到不一致值"的极隐蔽 bug。**首批4字段（position/velocity/aabb/rotation）迁移后，getter/setter 必须只读写 `m_builtIn` 组件指针，原成员字段已删除，绝无两份可写副本。** 后续批次迁移新字段时同样须遵守：要么字段删除改读组件，要么字段改为组件引用，不允许两份可写副本并存。

### 2. entt::entity 句柄脆弱性

`EntityId` 销毁后版本号变，跨帧持有需 `registry.valid()` 检查。版本号回绕有极端风险窗口（已通过 64 位 entity_type + 32 位 version 缓解，配合 `EntityUniqueIDComponent` 持久投影）。日常代码规范：
- 跨帧/跨系统持有实体引用，优先用 `EntityInstanceId`（u64 永不复用）或 `EntityUniqueIDComponent`，不用裸 `EntityId`。
- 用 `EntityId` 前若来源不确定，先 `registry.valid(id)` 校验。

### 3. entt 实体不可跨 registry 迁移（硬限制）

entt 实体与组件绑定在创建它的 registry 上，`entt::entity` 在不同 registry 间不通用。故 **Entity 构造签名必须透传 `ecs::EntityRegistry&`，构造时 registry 必须就位**（对齐基岩版 `Actor(ILevel&, EntityContext&)`）。「构造到临时 registry→addEntity 时迁到世界 registry」不可行。

### 4. 命名遮蔽：entity::EntityRegistry vs ecs::EntityRegistry

项目有两个 `EntityRegistry`：
- `mc::entity::EntityRegistry` — 实体**类型**注册表（单例，`EntityType` 的注册与查询）。
- `mc::ecs::EntityRegistry` — ECS 注册表（每世界一个，entt 包装）。

两者同名 `registry` 变量在同作用域会冲突（编译器报 `auto& registry` 与 `auto* registry` 重定义，或非限定名查找歧义）。**同函数内若两者都要用，ECS registry 变量改名 `ecsRegistry`**。已踩坑文件：`EndDragonFight.cpp`、`VillageSiege.cpp`、`NaturalSpawner.cpp`、`SummonCommand.cpp`、`IDispenseItemBehavior.cpp`、`TNTBlock.cpp`、`ZombieEntity.cpp`。

### 5. BuiltInEntityComponents 指针稳定性契约

`m_builtIn` 缓存的 4 裸指针指向 entt pool 内部数据。entt sparse_set 保证：组件 emplace 后只要不被 remove，其地址在 pool 生命周期内稳定（packed array 分页存储，不因其他实体增删而移动）。**前提：首批4组件一旦 attach 永不移除。** 后续批次若引入组件动态移除，须改用 `EntityContext` 查询或重设计缓存失效策略。

### 6. CMake 显式列举

`src/common/CMakeLists.txt` 是**显式逐文件列举**（非 glob 递归）。新增 `.cpp` 必须手动登记到 `mc_common` target，漏登记会导致链接时符号缺失（易误判为代码错误）。`.hpp` 无需登记。

### 7. ClientWorld 不继承 IWorld

`ClientWorld` 继承 `ICollisionWorld` + `IBlockAnimateContext`（不继承 `IWorld`），故其 `entityRegistry()` 是自有方法（非 `override`）。`ServerWorld` 继承 `IWorld`，`entityRegistry()` 是 `override`。两者签名一致但 override 关系不同，勿在 ClientWorld 加 `override`。

### 8. 占位 Player 的 registry 来源

无 world 上下文的临时占位 Player（ContainerManager / BlockInteractionManager / IntegratedServer 菜单 等）无法从 IWorld 取 registry，配**静态局部 `ecs::EntityRegistry`**（如 `static ecs::EntityRegistry s_menuRegistry{"menu"};`）。此类占位 Player 是临时方案，已加 TODO，后续应重构避免构造完整 Player 仅为传参。

### 9. 低频组件不进 m_builtIn，热路径须缓存局部指针

Portal/Fire/Freeze/HurtState 四组件走 `tryGetComponent<T>()` 查询（贴合基岩版极简缓存设计），不进 `m_builtIn` 裸指针缓存。单次 `try_get` 约 2-3ns 可接受，但**热路径方法（baseTick/move/actuallyHurt/tickFreeze 等）若同帧多次访问同一组件，须在方法开头取一次组件指针缓存局部变量复用**，避免重复哈希查找。PhysicsState 因 move/checkOnGround/updateFallDistance 及各 tick 40+ 处直接访问，破例进 `m_builtIn` 缓存。判定标准：单方法内访问 ≥3 次或被每帧调用的方法，考虑进缓存或局部指针。

### 10. 同步字段组件化：真相源在组件，DataParameter 退为镜像

含 DataParameter 的同步字段（如 `m_ticksFrozen` → `DATA_TICKS_FROZEN_PARAM`）组件化时，**FreezeComponent 成为真相源，DataParameter 退为同步镜像**。要点：
- `setTicksFrozen()` 内同时写组件（真相源）+ `m_dataManager.set()`（镜像），缺一则会真相源失真或网络断链。
- `syncMetadataFromDataManager()` **删除**从 DataParameter 回填组件的行——真相源不从镜像回填，否则双写时序错乱。
- NBT 反序列化统一走 setter（由 setter 完成双写），不直接写字段。
- **客户端回填坑**：客户端若 attach 同一组件但不跑产生该字段的 tick（如客户端不跑 tickFreeze），须在客户端 metadata 反序列化路径从 DataParameter 回填组件，否则客户端读组件恒为默认值。本项目 ClientEntity 是独立类不继承 mc::Entity、不 attach FreezeComponent、不消费 ticksFrozen，故 freeze 客户端回填点暂不存在；未来实现客户端冰冻渲染时需补此回填。

### 11. SystemPhase 演进路径与跨帧延迟

第二批阶段枚举为 `{ EntityTick, PostEntityTick }`：EntityTick 承载 `EntityLegacyTickSystem`（OOP `Entity::tick()` 桥接），PostEntityTick 承载 `PortalTickSystem`/`FireTickSystem`（状态递减/环境交互类真实 System）。**抽 System 到 PostEntityTick 会引入跨帧延迟 1 tick**：fire/portal 递减结果（`m_fire--`/`m_portalTime++`）下帧 baseTick 才读到。单帧 50ms 玩家无感，但涉及 hurt 时序的 System 须经回归验证覆盖。后续批次按业务时序新增阶段（如 Movement/AiStep），强时序内聚的逻辑（如 tickFreeze 调 hurt + 读 m_attributes）留 OOP 不抽 System。

### 12. 不可移动/不可拷贝类型用 unique_ptr 包裹进组件（第三批范式）

entt 组件池要求组件类型可移动（swap-and-pop 重排），故含不可移动成员的类型不能直接内嵌为组件。典型如 `AttributeMap`（含 `mutable std::mutex` + `unordered_map<string, unique_ptr<...>>`，不可移动/拷贝）。**解法：用 `std::unique_ptr<T>` 包裹**——`struct AttributeComponent { std::unique_ptr<AttributeMap> m_attributes; ... }`，组件移动只搬指针，被包裹类型本体不移动。这与"直接内嵌 + 依赖 entt pinned type（in_place_delete trait，永不 remove/sort/compact）运行时契约"相比，不引入首个 pinned 组件，容错性更高。后续遇含 mutex/atomic/不可移动容器的类型进组件，照此范式。

### 13. C 类同步字段批量组件化：protected 成员转 public getter 委托

第三批将 LivingEntity 的 health/equipment/arrows/attribute 四组字段搬入组件，其中 health/arrows 是 C 类同步字段（含 DataParameter），延续第二批 freeze 的"组件真相源 + DataParameter 镜像"模式。**大规模机械替换要点**：
- 原 `protected` 成员（如 `m_attributes`）被子类约 70 文件 320 处直接访问，删除成员后子类改调 `public` getter 委托（`m_attributes.xxx` → `attributes().xxx`）。getter 须提供 const + 非 const 双重载（const 方法如 `writeAdditionalSaveData` 调 const 版，read 方法调非 const 版）。
- inline getter 用了 `MC_ASSERT_RELEASE` 须显式 include `common/util/assert/AssertMacros.hpp`（基类 Entity.hpp 未传递，tests/ 包含时暴露 IWYU 违规）。
- 属性注册时序：基类 LivingEntity 构造期调 `registerAttributes()` 因虚函数不向下派发仅注册基类属性，叶子类构造体显式再调 `registerAttributes()` 经继承链逐层向上注册专属属性——组件化后 `registerAttributes()` 内 `attributes().xxx` 在叶子类构造体执行时基类已 attach 组件，时序成立。
- getter 返回引用（`AttributeMap&`），链式 `attributes().setBaseValue(...)` 语义与原 `m_attributes.setBaseValue(...)` 等价，顺序敏感的 remove+add 序列无副作用（同一组件同一对象）。

### 14. C 类字段全量组件化规模化：枚举提取消除循环依赖 + 异构字段聚合组件

第四批把 Entity 层剩余 7 个 C 类同步字段（flags/air/customName/customNameVisible/silent/noGravity/pose）+ Player score + Player absorption 下发全部组件真相源化，沿用第二批 freeze / 第三批 health/arrows 的同步镜像模式规模化。**规模化的几个新坑**：
- **枚举提取消除循环依赖**：`EntityFlags` 原内联于 `Entity.hpp`，新建 `EntityFlagsComponent.hpp` 若 include `Entity.hpp` 则与 `Entity.hpp` include 组件头形成循环。解法：把 `EntityFlags` 枚举 + 位运算符提取到独立头 `src/common/entity/core/EntityFlags.hpp`（参照 `EquipmentSlot` 提取先例），组件头只 include 这个轻量枚举头。`Entity.hpp` 删除内联定义改为 include `EntityFlags.hpp`，下游零改动。
- **异构字段聚合进单组件**：`EntityStateComponent` 聚合 6 个类型异构的轻量同步字段（`i32`/`unique_ptr<ITextComponent>`/3×`bool`/`EntityPose`），对齐基岩版 ActorDataSynched 组件族（轻量元数据聚合，`mc/entity/components/`）。聚合避免组件爆炸——若每字段一组件，Entity 层 10+ 组件徒增内存与查询开销。HealthComponent 已证明"同层多字段进单组件"可行，本批把模式推广到异构类型字段。
- **customName unique_ptr 组件存储**：组件存 `std::unique_ptr<text::ITextComponent>` 承接原 `m_customName` 类型（保留样式/颜色富文本），与 `AttributeComponent` 用 `unique_ptr<AttributeMap>` 包裹范式一致。DataParameter 仅存 `OptionalComponentValue`（present + 纯文本）作有损同步投影，组件与镜像并非全等。`setCustomNameComponent` 内须先 `c->m_customName = std::move(name)` 写组件，再从组件取 text 写 DataParameter，确保两份一致。组件头前向声明 `class ITextComponent` + include `<memory>`，不 include 全定义。
- **inline getter MC_ASSERT_RELEASE 守卫**：pose/flags/air 等 getter 走 `tryGetComponent + MC_ASSERT_RELEASE`，与第三批 `attributes()` 同模式。`Entity.hpp` 须显式 include `common/util/assert/AssertMacros.hpp`（基类不传递，IWYU 违规在 tests/ 暴露）。
- **pose refreshDimensions 副作用保留**：`setPose` 迁移后**必须保留 `refreshDimensions()` 调用**（潜行变矮/游泳变扁的碰撞箱更新）。`syncMetadataFromDataManager` 末尾原 `refreshDimensions`（服务端 pose 回填时触发的）随回填行删除，但 `setPose` 内的 `refreshDimensions` 保留。删回填行时勿误删 setPose 路径的副作用。
- **syncMetadataFromDataManager 清空留骨架**：删除全部 7 字段回填行后函数体空，保留空函数 + 注释「组件为真相源，不再从 DataParameter 回填」。ClientEntity 是独立类有自己的 syncMetadata 实现，仅删 Entity::syncMetadata 不影响客户端。
- **setAbsorptionAmount 改 virtual + Player override 下发**：absorption 真相源已在 `HurtStateComponent.m_absorption`（第三批），无需新组件。缺陷是原 `LivingEntity::setAbsorptionAmount` 非虚、不下发 `DATA_PLAYER_ABSORPTION_PARAM`。修复：基类改 `virtual`，Player `override` 重写——调基类写组件（含 clamp）后再 `m_dataManager.set(DATA_PLAYER_ABSORPTION_PARAM, absorptionAmount())` 下发 clamp 后值。基类 `actuallyHurt` 内调用经虚派发到 Player 版本，同步链路完整。这是"真相源已在组件、仅需打通下发"的轻量收口范式，区别于新建组件的重迁移。

### 15. mixin 接口转 tag component：接口保留 + Entity public hasComponent 包装 + 热路径外部指针改造

第五批把实体能力 mixin 接口（IMob/IShearable/IRideable 等）转 tag/capability component，**接口保留作 OOP 行为层**（虚函数不删），tag 作类型标记层，`dynamic_cast<接口*>` 改 `hasComponent<TagComponent>()`，二者并存对齐基岩版混合架构。子批 5.1（IMob→MobFlagComponent）落地的几个坑：
- **Entity public hasComponent 包装是外部指针改造的前置条件**：`m_entityContext` 是 Entity 的 **protected** 成员，无 public 访问器。4 处外部指针 dynamic_cast（AI goal/sensor、BlockEntity 等继承体系外调用方）无法访问 `other->m_entityContext`。解法：Entity.hpp public 段加 `template<class T> bool hasComponent() const { return m_entityContext->hasComponent<T>(); }` 透传包装。只暴露布尔查询不暴露整个 EntityContext（方案 A，权限可控；暴露整个 context 的方案 B 权限过大）。const 安全，`const LivingEntity*` 与 `const` 方法兼容。这是后续 5.2/5.3/5.4 及更远批次 tag 查询的统一外部入口。MobEntity.cpp:744 的 `this` 场景无需包装（派生类内 protected 可达，直接 `hasComponent<T>()` 调 public 包装即可）。
- **接口保留不删，tests/ 零改动**：IMob.hpp 不动，MonsterEntity 仍 `public IMob`。tag 只承担类型标记，行为层虚函数继续走 vftable。tests/ 9 处 `dynamic_cast<IMob*>`（4 文件）因此零改动——测试走 OOP 接口层，生产走 ECS tag 层，并存即设计意图。激进删除接口类会致 20+ 测试文件连锁崩溃，不可取。
- **空 struct tag 零内存开销**：entt 原生支持空 tag（`emplace<EmptyTag>` / `all_of<EmptyTag>` 编译期类型 id 比较），对齐基岩版 `Is*FlagComponent` 命名约定。MobFlagComponent 是空 struct，无数据字段。
- **IMob 唯一继承点单点 attach**：IMob 唯一继承点是 MonsterEntity（`class MonsterEntity : public CreatureEntity, public entity::IMob`），20+ 怪物子类经 MonsterEntity 间接获得 tag。只需在 MonsterEntity 构造 attach 一次，不在每个子类重复 attach。中间基类（AbstractSkeletonEntity/PatrollerEntity 等）也无需 attach。
- **热路径 RTTI 收益**：4 处外部指针 dynamic_cast 在每_tick 调用的 AI 谓词里（Sensors/AvoidHostileGoal/ConduitEntity/ShulkerGoals），`dynamic_cast` 走 RTTI 字符串比较，改 `hasComponent`（entt `all_of` 编译期类型 id 比较）收益显著。
- **Player 子类下行 cast 保留**：Sensors.cpp:615 改 IMob 判定为 hasComponent 后，同处的 `dynamic_cast<Player*>(other)`（判创造/旁观模式）保留——那是实体子类下行，子类太多且组件查询无法替代虚函数，另案处理，不在本批。

### 16. 组件序列化器注册表：序列化按组件注册 + Entity public tryGetComponent 包装 + setter 副作用绕过直写组件

第六批子目标1 把已组件化字段的 NBT 序列化从 OOP 虚函数链（`writeToNBT`/`addAdditionalSaveData` 逐层 super）搬到按组件注册的自由函数序列化器（`src/common/entity/serialization/components/`）。**几个关键坑**：
- **序列化器签名选 `Entity&` 非 `EntityContext&`**：序列化器必须调 setter（C 类字段 DataParameter 同步副作用是硬约束，绕过 setter 直写组件会丢网络同步）。setter 是 Entity 继承体系成员，经 `EntityContext` 无法调（不持 Entity 指针）。
- **Entity public tryGetComponent 包装**：序列化器经 `Entity&` 调，需直写组件内部（Pos/Rotation/OnGround 绕过 setter 副作用，Health 的 m_healthSynced 置位）。`m_entityContext` 是 protected，序列化器是自由函数无法访问。解法：Entity.hpp public 段加 `template<class T> T* tryGetComponent()` const/非 const 双重载透传（与第五批 hasComponent 包装配套）。注释限定"仅供序列化器/AI/BlockEntity 等横切关注点使用，业务逻辑走 getter/setter"。
- **3 字段绕过 setter 副作用直写组件**：Pos（`setPosition` 会污染 m_posPrev + 重建 AABB）、Rotation（`setRotation` 会污染 m_rotPrev）、OnGround（`setOnGround` 落地瞬间清 m_lastClimbPos）。原 `readFromNBT` 刻意直写 `m_builtIn.*` 组件绕过这些副作用，序列化器经 `tryGetComponent<T>()` 拿同一组件指针直写，语义完全一致（`m_builtIn.stateVector` 就是 `tryGetComponent<StateVectorComponent>()` 返回值）。**勿改调 setter**——会引入 prev 更新/AABB 重建/climbPos 清空副作用，破坏反序列化语义。
- **dynamic_cast 早退**：LivingEntity/Player 层序列化器经 `Entity&` 调用，内部 `dynamic_cast<LivingEntity*>`/`dynamic_cast<Player*>`。非目标类型实体返回 nullptr 早退无副作用。LivingEntity/Player 非 final、Entity 虚析构，RTTI 可用。序列化器按继承层分文件（EntityComponentSerialization/LivingEntityComponentSerialization/PlayerComponentSerialization），dynamic_cast 集中一处。
- **注册时机坑**：`VanillaEntities::registerAll()`（:132）用 `hasType(PIG)` 哨兵提前返回，故 `ComponentSerializerRegistry::registerAll()` 不能放 `registerAll()` 开头，须放 `doRegisterAll()` 末尾。`registerAll` 幂等（`m_registered` 标志 + clear 重注册，同 typeId 覆盖非追加），测试 EntityRegistry::clear 后重跑 doRegisterAll 顺带重注册序列化器。
- **FallFlying 跨层迁移**：原在 `LivingEntity::addAdditionalSaveData/readAdditionalSaveData` 处理（属 EntityFlags 位标志），本批上提为按 `EntityFlagsComponent` 注册的自由函数（仅依赖 Entity 基类 public 接口 isElytraFlying/addFlag/removeFlag）。须从 LivingEntity 同步删除避免重复写。
- **Health load 顺序依赖（已化解）**：`setHealth` 内 `clamp(0, maxHealth)` 读 AttributeMap，但 AttributeMap 在构造期 `registerAttributes` 已就位，非 NBT load 顺序依赖。本批不迁 Attributes（仍留 `readAdditionalSaveData` 虚函数内）。`loadAll` 在 `readAdditionalSaveData` 之前调，Health load 时 Attributes NBT 尚未读入，与原顺序一致。`Entry` 保留 `priority` 字段为未来迁 Attributes（priority=100）/ActiveEffects（priority=200）保证顺序。
- **存档格式不变**：保持 Java 版平铺格式，键名不变，零迁移成本旧存档兼容。`writeToNBT` 被 11 处复用（DataAccessor/EntityResolver/CopyNbtFunction/Template/NBTPredicate/PlayerResolver/EntityDeserializer），改造内部结构对调用方透明。

### 17. projectile 族整块 ECS 化：同步字段 id 续接 + dealtDamage load 顺序 priority + owner UUID 双 long + FishingBobber 不持久化

**同步字段 id 续接（沿用 FishingBobber 范式）**：投掷物族补齐对齐 vanilla 1.21.11 的 C 类 DataParameter 同步字段。`EntityClassInfo` 父链占位节点机制：无同步字段的中间基类（DamagingProjectile/AbstractFireball）补 `classInfo()` 占位节点（无 `registerData` override），供子类 `ClassRegisterGuard` 沿父链查找最高 id 时穿过续接。子类首字段续接到 id8（Entity 用 id0..7，ProjectileEntity 无同步字段）。不同类树（Fireball vs WitherSkull vs Firework vs AbstractArrow）id 各自从父链续接，同类树内唯一即可，客户端按实体类型分发不冲突（vanilla 行为）。C++ 虚函数在构造函数不派生，子类构造函数须显式调本类 `registerData()`。

**同步字段真相源进组件 + DataParameter 镜像（批次4 模式）**：AbstractArrow DATA_ARROW_FLAGS(u8 位标志 bit0=crit/bit2=shotFromCrossbow)/PIERCE_LEVEL/IN_GROUND、Trident DATA_LOYALTY/DATA_FOIL、FireworkRocket DATA_FIREWORKS_ITEM/ATTACHED_TO_TARGET/SHOT_AT_ANGLE、Fireball DATA_ITEM_STACK、WitherSkull DATA_DANGEROUS、EyeOfEnder DATA_ITEM_STACK。setter 同时写组件+镜像，`syncMetadataFromDataManager` 不回填。位标志字段（DATA_ARROW_FLAGS）用 `_syncArrowFlags()` 辅助聚合 crit+shotFromCrossbow 写回。

**dealtDamage 归属与 load 顺序 priority**：dealtDamage 是 ThrownTrident 的字段但复用父类 `ProjectileArrowStateComponent.m_dealtDamage`（vanilla AbstractArrow 也有此字段仅 Trident 用），不另存。load 顺序靠 priority：`TridentStateComponent`=0 先 load（读 Trident item 调 `setItemStack` 重算 loyalty），`ProjectileArrowStateComponent`=10 后 load（读 dealtDamage）。`loyalty` 不存盘（从 item 忠诚附魔重算，对齐 vanilla ThrownTrident）。

**owner UUID 双 long 格式（非 vanilla 1.21.11 单键）**：vanilla 1.21.11 已改用 `EntityReference` 单一 "Owner" 键。项目沿用 `OwnerUUIDMost/OwnerUUIDLeast` 双 long 格式（与既有 EvokerFangs/AreaEffectCloud 一致，零迁移成本），此为项目既有存档约定。owner 字段无 DataParameter 同步副作用，序列化器直写组件（与 EvokerFangs 既有 OOP 实现一致）。

**FishingBobber 不持久化（对齐 vanilla）**：vanilla `FishingHook` 两个 save 方法体全空（连 Owner 都不存）。项目 `FishingBobberComponent` 不注册序列化器。同理 ArrowEffects/SpectralArrow/Potion/ExperienceBottle/WindCharge 子类 vanilla 无自有 NBT 键（纯继承），不注册序列化器。

**OOP override 删除防双重写入**：EvokerFangs/FireworkRocket/Spear 既有 OOP `addAdditionalSaveData`/`readAdditionalSaveData` override 搬注册表后改空壳（保留 override 声明防未来误加字段）。`Entity::writeToNBT` 先 `saveAll`（注册表）再 `addAdditionalSaveData`（虚函数），删除子类 override 后回落 Entity 基类空实现，否则双重写入键冲突。

**ProjectileItemComponent 跨子树键名分发**：Spear（AbstractArrow 子树）用小写 "item" 键（对齐 vanilla AbstractArrow），ThrowableItemProjectile 子树（Snowball/Egg/Potion/ExperienceBottle/EnderPearl）用 "Item" 键。序列化器按 `dynamic_cast<AbstractArrowEntity*>` 分发键名。

**EyeOfEnder 断链特例**：vanilla EyeOfEnder 覆盖 save 且不调 super（只存 Item 不存 Owner）。项目 EyeOfEnderEntity 直接继承 Entity 无 ProjectileOwnerComponent，与 vanilla 断链语义一致（自然不存 owner）。当前无 item 字段，序列化器占位标 TODO。

### 18. Brain tick 调度上移 System：回调委托型非组件驱动 + 时序迁移到 PostEntityTick

ECS改造.md 第 19 行决策"AI 系统保留 OOP（Goal/Brain/Navigator/Controller 约 5 万行不 ECS 化），System 做 tick 调度"。`Brain<E>` 是 header-only 模板类，全仓仅 `Brain<VillagerEntity>` 一个实例化点。原调度决策耦合在 `VillagerEntity::tick()` 内部（`AbstractVillagerEntity::tick()` 之后硬编码调 `m_brain->tick()`）。子目标3 把这块调用从 OOP `tick()` 抽到统一 ECS System。

**回调委托型而非组件驱动型**：新建 `BrainTickSystem`（持 `std::function<void(EntityRegistry&)>` 回调），逐字仿 `EntityLegacyTickSystem.hpp`，注册到 `SystemPhase::PostEntityTick`（注册在 PortalTickSystem/FireTickSystem 之后），回调委托 `EntityManager::_tickBrains()`。不把 `unique_ptr<Brain>` 包进组件——Brain 含虚函数破坏"组件纯数据"边界，且违背"AI 不 ECS 化"决策；System 内复现模拟距离门控须访问 EntityManager 形成循环依赖；Brain 模板类型擦除须引入 IBrain 抽象基类改动 5 万行 AI 代码。回调委托型把 System 仅当"何时调用 tick()"的调度壳，Brain 数据仍 OOP 成员，契合混合架构。

**门控零重复复用**：`_tickBrains()` 复用 `_tickEntities()` 的全部门控框架——playerChunks 快照 + `isRemoved()` 跳过 + ServerPlayer 短路 + 模拟距离门控。`playerChunks` 独立快照不与 `_tickEntities` 共用（共用须把快照提到 `tick()` 顶层改变三步编排结构，不值得；ServerPlayer 数量少重复快照成本可忽略）。

**时序迁移（主风险）**：原 Brain tick 在 `VillagerEntity::tick` 内紧邻 `goalSelector.tick`/`navigator.tick`；新时序延后到所有实体 OOP tick + portal/fire 递减之后。**行为更正确**：跨实体传感器（NearestPlayersSensor 等）读到本帧最终位置而非中间状态。潜在 1 tick 延迟：Goal 读 Brain memory 可能读到上一帧值（原同帧紧邻），单帧 50ms 玩家无感。须 GameTest 验证村民日程/工作/睡眠切换无回归。

**类型识别用 `dynamic_cast<entity::VillagerEntity*>`**（非 Entity 基类虚函数）：①调度决策留 System 符合"System 做调度"，加虚函数 `tickBrain()` 是把调度下沉到 Entity 违背方向；②不动 Entity 基类 vtable；③与本仓既有 dynamic_cast 范式一致（序列化器/MobEntity 分类）。当前仅 VillagerEntity 持 Brain，RTTI 开销可忽略。新增持 Brain 实体类型时只改 `_tickBrains` 一处 dynamic_cast。

**死亡帧与客户端**：`isRemoved()` 在 `remove()` 时置 true（死亡消散结束后才 remove），死亡帧 `isRemoved()==false` Brain 仍 tick，与原时序一致不引入新风险。ClientWorld 不继承 IWorld，客户端不构造 VillagerEntity，BrainTickSystem 只注册到服务端 EntityManager，客户端无影响。
