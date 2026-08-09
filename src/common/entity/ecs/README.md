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
│   └── BuiltInEntityComponents.hpp  # 高频组件裸指针缓存（首批4+第二批PhysicsState=5指针，非组件，是 Entity 内缓存结构）
│
└── systems/                         # 系统层
    ├── ISystem.hpp                  # 系统接口
    ├── ITickingSystem.hpp           # 每 tick 系统接口（virtual void tick(EntityRegistry&)）
    ├── SystemPhase.hpp              # 命名阶段枚举（EntityTick / PostEntityTick）
    ├── EntitySystemScheduler.hpp/cpp # 阶段化编排器（EntityManager::tick 委托，阶段内预留 organizer 钩子）
    ├── EntityLegacyTickSystem.hpp   # 包装 Entity::tick() 虚函数链为系统，注册入 EntityTick 阶段
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
       │         └─ PostEntityTick 阶段: PortalTickSystem / FireTickSystem（真实业务 System）
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
