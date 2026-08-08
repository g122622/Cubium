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

## 后续批次路线（备忘，未落地）

- **批次2**：projectile 族整块 ECS 化试点（验证创建→tick→同步→销毁全链路）。
- **批次3**：mob 基础数据（health/attribute/equipment）组件化。
- **批次4**：同步体系重构为 SynchedData 中间层（替换 EntityDataManager，保留 vanilla ID 分配语义）。
- **批次5**：11 个混入接口转 tag/capability component；约 1220 处 dynamic_cast 改组件查询。
- **批次6**：序列化按组件注册序列化器；Brain 模板实例化重构为泛型 System。
- **批次7**：server/client 专属 system 落地（EntityTracker 同步 system、客户端镜像 system）。
- **批次8**：引入定义驱动层（ActorDefinitionIdentifier + 组件工厂），适配脚本/gametest 组件式 API。

> 本目录 `src/common/entity/ecs/README.md` 记录了 ECS 层内部结构、上下游依赖与 8 条容易踩的坑（双写禁忌、句柄脆弱性、跨 registry 不可迁移、命名遮蔽、指针稳定性、CMake 显式列举、ClientWorld 不继承 IWorld、占位 Player registry 来源），迁移后续批次前务必先读。
