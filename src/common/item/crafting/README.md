# Crafting 模块

Minecraft Reborn 的合成配方系统，实现了 MC 1.16.5 风格的配方管理、解析和匹配功能。

## 目录结构

```
src/common/item/crafting/
├── Ingredient.hpp          # 原料匹配器接口
├── Ingredient.cpp          # 原料匹配器实现
├── IRecipe.hpp             # 配方接口和类型定义
├── RecipeType.cpp          # 配方类型字符串转换
├── ShapedRecipe.hpp        # 有序合成配方
├── ShapedRecipe.cpp        # 有序合成实现
├── ShapelessRecipe.hpp     # 无序合成配方
├── ShapelessRecipe.cpp     # 无序合成实现
├── RecipeManager.hpp       # 配方管理器
├── RecipeManager.cpp       # 配方管理器实现
├── RecipeSerializers.hpp   # 配方序列化器
├── RecipeSerializers.cpp   # JSON 解析实现
├── RecipeLoader.hpp        # 配方加载器
└── RecipeLoader.cpp        # 文件系统加载实现
```

## 文件详细介绍

### Ingredient.hpp / Ingredient.cpp

**职责**: 原料匹配器，用于配方中检查物品是否匹配。

**主要内容**:
- 支持三种匹配方式：
  - `fromItem()` - 单个物品匹配
  - `fromItems()` - 多个物品匹配（任一即可）
  - `fromTag()` - 物品标签匹配（预留接口，待实现）
  - `fromStacks()` - 从物品堆列表创建
- `test()` 方法检查物品是否匹配
- 支持空 Ingredient（不匹配任何物品）
- 实现了 `std::hash` 特化，可用于 `unordered_map`/`unordered_set`

**核心接口**:
```cpp
class Ingredient {
public:
    static Ingredient fromItem(const Item& item);
    static Ingredient fromItems(std::vector<const Item*> items);
    static Ingredient fromTag(const std::string& tag);
    static Ingredient fromStacks(std::vector<ItemStack> stacks);
    
    [[nodiscard]] bool test(const ItemStack& stack) const;
    [[nodiscard]] bool test(const Item& item) const;
    [[nodiscard]] const std::vector<ItemStack>& getMatchingStacks() const;
    [[nodiscard]] bool isEmpty() const;
};
```

### IRecipe.hpp

**职责**: 配方接口模板，定义所有配方的通用行为。

**主要内容**:
- `RecipeType` 枚举：定义 10 种配方类型
  - `Crafting` - 通用合成
  - `ShapedCrafting` - 有序合成
  - `ShapelessCrafting` - 无序合成
  - `Smelting` - 熔炼
  - `Blasting` - 高炉
  - `Smoking` - 烟熏炉
  - `CampfireCooking` - 营火烹饪
  - `Stonecutting` - 切石机
  - `Smithing` - 锻造台
  - `Special` - 特殊配方
- `IRecipe<C>` 模板接口：定义配方的核心方法
  - `matches()` - 检查是否匹配容器
  - `assemble()` - 生成结果物品
  - `getResultItem()` - 获取结果物品
  - `getIngredients()` - 获取原料列表
  - `getId()` - 获取配方 ID
  - `getType()` - 获取配方类型
- `IRecipeSerializer<C>` 接口：定义序列化/反序列化方法

### RecipeType.cpp

**职责**: 配方类型与字符串的双向转换。

**主要内容**:
- `recipeTypeToString()` - 配方类型转字符串（如 `minecraft:crafting_shaped`）
- `recipeTypeFromString()` - 字符串转配方类型
- 支持带命名空间和不带命名空间的字符串解析

### ShapedRecipe.hpp / ShapedRecipe.cpp

**职责**: 有序合成配方实现，要求原料按特定图案放置。

**主要内容**:
- 继承 `CraftingRecipe`（即 `IRecipe<CraftingInventory>`）
- 支持配方尺寸：1x1 到 3x3
- 匹配算法：
  1. 计算输入网格的边界框
  2. 检查边界框尺寸是否匹配配方尺寸
  3. 尝试正向匹配和水平镜像匹配
- 核心方法：
  - `matches()` - 检查是否匹配
  - `checkMatch()` - 检查指定偏移和镜像的匹配
  - `getContentBounds()` - 计算网格内容边界

**JSON 格式示例**:
```json
{
  "type": "minecraft:crafting_shaped",
  "pattern": ["##", "##"],
  "key": {
    "#": { "item": "minecraft:oak_planks" }
  },
  "result": {
    "item": "minecraft:crafting_table",
    "count": 1
  }
}
```

### ShapelessRecipe.hpp / ShapelessRecipe.cpp

**职责**: 无序合成配方实现，只要求原料存在，不要求位置。

**主要内容**:
- 继承 `CraftingRecipe`
- 匹配算法：
  1. 检查原料数量是否等于非空槽位数量
  2. 使用贪心匹配：对每个原料在网格中查找匹配物品
  3. 使用过的物品不能再次使用
- 支持任意位置放置，原料顺序不影响匹配

**JSON 格式示例**:
```json
{
  "type": "minecraft:crafting_shapeless",
  "ingredients": [
    { "item": "minecraft:iron_ingot" },
    { "item": "minecraft:stick" }
  ],
  "result": {
    "item": "minecraft:iron_nugget",
    "count": 9
  }
}
```

### RecipeManager.hpp / RecipeManager.cpp

**职责**: 配方中央注册表，存储和管理所有注册的配方。

**主要内容**:
- 单例模式
- 线程安全（使用 `std::mutex`）
- 多重索引：
  - `m_recipesById` - 按 ID 索引
  - `m_recipesByType` - 按类型索引
  - `m_recipesByResult` - 按结果物品索引
- 核心方法：
  - `registerRecipe()` - 注册配方
  - `getRecipe()` - 按 ID 获取
  - `getRecipesByType()` - 按类型获取
  - `findMatchingRecipe()` - 查找匹配配方
  - `getRecipesForResult()` - 按结果物品获取

**使用示例**:
```cpp
RecipeManager& manager = RecipeManager::instance();

// 注册配方
manager.registerRecipe(std::make_unique<ShapedRecipe>(...));

// 查找匹配的配方
CraftingInventory inv(3, 3);
const CraftingRecipe* recipe = manager.findMatchingRecipe(inv);
if (recipe) {
    ItemStack result = recipe->assemble(inv);
}
```

### RecipeSerializers.hpp / RecipeSerializers.cpp

**职责**: 配方序列化器，从 JSON 解析配方数据。

**主要内容**:
- 支持 MC 1.16.5 数据包格式
- 解析方法：
  - `fromJson()` - 从 JSON 解析配方（自动识别类型）
  - `parseShapedRecipe()` - 解析有序合成
  - `parseShapelessRecipe()` - 解析无序合成
  - `parseIngredient()` - 解析原料
  - `parseResult()` - 解析结果物品（支持 NBT 数据）
- 支持的原料格式：
  - 单物品: `{ "item": "minecraft:stone" }`
  - 标签: `{ "tag": "minecraft:planks" }`
  - 多选项: `[{ "item": "a" }, { "item": "b" }]`
- 支持的结果格式：
  - 字符串形式: `"minecraft:stone"`
  - 对象形式: `{ "item": "minecraft:stone", "count": 1 }`
  - 带 NBT 数据（Mojangson 字符串）: `{ "item": "minecraft:iron_sword", "nbt": "{display:{Name:\"Custom Sword\"}}" }`
  - 带 NBT 数据（JSON 对象）: `{ "item": "minecraft:iron_sword", "nbt": {"display":{"Name":"Custom Sword"}} }`

**NBT 数据解析**（2026-05-17 新增）:
- 支持 Mojangson 字符串格式解析（使用 `nbt::contexts::mojangson`）
- 支持 JSON 对象格式直接合并到 ItemStack
- 参考 MC 1.16.5 `CraftingHelper.getItemStack()` 实现
- NBT 数据通过 `ItemStack::mergeTag()` 合并到物品堆

### RecipeLoader.hpp / RecipeLoader.cpp

**职责**: 从文件系统加载配方 JSON 文件。

**主要内容**:
- 支持从目录递归加载
- 路径到配方 ID 的自动转换
  - `data/minecraft/recipes/crafting_table.json` → `minecraft:crafting_table`
- 内置原版配方加载（预留接口）
- 进度回调支持
- 加载结果统计

**使用示例**:
```cpp
RecipeLoader loader;
auto result = loader.loadFromDirectory("data/minecraft/recipes");
if (result.success()) {
    std::cout << "Loaded " << result.value().successCount << " recipes\n";
}
```

## 文件关系图

```
                    ┌─────────────────┐
                    │    IRecipe.hpp   │ (接口定义)
                    │    RecipeType    │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │ShapedRecipe  │ │ShapelessRecipe│ │  (其他配方)   │
    └──────────────┘ └──────────────┘ └──────────────┘
              │              │
              └──────┬───────┘
                     │
                     ▼
          ┌──────────────────┐
          │  RecipeManager   │ (配方注册表)
          └────────┬─────────┘
                   │
                   ▼
          ┌──────────────────┐
          │RecipeSerializers │ (JSON 解析)
          └────────┬─────────┘
                   │
                   ▼
          ┌──────────────────┐
          │  RecipeLoader    │ (文件加载)
          └──────────────────┘

    ┌──────────────────┐
    │   Ingredient     │ (原料匹配，被所有配方使用)
    └──────────────────┘
```

## 模块概述

### 整体职责

Crafting 模块负责管理 Minecraft 中的合成配方系统：

1. **配方定义**: 定义有序合成和无序合成配方
2. **配方匹配**: 检查玩家合成网格是否匹配配方
3. **配方注册**: 集中管理所有配方
4. **配方加载**: 从数据包 JSON 文件加载配方

### 输入和输出

**输入**:
- `CraftingInventory` - 玩家合成网格状态
- JSON 配方文件（MC 1.16.5 数据包格式）

**输出**:
- `ItemStack` - 合成结果物品
- 配方查询结果

### 依赖项

| 依赖模块 | 用途 |
|---------|------|
| `item/Item.hpp` | 物品定义 |
| `item/ItemStack.hpp` | 物品堆 |
| `item/ItemRegistry.hpp` | 物品注册表 |
| `entity/inventory/CraftingInventory.hpp` | 合成容器 |
| `resource/ResourceLocation.hpp` | 资源位置 ID |
| `core/Result.hpp` | 错误处理 |
| `nlohmann/json` | JSON 解析 |

### 使用方法

#### 1. 从 JSON 加载配方

```cpp
#include "item/crafting/RecipeLoader.hpp"

// 加载配方目录
RecipeLoader loader;
auto result = loader.loadFromDirectory("data/minecraft/recipes");
if (result.success()) {
    std::cout << "成功加载 " << result.value().successCount << " 个配方\n";
    for (const auto& error : result.value().errors) {
        std::cerr << "错误: " << error << "\n";
    }
}
```

#### 2. 查找匹配配方

```cpp
#include "item/crafting/RecipeManager.hpp"
#include "entity/inventory/CraftingInventory.hpp"

// 创建合成网格
CraftingInventory inventory(3, 3);  // 3x3 工作台
inventory.setItemAt(0, 0, ItemStack(oakPlanks, 1));
inventory.setItemAt(0, 1, ItemStack(oakPlanks, 1));
inventory.setItemAt(1, 0, ItemStack(oakPlanks, 1));
inventory.setItemAt(1, 1, ItemStack(oakPlanks, 1));

// 查找匹配配方
const CraftingRecipe* recipe = RecipeManager::instance().findMatchingRecipe(inventory);
if (recipe) {
    ItemStack result = recipe->assemble(inventory);
    // result = crafting_table
}
```

#### 3. 手动创建配方

```cpp
#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/ShapelessRecipe.hpp"

// 有序合成：木板 -> 工作台
std::vector<Ingredient> ingredients(4, Ingredient::fromItem(oakPlanks));
auto shapedRecipe = std::make_unique<ShapedRecipe>(
    ResourceLocation("minecraft", "crafting_table"),
    2, 2,  // 2x2 配方
    std::move(ingredients),
    ItemStack(craftingTable, 1)
);

// 无序合成：铁锭 + 木棍 -> 铁剑
auto shapelessRecipe = std::make_unique<ShapelessRecipe>(
    ResourceLocation("minecraft", "iron_sword"),
    std::vector<Ingredient>{
        Ingredient::fromItem(ironIngot),
        Ingredient::fromItem(stick)
    },
    ItemStack(ironSword, 1)
);

// 注册配方
RecipeManager::instance().registerRecipe(std::move(shapedRecipe));
RecipeManager::instance().registerRecipe(std::move(shapelessRecipe));
```

### 容易踩的坑

1. **有序配方的镜像匹配**
   - 有序配方默认支持水平镜像匹配
   - 如果配方不应该镜像（如特殊图案），需要在配方定义中禁用

2. **空 Ingredient 的行为**
   - 空 Ingredient 不匹配任何物品，包括空物品堆
   - 如果配方某个位置需要留空，不要添加 Ingredient 而是减少配方尺寸

3. **配方 ID 冲突**
   - 相同 ID 的配方注册会失败（返回 `false`）
   - 建议在加载前调用 `clear()` 或设置 `setClearBeforeLoad(true)`

4. **物品标签未实现**
   - `Ingredient::fromTag()` 目前返回空 Ingredient
   - 等待物品标签系统（ItemTags）实现后才能使用

5. **ItemRegistry 依赖**
   - JSON 解析时如果物品未在 ItemRegistry 中注册，会返回空 Ingredient 或错误
   - 确保在加载配方前完成物品注册

6. **线程安全**
   - `RecipeManager` 的所有公共方法都是线程安全的
   - 但 `RecipeLoader` 不是线程安全的，不要在多线程中共享实例

7. **配方匹配顺序**
   - `findMatchingRecipe()` 先检查有序配方，再检查无序配方
   - 如果有多个配方匹配，返回第一个找到的

## 测试用例

测试文件位于 `tests/common/item/crafting/` 目录：

| 测试文件 | 测试内容 |
|---------|---------|
| `RecipeManagerTest.cpp` | 配方管理器注册、查询、匹配测试 |
| `ShapedRecipeTest.cpp` | 有序合成构造、匹配、边界测试 |
| `ShapelessRecipeTest.cpp` | 无序合成构造、匹配测试 |
| `RecipeLoaderTest.cpp` | JSON 解析、文件加载测试 |
| `RecipeSerializersTest.cpp` | 配方序列化器测试（parseResult NBT 解析、parseIngredient 测试） |

### 主要测试场景

**RecipeManager 测试**:
- 单例模式验证
- 配方注册和重复 ID 检测
- 按 ID、类型、结果查询
- 配方匹配查找

**ShapedRecipe 测试**:
- 1x1、2x2、3x3 配方构造
- `canFitIn()` 尺寸检查
- 正确位置匹配
- 错误物品检测
- 镜像匹配

**ShapelessRecipe 测试**:
- 空配方匹配
- 单原料/多原料匹配
- 原料数量验证
- 原料顺序无关性

**RecipeLoader 测试**:
- 不存在目录错误处理
- 空目录加载
- 有效 JSON 解析
- 无效 JSON 错误处理
- 缺少字段错误处理
- 未知类型错误处理

**RecipeSerializers 测试**:
- `parseResult()` 字符串形式解析
- `parseResult()` 对象形式解析（带 count）
- `parseResult()` NBT JSON 对象格式解析
- `parseResult()` NBT Mojangson 字符串格式解析
- `parseResult()` 无效 NBT 字符串处理
- `parseResult()` 多字段 NBT 合并
- `parseIngredient()` 单物品解析
- `parseIngredient()` 物品数组解析
- `parseIngredient()` 标签解析
- `parseIngredient()` 未知物品处理

## 未来扩展

当前系统已实现的配方类型：

1. **有序合成** (`ShapedRecipe`) - ✅ 完成
2. **无序合成** (`ShapelessRecipe`) - ✅ 完成
3. **熔炉配方** (`SmeltingRecipe`) - ✅ 完成
4. **高炉配方** (`BlastingRecipe`) - ✅ 完成（继承 SmeltingRecipe，100 tick）
5. **烟熏炉配方** (`SmokingRecipe`) - ✅ 完成（继承 SmeltingRecipe，100 tick）
6. **营火烹饪配方** (`CampfireCookingRecipe`) - ✅ 完成（继承 SmeltingRecipe，600 tick）
7. **切石机配方** (`StonecuttingRecipe`) - ✅ 完成
8. **锻造台配方** (`SmithingRecipe`) - ✅ 完成（支持 NBT 数据复制）
9. **特殊配方** (`SpecialRecipe`) - ✅ 完成
   - `RepairItemRecipe` - 物品修复配方 ✅
   - `ArmorDyeRecipe` - 盔甲染色配方 ✅（支持 16 种染料 + 墨囊 + 可可豆）
   - `BookCloningRecipe` - 书复制配方 ✅（支持代数限制）
   - `MapCloningRecipe` - 地图复制配方 ✅（待地图物品实现后完全可用）

已实现功能：

1. **剩余物品系统** - ✅ 完成
   - `IRecipe::getRemainingItems()` 接口
   - `RecipeUtils::getDefaultRemainingItems()` 工具函数
   - `ItemStack::getContainerItem()` 和 `hasContainerItem()` 方法

2. **网络同步** - ✅ 完成
   - `Ingredient::serialize()` 和 `Ingredient::deserialize()` 网络序列化
   - `RecipeNetworkSerializer` 类提供配方序列化/反序列化
   - 支持有序合成、无序合成、熔炼、切石机、锻造台配方

已实现功能（2026-05-15）：

1. **配方书系统** - ✅ 完成
   - `RecipeBook` 基类 - 存储已解锁配方和新配方列表
   - `RecipeBookStatus` 类 - GUI 状态管理（工作台、熔炉、高炉、烟熏炉）
   - `ServerRecipeBook` 类 - 服务端扩展，支持 NBT 序列化
   - `ServerPlayer::unlockRecipe()` - 解锁配方并触发成就
   - `ServerPlayer::unlockRecipes()`/`lockRecipes()` - 批量操作
   - `RecipeCommand` 完善 - `/recipe give/take` 命令实现
