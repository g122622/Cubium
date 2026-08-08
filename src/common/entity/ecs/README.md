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
│   ├── StateVectorComponent.hpp     # m_pos / m_posPrev / m_posDelta（替代 Entity::m_position 等）
│   ├── VelocityComponent.hpp        # m_velocity
│   ├── AABBShapeComponent.hpp       # m_aabb / m_bbDim
│   ├── EntityRotationComponent.hpp  # m_rot / m_rotPrev（yaw/pitch）
│   ├── EntityOwnerComponent.hpp     # unique_ptr<Entity> 反向桥接（ECS→OOP）
│   ├── EntityUniqueIDComponent.hpp  # u64 持久投影（网络/存档/跨 registry）
│   └── BuiltInEntityComponents.hpp  # 4 高频组件裸指针缓存（非组件，是 Entity 内的缓存结构）
│
└── systems/                         # 系统层（首批仅编排器 + 旧 tick 包装）
    ├── ISystem.hpp                  # 系统接口
    ├── ITickingSystem.hpp           # 每 tick 系统接口（virtual void tick(EntityRegistry&)）
    ├── SystemPhase.hpp              # 命名阶段枚举（PreMovement/Movement/PostMovement/AiStep/Reset）
    ├── EntitySystemScheduler.hpp/cpp # 阶段化编排器（阶段内预留 organizer 钩子）
    └── EntityLegacyTickSystem.hpp   # 包装现有 Entity::tick() 虚函数链为系统，注册入编排器
```

## 内部模块关系

```
IWorld（ServerWorld / ClientWorld）
  └─ 持有 ecs::EntityRegistry 成员            ← entityRegistry() 访问器
       │
       ├─ EntityManager 内部委托 registry       ← addEntity 时 registry.create() + attach ActorOwnerComponent
       │
       └─ entt::basic_registry<EntityId>        ← 组件池存储
            │
            ├─ Entity 构造时（透传 registry&）：
            │    registry.create() → EntityId
            │    emplace<StateVector/Velocity/AABBShape/Rotation>()
            │    缓存 4 裸指针到 m_builtIn（BuiltInEntityComponents）
            │
            └─ EntityContext（Entity 内嵌）
                 └─ tryGetComponent<T>() / getOrAddComponent<T>() / ...
```

**双向桥接**：
- OOP→ECS：`Entity` 持 `unique_ptr<EntityContext>`，getter/setter（`position()`/`setPosition()`/`velocity()` 等）读写 `m_builtIn.*->m_*` 组件裸指针。
- ECS→OOP：`EntityOwnerComponent` 持 `unique_ptr<Entity>`，系统遍历组件时可反查 OOP 句柄。

## 上下游依赖

- **上游（依赖）**：`entt`（vcpkg 安装，`#include <entt/entt.hpp>`）、`common/core/Types.hpp`（u64/u32/Vec3）、`common/util/AxisAlignedBB.hpp`、`common/util/math/Vector3.hpp`。
- **下游（被依赖）**：
  - `common/entity/core/Entity.hpp` — 内嵌 `EntityContext` + `BuiltInEntityComponents m_builtIn`。
  - `common/world/IWorld.hpp` — `virtual ecs::EntityRegistry* entityRegistry()`。
  - `common/world/entity/EntityManager.hpp` — 持 `ecs::EntityRegistry&` + `registry()` 访问器。
  - `server/world/ServerWorld.hpp` / `client/world/ClientWorld.hpp` — 各持 `ecs::EntityRegistry` 成员。
- **不依赖**：本目录不依赖 `ai/`、`registry/`、`serialization/`（OOP 层），保持数据层纯净。

## 组件工厂

首批未建独立 `factory/` 目录。组件 attach 内联在 `EntityType::create(IWorld*, ecs::EntityRegistry&)` 工厂链内：构造 `Entity` 子类（其构造函数已透传 registry 并 attach 4 高频组件）→ 返回 `unique_ptr<Entity>`。差异化组件的工厂注入留待后续批次（见 `docs/iterations/ECS改造.md` 路线）。

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
