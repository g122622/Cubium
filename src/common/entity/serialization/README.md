# Entity Serialization

实体序列化模块，负责实体与 NBT 数据之间的转换。

## 目录结构

```
src/common/entity/serialization/
├── EntityDeserializer.hpp/cpp          # 实体反序列化器
├── EntityNbtKeys.hpp                   # NBT 键名常量
├── EquipmentSlotNames.hpp/cpp          # EquipmentSlot 枚举名与 NBT 键名映射（MC 1.21.11 equipment 格式）
├── NbtHelper.hpp/cpp                   # NBT 辅助工具函数
├── components/                          # 组件序列化器注册表（批次6 子目标1）
│   ├── ComponentSerializerRegistry.hpp/cpp        # 注册表（进程单例，entt::type_id 键 + 裸函数指针）
│   ├── EntityComponentSerialization.hpp/cpp        # Entity 层 13 字段（Pos/Motion/Rotation/.../FallFlying）
│   ├── LivingEntityComponentSerialization.hpp/cpp  # LivingEntity 层 5 字段（Health/.../Equipment）
│   └── PlayerComponentSerialization.hpp/cpp        # Player 层 Score
└── README.md
```

## 内部模块关系

```
┌─────────────────────┐
│  EntityDeserializer │
└──────────┬──────────┘
           │
     ┌─────┴─────┐
     │           │
     ▼           ▼
┌─────────┐  ┌──────────────────┐
│  Nbt    │  │  EntityNbtKeys   │
│ Helper  │  │  (常量)          │
└────┬────┘  └──────────────────┘
     │                │
     ▼                ▼
┌─────────────────┐  ┌──────────────────────┐
│ nbt::tags 命名空间│  │ EquipmentSlotNames   │
│ (compound_tag等) │  │ (EquipmentSlot↔键名) │
└─────────────────┘  └──────────────────────┘
```

**依赖关系**：
- `EntityDeserializer` 依赖 `EntityRegistry` 获取实体类型
- `EntityDeserializer` 依赖 `Entity::readFromNBT()` 和 `Entity::writeToNBT()`
- `EntityDeserializer` 依赖 `NbtHelper` 读取 NBT 数据
- `EntityDeserializer` 依赖 `EntityNbtKeys` 获取键名常量
- `EntityDeserializer::attachPassengers` 依赖 `IWorld`（spawn 乘客、解析乘客 id）
- `EquipmentSlotNames` 依赖 `LivingEntity.hpp` 获取 `EquipmentSlot` 枚举定义
- `EquipmentSlotNames` 被 `LivingEntity` 的 `addAdditionalSaveData`/`readAdditionalSaveData` 使用

## 外部依赖关系

### 被依赖方

本模块被以下模块使用：

1. **存档系统** (`src/common/world/storage/`)
   - 实体数据持久化
   - 区块实体加载/保存

2. **网络同步** (`src/common/network/`)
   - 实体数据包序列化
   - 客户端实体状态同步

3. **命令系统** (`src/common/command/`)
   - `/summon` 命令实体生成
   - NBT 数据解析

### 依赖方

本模块依赖以下模块：

1. **实体核心** (`src/common/entity/core/`)
   - `Entity` 基类
   - `EntityType` 类型定义
   - `EntityRegistry` 注册表

2. **NBT 系统** (`src/common/util/nbt/`)
   - `compound_tag` 复合标签
   - `list_tag` 列表标签
   - NBT 上下文（Java 版格式）

3. **核心类型** (`src/common/core/`)
   - `Result<T>` 结果类型
   - `Error` 错误类型
   - `Types.hpp` 基本类型定义

4. **世界接口** (`src/common/world/`)
   - `IWorld` 世界访问接口

## 容易踩的坑

### 1. 实体类型未注册

```cpp
// 错误：实体类型未注册
auto result = EntityDeserializer::deserialize(tag);
// 如果 "id" 对应的类型未注册，返回 InvalidEntity 错误

// 正确：确保实体类型已注册
// 在游戏初始化时调用 VanillaEntities::registerAll()
```

### 2. 乘客实体的挂载流程

```cpp
// 反序列化阶段：deserialize() 不再 spawn 乘客，仅把 Passengers NBT 暂存到主实体的
// m_pendingPassengersNbt 字段。主实体此时尚未 spawn，id 仍为 0。
auto result = EntityDeserializer::deserialize(tag);
if (result.success()) {
    auto& entity = result.value();
    // entity->hasPendingPassengersNbt() == true（若 NBT 含 Passengers）

    // spawn 主实体：由 world 分配真实 id
    EntityId vehicleId = world.spawnEntity(std::move(entity));

    // 挂载乘客：attachPassengers 会递归 spawn 乘客并 startRiding
    // 此时主实体已有真实 id，乘客的 m_vehicle 会被正确设置为 vehicleId
    Entity* vehicle = world.getEntity(vehicleId);
    auto attachResult = EntityDeserializer::attachPassengers(*vehicle, world);
}
```

**设计要点：为什么不在 deserialize 阶段 spawn 乘客？**

`INVALID_ENTITY_ID == 0`，而主实体在 `deserialize` 返回时尚未 spawn 进世界，
其 `id()` 仍为构造时的 0。若此时 spawn 乘客并 `startRiding`，乘客的 `m_vehicle`
会被记为 `vehicle.id() == 0`。后续主实体 spawn 时 id 改写为真实值，乘客的
`m_vehicle` 仍为 0，骑乘关系失效。

解决方案对齐 MC Java 的 `EntityType.loadPassengersRecursive` 模式：反序列化阶段
仅构造实体对象树（暂存 Passengers NBT），主实体被 `spawnEntity` 注入世界、拿到
真实 id 后，再由 `attachPassengers` 递归 spawn 乘客并 `startRiding`。这样保证
每一层乘客的 `m_vehicle` 都指向上一层的真实 id。

**调用方集成：** `ServerWorld::onChunkLoaded` 在 `spawnEntity` 之后自动调用
`attachPassengers`，覆盖 Java 存档路径（`JavaColumnReader`）和 Native 存档路径
（`EntityStorageManager`）。

**序列化：** `Entity::writeToNBT` 在 `hasPassengers()` 时递归写入 Passengers
列表（对齐 MC Java `saveWithoutId`）。`EntityStorageManager::saveEntity` 跳过
`isRiding()` 的实体（乘客不单独落盘，作为载具 Passengers 标签的一部分被保存），
对齐 MC Java `Entity.save` 中 `isPassenger` 返回 false 的行为。

### 3. 压缩格式

```cpp
// EntityDeserializer 使用 gzip 压缩格式
// 与 PlayerSaveData 保持一致
// 解压时先尝试解压，失败则尝试直接解析

// zlib 压缩级别：Z_BEST_COMPRESSION (9)
// 提供最佳压缩率，但压缩速度较慢
```

### 4. NBT 键名大小写

```cpp
// MC 1.16.5 Java 版 NBT 键名区分大小写
// 使用 EntityNbtKeys.hpp 中的常量，避免硬编码

// 正确
tag.put(nbt_keys::ID, "minecraft:pig");

// 错误：大小写错误
tag.put("ID", "minecraft:pig");  // 应该是 "id"
tag.put("uuid", "...");          // 应该是 "UUID"
```

### 5. 递归乘客加载

```cpp
// deserialize() 遇到 Passengers 标签时，不立即 spawn 乘客，
// 而是把 Passengers NBT 暂存到主实体的 m_pendingPassengersNbt。
// 调用方在 spawn 主实体后调用 attachPassengers 递归处理：
//   - 对每个乘客 NBT 调用 deserialize 构造乘客实体
//   - 调用 world.spawnEntity 把乘客注入世界（拿到真实 id）
//   - 调用 passenger.startRiding(vehicle) 建立骑乘关系
//   - 递归处理乘客自身的 m_pendingPassengersNbt（多层骑乘）

// 如果乘客反序列化失败，attachPassengers 返回错误
// 已挂载的乘客不会被回滚（与 MC Java 行为一致）
```

### 6. 空指针处理

```cpp
// deserialize() 不再接收 IWorld 参数
// 乘客 spawn 延迟到 attachPassengers 阶段，由调用方传入 world

// EntityType::create() 返回 std::unique_ptr<Entity>
// 需要检查返回值是否为 nullptr
```

### 7. 二进制格式兼容性

```cpp
// deserializeFromBinary() 使用 Java 版 NBT 格式
// nbt::contexts::java

// 如果需要支持基岩版格式，需要：
// 1. 检测输入数据格式
// 2. 使用对应的上下文（bedrock_disk 或 bedrock_net）
// 3. 处理键名差异（基岩版使用不同的键名）
```

## EquipmentSlotNames

`EquipmentSlotNames` 提供 `EquipmentSlot` 枚举与 NBT 键名之间的双向映射，用于 MC 1.21.11 新格式的装备序列化。

### 映射关系

| EquipmentSlot | NBT 键名 |
|---|---|
| `MainHand` | `"mainhand"` |
| `OffHand` | `"offhand"` |
| `Feet` | `"feet"` |
| `Legs` | `"legs"` |
| `Chest` | `"chest"` |
| `Head` | `"head"` |
| `Body` | `"body"` |
| `Saddle` | `"saddle"` |

> **注**：`EquipmentSlot::Saddle` 对应 MC 1.21.11 `CopperGolem.EQUIPMENT_SLOT_ANTENNA`，用于铜傀儡天线槽（持有罂粟花）。玩家不使用此槽位。

### 使用场景

MC 1.21.11 中，`LivingEntity` 的装备数据以 `equipment` 复合标签存储，键名为 `EquipmentSlot` 的枚举名称：

```nbt
equipment: {
    head: { id: "minecraft:diamond_helmet", count: 1, ... },
    chest: { id: "minecraft:diamond_chestplate", count: 1, ... },
    offhand: { id: "minecraft:shield", count: 1, ... }
    // 空槽位省略
}
```

`EquipmentSlotNames::toName()` 和 `EquipmentSlotNames::fromName()` 用于此格式中 `EquipmentSlot` 与字符串键名的互转。

### 设计说明

- 头文件 `EquipmentSlotNames.hpp` 仅前向声明 `EquipmentSlot`，避免循环包含
- 实现文件 `EquipmentSlotNames.cpp` 包含 `LivingEntity.hpp` 获取完整枚举定义
- `fromName()` 对未知键名返回 `std::nullopt`，便于静默跳过无效数据

## 组件序列化器注册表（批次6 子目标1）

`components/ComponentSerializerRegistry` 把已 ECS 组件化的实体字段的 NBT 序列化逻辑，从 OOP 虚函数链（`writeToNBT`/`addAdditionalSaveData` 逐层 super）搬到按组件注册的自由函数序列化器。对齐基岩版 `InternalComponentRegistry`（`unordered_map<组件名, {save,load,legacy-convert}>` 静态注册表，与 `addAdditionalSaveData` 虚函数并存）。

### 设计要点

- **键用 `entt::type_id<T>().hash()`**（编译期类型安全）而非基岩版的 `HashedString`。项目单二进制无需跨进程稳定字符串标识，类型错配编译期即报错。
- **裸函数指针** `SaveFn`/`LoadFn`（无状态闭包，零分配）而非 `std::function`。
- **`std::vector<Entry>`** 而非 `unordered_map`（仅 13 条目，cache 友好，可按 priority 排序）。
- **序列化器签名 `Entity&` 非 `EntityContext&`**：序列化器必须调 setter（C 类字段 DataParameter 同步副作用是硬约束，绕过 setter 直写组件会丢网络同步）。setter 是 Entity 继承体系成员，只能经 `Entity&` 调。
- **存档格式不变**：保持 Java 版平铺格式（Pos/Motion/Health 等直接在根 tag），不走基岩版 `internalComponents` 命名空间隔离，零迁移成本旧存档兼容。
- **注册时机**：`VanillaEntities::doRegisterAll()` 末尾调 `registerAll()`（PIG 哨兵致 `registerAll()` 早退，故放 `doRegisterAll` 内部）。`registerAll` 幂等（`m_registered` 标志 + clear 重注册，同 typeId 覆盖非追加）。

### 字段访问策略

13 个序列化器对，按承载组件注册，覆盖 19 字段：

| 组件 | 字段 | 读写路径 |
|---|---|---|
| StateVectorComponent | Pos | `tryGetComponent` 直写 m_pos（绕过 setPosition 副作用：不污染 m_posPrev/不重建 AABB） |
| VelocityComponent | Motion | `setVelocity`（纯直写无副作用，等价） |
| EntityRotationComponent | Rotation | `tryGetComponent` 直写 m_rot（绕过 setRotation 副作用：不污染 m_rotPrev）+ setYHeadRot/setYBodyRot |
| PhysicsStateComponent | FallDistance + OnGround | FallDistance 走 `setFallDistance`（纯直写）；OnGround 直写组件（绕过 setOnGround 落地清 climbPos 副作用） |
| FireComponent | Fire | `setRemainingFireTicks`（纯直写） |
| PortalComponent | PortalCooldown | `setPortalCooldown`（纯直写） |
| FreezeComponent | TicksFrozen | `setTicksFrozen`（写组件 + DataParameter 同步） |
| EntityStateComponent | Air + CustomName + CustomNameVisible + Silent + NoGravity | 走对应 setter（含 DataParameter 同步） |
| EntityFlagsComponent | FallFlying | `isElytraFlying` 读 + `addFlag`/`removeFlag` 写（从 LivingEntity 层上提） |
| HealthComponent | Health | `setHealth`（clamp(0,maxHealth)）+ 置 m_healthSynced=true 避免首帧覆盖 |
| HurtStateComponent | Absorption + HurtTime + DeathTime | Absorption 走 `setAbsorptionAmount`（virtual，Player override 下发镜像）；HurtTime/DeathTime 无 setter 直写组件 |
| EquipmentComponent | Equipment | `getEquipment`/`setEquipment`（virtual，Player 派发到 PlayerInventory），含旧格式 HandItems/ArmorItems 回退 |
| PlayerScoreComponent | Score | `getScore`/`setScore`（同步 DATA_PLAYER_SCORE_PARAM 镜像） |

**核心取舍**：能调 public setter 的优先调（保留 DataParameter 同步副作用）。3 个字段（Pos/Rotation/OnGround）现行 `readFromNBT` 刻意绕过 setter 副作用直写 `m_builtIn.*` 组件，序列化器经 public `tryGetComponent<T>()` 拿同一组件指针直写，语义完全一致（`m_builtIn.stateVector` 就是 `tryGetComponent<StateVectorComponent>()` 返回值）。

### dynamic_cast 早退

LivingEntity/Player 层序列化器经 `Entity&` 调用，内部 `dynamic_cast<LivingEntity*>`/`dynamic_cast<Player*>`。非目标类型实体（ItemEntity/ItemFrame 等调 LivingEntity 序列化器）返回 nullptr 早退，无副作用。LivingEntity/Player 均 非 final，Entity 虚析构，RTTI 可用。

### writeToNBT/readFromNBT 改造后结构

`Entity::writeToNBT`：① 纯 OOP 基类字段（UUID/Invulnerable/Glowing/Tags）直写 → ② `saveAll(*this, tag)` 注册表遍历写 19 组件字段 → ③ `addAdditionalSaveData(tag)` 虚函数（剩余纯 OOP 字段：HurtByTimestamp/ActiveEffects/Attributes；Player 的 GameMode/Food/XP/Inventory/...；MobEntity 全层）→ ④ Passengers 递归。

`Entity::readFromNBT` 对称：① 纯 OOP 基类字段直读 → ② `loadAll(*this, tag)` 按 priority 升序读 19 组件字段 → ③ `reapplyPosition()` 重建 AABB → ④ `readAdditionalSaveData(tag)` 虚函数。

### load 顺序依赖

本批 19 字段间无依赖。Health/Absorption 的 `setHealth` 内 `clamp(0, maxHealth)` 读 AttributeMap，但 AttributeMap 在构造期 `registerAttributes` 已就位（派生类 MAX_HEALTH 默认值），非 NBT load 顺序依赖。本批不迁 Attributes（仍留 `readAdditionalSaveData` 虚函数内）。`loadAll` 在 `readAdditionalSaveData` 之前调，Health load 时 Attributes NBT 尚未读入，与原顺序一致。`Entry` 保留 `priority` 字段为未来扩展（Attributes priority=100 / ActiveEffects priority=200 保证顺序），`loadAll` 按 priority 升序遍历，`saveAll` 无序。

## 参考

- MC 1.21.11 `net.minecraft.world.entity.LivingEntity.addAdditionalSaveData()`
- MC 1.21.11 `net.minecraft.world.entity.LivingEntity.readAdditionalSaveData()`
- MC 1.21.11 `net.minecraft.world.entity.EquipmentSlot`
- MC 1.21.11 `net.minecraft.world.entity.player.Inventory.save()`
- MC 1.16.5 `net.minecraft.entity.EntityType.loadEntityAndExecute()`
- MC 1.16.5 `net.minecraft.entity.Entity.writeWithoutTypeId()`
- MC 1.16.5 `net.minecraft.entity.Entity.read(CompoundNBT)`
- MC 1.16.5 `net.minecraft.nbt.CompoundNBT`
