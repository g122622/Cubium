# Item Tag 模块

物品标签系统，用于物品分组，支持配方匹配和功能判断。

## 目录结构

```
tag/
├── ItemTag.hpp           # 物品标签类（物品集合）
├── ItemTag.cpp           # 物品标签实现
├── ItemTags.hpp          # 物品标签注册表（FLOWERS、CARPETS 等）
├── ItemTags.cpp          # 内置标签初始化
└── README.md             # 本文件
```

## 内部模块关系

```
┌─────────────────┐
│    ItemTags     │  注册表（静态方法）
│  FLOWERS()      │
│  CARPETS()      │
│  registerTag()  │
└────────┬────────┘
         │ 管理
         ▼
┌─────────────────┐
│     ItemTag     │  单个标签（物品集合）
│  add()          │
│  contains()     │
│  getItems()     │
└─────────────────┘
```

## 上下游外部依赖关系

### 上游依赖

| 模块 | 依赖内容 |
|------|---------|
| `item/core/Item.hpp` | `Item` 类定义 |
| `item/core/ItemStack.hpp` | `ItemStack` 类定义 |
| `item/core/ItemRegistry.hpp` | 物品注册表（初始化时获取物品指针） |
| `core/ResourceLocation.hpp` | 资源位置标识符 |

### 下游依赖

| 模块 | 依赖内容 |
|------|---------|
| 实体繁殖逻辑 | `FLOWERS` 标签判断蜜蜂繁殖物品 |
| 羊驼装饰槽位 | `CARPETS` 标签判断有效装饰 |
| 配方系统（未来） | `Ingredient` 将支持标签匹配 |

## 容易踩的坑

### 1. 初始化顺序依赖

`ItemTags::initialize()` **必须**在以下初始化之后调用：

1. `VanillaBlocks::initialize()` - 方块注册
2. `Items::initialize()` - 物品注册
3. `BlockItemRegistry::instance().initializeVanillaBlockItems()` - 方块物品注册

初始化入口：
- 客户端：`ClientApplication::initializeCoreRegistries()`
- 服务器：`MinecraftServer::initializeRegistries()`

### 2. 标签判断使用方式

`Item::isIn(tag)` 和 `ItemTag::contains(item)` 功能相同，推荐使用前者更直观。检查 `ItemStack` 时需要先判空：

```cpp
if (!stack.isEmpty() && stack.getItem()->isIn(tag)) { ... }
```

### 3. 扩展标签

新增内置标签时，需要在 `ItemTags.cpp` 的 `initialize()` 中注册，并通过静态方法暴露访问入口。
