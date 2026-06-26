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

### 1. 忘记设置 SimpleInventory 变更回调

修改 `SimpleInventory` 的数据后不会自动通知方块实体保存，必须设置回调：
```cpp
// 正确：设置变更回调
m_inventory(27, [this]() { setChanged(); })

// 错误：忘记回调，数据不会保存
m_inventory(27)
```

### 2. 锁定钥匙匹配逻辑

钥匙匹配使用物品的**显示名**而非物品ID。`canOpen()` 检查的是 `heldItem.getCustomName() == m_lockKey`。

### 3. 战利品表填充时机

`LootableContainerBlockEntity::fillWithLoot()` 已在基类实现，子类无需重写。填充通过 `IWorld::lootTableManager()` 获取战利品表管理器，只有 ServerWorld 返回有效指针。`isEmpty()` 和 `openContainer()` 会自动触发填充。

### 4. 注册时序

方块实体注册应在游戏启动时完成（调用 `registerBuiltinTypes()`），之后才能从存档加载方块实体。

### 5. BlockEntityDeserializer 的 NBT 格式

NBT 标签必须包含 `"id"`, `"x"`, `"y"`, `"z"` 字段，否则反序列化失败。`id` 字段应为方块实体类型字符串（如 `"minecraft:chest"`）。

### 6. LockableBlockEntity 命名空间

`LockableBlockEntity`、`LootableContainerBlockEntity` 位于 `mc::blockentity` 命名空间，而非 `mc` 直接命名空间。使用时需要完整路径或 using 声明。
