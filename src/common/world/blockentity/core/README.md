# 方块实体核心模块 (core)

方块实体系统的基础设施组件，提供方块实体的注册、反序列化、锁定、战利品表填充、背包实现等功能。

## 目录结构

```
core/
├── BlockEntityRegistry.hpp/cpp      # 方块实体注册表（工厂方法创建实例）
├── BlockEntityDeserializer.hpp/cpp  # NBT反序列化器（从存档数据创建实例）
├── LockableBlockEntity.hpp/cpp      # 可锁定容器基类（支持钥匙锁定和自定义名称）
├── LootableContainerBlockEntity.hpp/cpp # 可填充战利品表的容器基类
├── SimpleInventory.hpp/cpp          # 通用背包实现（用于箱子/漏斗等容器）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       │
ContainerBlockEntity (父模块容器基类)
       ↑
       │
LockableBlockEntity ←── SimpleInventory（组合关系，子类使用）
       ↑
       │
LootableContainerBlockEntity
       ↑
       │（继承：父模块storage/transport/processing下的具体方块实体）
       │
  ChestEntity, HopperEntity, FurnaceEntity等

BlockEntityRegistry ──创建──→ BlockEntity（及其子类）
BlockEntityDeserializer ──反序列化──→ BlockEntity（通过Registry创建）
```

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- **storage/** - `ChestEntity`, `TrappedChestEntity`, `BarrelEntity`, `ShulkerBoxEntity` 继承 `LootableContainerBlockEntity`
- **transport/** - `HopperEntity` 继承 `LockableBlockEntity`
- **processing/** - `AbstractFurnaceEntity`, `BrewingStandEntity` 继承 `LockableBlockEntity`
- **interactive/** - `DispenserBlockEntity` 继承 `LootableContainerBlockEntity`
- **spawner/** - `MobSpawnerBlockEntity` 继承 `BlockEntity`，通过 `BlockEntityRegistry` 注册
- **world/chunk/** - 区块加载时通过 `BlockEntityDeserializer` 反序列化方块实体
- **server/** - 服务器启动时调用 `BlockEntityRegistry::registerBuiltinTypes()`

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/ContainerBlockEntity.hpp` - 容器方块实体基类
- `world/blockentity/BlockEntityType.hpp` - 方块实体类型枚举
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/loot/LootTableManager.hpp` - 战利品表管理器（填充战利品）
- `resource/ResourceLocation.hpp` - 资源位置
- `util/nbt/Nbt.hpp` - NBT 序列化
- `core/Result.hpp` - 结果类型

## 容易踩的坑

### 1. 忘记设置 SimpleInventory 变更通知

修改 `SimpleInventory` 的数据后不会自动通知方块实体保存，必须设置回调或注册监听器：
```cpp
// 方式一：设置变更回调（兼容旧接口）
m_inventory(27, [this]() { setChanged(); })

// 方式二：注册 ContainerListener（推荐，支持多监听者）
m_inventory.addListener(&myListener);

// 错误：忘记设置通知，数据不会保存
m_inventory(27)
```

### 2. 锁定钥匙匹配逻辑

钥匙匹配使用物品的**显示名**而非物品ID。`canOpen()` 检查的是 `heldItem.getCustomName() == m_lockKey`。

### 3. 战利品表延迟填充机制（_unpackLootTable）

`LootableContainerBlockEntity` 实现了 MC 的延迟填充机制（参考 `RandomizableContainerBlockEntity`）：

- **结构生成时**：调用 `setLootTable()` 设置战利品表 ID 和种子
- **首次访问时**：通过 `_unpackLootTable(player)` 自动触发填充，填充后清除标记（幂等性）
- **自动触发的方法**：`isEmpty()`、`clearContainer()`、`openContainer(player)`
- **ShulkerBoxEntity 的 IInventory 方法**：`getItem()`、`setItem()`、`removeItem()`、`removeItemNoUpdate()`、`clear()` 也调用 `_unpackLootTable(nullptr)`

`_unpackLootTable` 使用 `const_cast` 从 const 方法中修改 `m_hasLootTable`/`m_lootFilled`（`m_lootFilled` 已声明为 `mutable`），这是安全的，因为填充战利品是缓存初始化而非逻辑状态变更。子类在实现 `IInventory` 接口方法时，应在操作前调用 `_unpackLootTable(nullptr)`。

填充通过 `IWorld::lootTableManager()` 获取战利品表管理器，只有 ServerWorld 返回有效指针。`fillWithLoot()` 和 `fillWithLootFromTable()` 是内部实现，子类无需重写。

#### SimpleInventory 战利品感知回调（推荐方式）

为了避免每个 `LootableContainerBlockEntity` 子类都重写一遍 `IInventory` 接口（如 `ShulkerBoxEntity` 那样的多重继承方式），项目在 `SimpleInventory` 上提供了战利品表延迟填充回调机制：

```cpp
// 子类构造函数中注入回调
BarrelEntity::BarrelEntity(const BlockPos& pos)
    : LootableContainerBlockEntity(BlockEntityType::Barrel, pos)
    , m_inventory(BARREL_SIZE)
{
    m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
}
```

`_makeLootUnpackCallback()` 是 `LootableContainerBlockEntity` 提供的 protected 方法，返回一个绑定 `this` 的 `std::function<void()>`，内部调用 `_unpackLootTable(nullptr)`。

设置回调后，`SimpleInventory` 的 `isEmpty`、`getItem`、`setItem`、`removeItem`、`removeItemNoUpdate`、`clear` 方法会在执行前自动触发回调，使所有通过 `getInventory()->...` 路径访问容器内容的代码（红石比较器、漏斗、`/loot` 命令、方块破坏等）都能正确触发延迟填充。

**移动语义注意**：子类如果实现移动构造/移动赋值，必须重新调用 `setLootUnpackCallback(_makeLootUnpackCallback())` 绑定新的 `this` 指针（参考 `ChestEntity` 的移动构造实现）。`ShulkerBoxEntity` 因多重继承 `IInventory` 已通过方法重写处理，无需注入回调。

#### NBT 序列化（结构模板 / 客户端同步）

`LootableContainerBlockEntity` 重写了 `loadFromNBT`/`saveToNBT`，处理 `LootTable`（string）与 `LootTableSeed`（long）两个键：

- **`loadFromNBT`**：调用 `BlockEntity::loadFromNBT` 后，重置战利品状态。若 NBT 中存在 `LootTable` 键则设置 `m_hasLootTable = true` 并重置 `m_lootFilled`，使后续容器访问触发延迟填充。`LootTableSeed` 始终读取（缺失默认 0，表示使用随机种子）。
- **`saveToNBT`**：调用 `BlockEntity::saveToNBT` 后，仅在 `m_hasLootTable && !m_lootFilled` 时写入两个键。已填充后不写入（避免持久化已生成物品与战利品表引用并存）。

子类（`ChestEntity`/`BarrelEntity` 等）通过继承自动获得战利品表 NBT 往返能力，无需各自重写。结构模板放置时，`Template::placeInWorld` 会在调用 `loadFromNBT` 前注入随机 `LootTableSeed`（见 `src/common/world/gen/feature/template/README.md` 第 13 节）。

**已知缺口 TODO**：当前子类未重写 `loadFromNBT`/`saveToNBT` 序列化容器物品列表（`Items` NBT 键），结构模板放置预填充物品的容器时物品会丢失。仅使用战利品表的容器不受影响。详见 `LootableContainerBlockEntity.cpp` 中的 TODO 注释。

### 4. 注册时序

方块实体注册应在游戏启动时完成（调用 `registerBuiltinTypes()`），之后才能从存档加载方块实体。

### 5. BlockEntityDeserializer 的 NBT 格式

NBT 标签必须包含 `"id"`, `"x"`, `"y"`, `"z"` 字段，否则反序列化失败。`id` 字段应为方块实体类型字符串（如 `"minecraft:chest"`）。

### 6. LockableBlockEntity 命名空间

`LockableBlockEntity`、`LootableContainerBlockEntity` 位于 `mc::blockentity` 命名空间，而非 `mc` 直接命名空间。使用时需要完整路径或 using 声明。
