# 工具层级模块 (Item Tier Module)

## 目录结构

```
src/common/item/tier/
├── IItemTier.hpp      # 工具层级接口定义
├── IItemTier.cpp      # 空实现文件（接口无实现）
├── ItemTiers.hpp      # 原版层级定义（静态工厂类，含7个层级）
└── ItemTiers.cpp      # 原版层级实现（含内部 ItemTierImpl 类）
```

## 内部模块关系

```
┌─────────────────┐
│  IItemTier.hpp  │  ←── 接口定义
└────────┬────────┘
         │ 继承
         ▼
┌─────────────────┐
│ ItemTierImpl    │  ←── 内部实现类（在 ItemTiers.cpp 中定义）
│ (ItemTiers.cpp) │
└────────┬────────┘
         │ 由其创建
         ▼
┌─────────────────┐
│  ItemTiers.hpp  │  ←── 静态工厂类，持有七个 IItemTier 实例
└─────────────────┘
```

## 上下游外部依赖关系

**上游依赖（本模块依赖）**：
- `common/core/Types.hpp` - 基础类型定义（i32, f32 等）
- `common/item/crafting/Ingredient.hpp` - 修复材料配方成分
- `common/item/Items.hpp` - 物品引用（修复材料）

**下游依赖（依赖本模块）**：
- `src/common/item/tool/TieredItem.hpp` - 层级物品基类
- `src/common/item/tool/ToolItem.hpp` - 工具基类
- `src/common/item/items/tool/PickaxeItem.hpp` - 镐
- `src/common/item/items/tool/AxeItem.hpp` - 斧
- `src/common/item/items/tool/ShovelItem.hpp` - 锹
- `src/common/item/items/tool/HoeItem.hpp` - 锄
- `src/common/item/items/tool/SwordItem.hpp` - 剑

## 容易踩的坑

### 1. 初始化顺序错误

如果在 `Items::initialize()` 之前调用 `ItemTiers::initialize()`，会导致崩溃或未定义行为，因为修复材料引用的物品指针尚未初始化。

**正确顺序**：
```cpp
Items::initialize();      // 先初始化物品注册表
ItemTiers::initialize();  // 再初始化层级
```

**注意**: `Items::initialize()` 内部已调用 `ItemTiers::initialize()`，通常无需手动调用。

### 2. 金工具的特殊性

金工具的挖掘等级是 0（与木相同），效率最高（12.0），但只能采集与木工具相同的方块类型。

### 3. 修复材料引用生命周期

`getRepairMaterial()` 返回 `const Ingredient&` 引用，生命周期由静态变量管理。不要存储修复材料的指针或引用到长期存在的对象中。

### 4. 层级引用不能为空

`ItemTiers::XXX()` 返回引用而非指针，不存在空层级，所有工具必须有有效的层级。
