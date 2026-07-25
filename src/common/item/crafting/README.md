# Crafting 模块

Cubium 的合成配方系统，实现了 MC 1.16.5 风格的配方管理、解析和匹配功能。

## 目录结构

```
src/common/item/crafting/
├── Ingredient.hpp/cpp            # 原料匹配器（支持物品/标签/合并，含延迟标签解析）
├── IRecipe.hpp                   # 配方接口和类型定义
├── RecipeType.cpp                # 配方类型字符串转换
├── ShapedRecipe.hpp              # 有序合成配方
├── ShapedRecipe.cpp
├── ShapelessRecipe.hpp           # 无序合成配方
├── ShapelessRecipe.cpp
├── SmeltingRecipe.hpp            # 熔炼配方（含高炉、烟熏炉、营火烹饪）
├── SmeltingRecipe.cpp
├── StonecuttingRecipe.hpp        # 切石机配方
├── StonecuttingRecipe.cpp
├── SmithingRecipe.hpp            # 锻造台配方
├── SmithingRecipe.cpp
├── TransmuteRecipe.hpp           # 物品转化配方（MC 1.21+，收纳袋染色等）
├── TransmuteRecipe.cpp
├── SpecialRecipe.hpp             # 特殊配方基类
├── RecipeManager.hpp             # 配方管理器（单例）
├── RecipeManager.cpp
├── RecipeSerializers.hpp         # 配方序列化器（JSON 解析）
├── RecipeSerializers.cpp
├── RecipeLoader.hpp              # 配方加载器（文件系统加载）
├── RecipeLoader.cpp
├── RecipeBook.hpp                # 配方书基类和服务端配方书
├── RecipeBook.cpp
└── special/                      # 特殊配方子模块
    ├── README.md
    ├── RepairItemRecipe.hpp      # 物品修复配方
    ├── RepairItemRecipe.cpp
    ├── ArmorDyeRecipe.hpp        # 盔甲染色配方
    ├── ArmorDyeRecipe.cpp
    ├── BookCloningRecipe.hpp     # 书复制配方
    ├── BookCloningRecipe.cpp
    ├── MapCloningRecipe.hpp      # 地图复制配方
    ├── MapCloningRecipe.cpp
    ├── MapExtendingRecipe.hpp    # 地图扩展配方
    ├── MapExtendingRecipe.cpp
    ├── BannerDuplicateRecipe.hpp # 旗帜复制配方
    ├── BannerDuplicateRecipe.cpp
    ├── ShieldDecorationRecipe.hpp # 盾牌装饰配方
    ├── ShieldDecorationRecipe.cpp
    ├── TippedArrowRecipe.hpp     # 药水箭配方
    └── TippedArrowRecipe.cpp
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │    IRecipe.hpp   │ 配方接口定义
                    │    RecipeType    │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┬──────────────┐
              │              │              │              │
              ▼              ▼              ▼              ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │ShapedRecipe  │ │ShapelessRecipe│ │SmeltingRecipe│ │SpecialRecipe │
    └──────────────┘ └──────────────┘ └──────────────┘ └──────┬───────┘
              │              │              │                 │
              └──────┬───────┴──────────────┘                 │
                     │                                        │
                     ▼                                        ▼
          ┌──────────────────┐                     ┌───────────────────┐
          │  RecipeManager   │ 配方注册表           │ special/* 特殊配方 │
          └────────┬─────────┘                     └───────────────────┘
                   │
          ┌────────┴────────┐
          │                 │
          ▼                 ▼
┌──────────────────┐ ┌──────────────────┐
│RecipeSerializers │ │RecipeBook        │ 配方书状态管理
│JSON 解析         │ │ServerRecipeBook  │ 服务端扩展
└────────┬─────────┘ └──────────────────┘
         │
         ▼
┌──────────────────┐
│  RecipeLoader    │ 文件系统加载
└──────────────────┘

┌──────────────────┐
│   Ingredient     │ 原料匹配器（被所有配方使用）
└──────────────────┘
```

> 注：原 `IRecipe::toNetwork/fromNetwork` 虚函数与 `RecipeNetworkSerializer.{hpp,cpp}` 已在 Phase4 网络重写中删除，配方不再走旧的 PacketSerializer 网络序列化路径。配方同步改由 IR `ir::play::*`（如 `RecipeBookAdd/Remove`）承担。

## 上下游外部依赖关系

**上游依赖（本模块使用的模块）**:

| 模块 | 用途 |
|------|------|
| `item/core/Item.hpp` | 物品定义 |
| `item/core/ItemStack.hpp` | 物品堆 |
| `item/core/ItemRegistry.hpp` | 物品注册表 |
| `item/tag/ItemTags.hpp` | 物品标签（Ingredient::fromTag） |
| `entity/inventory/CraftingInventory.hpp` | 合成容器 |
| `resource/ResourceLocation.hpp` | 资源位置 ID |
| `core/Result.hpp` | 错误处理 |
| `core/Types.hpp` | 基础类型 |
| `nlohmann/json` | JSON 解析 |

**下游依赖（使用本模块的模块）**:

| 模块 | 用途 |
|------|------|
| `server/core/MinecraftServer` | 注册特殊配方、配方加载 |
| `server/player/ServerPlayer` | 配方解锁、配方书同步 |
| `server/command/RecipeCommand` | /recipe 命令实现 |
| `entity/player/Player` | 合成逻辑调用 |
| `block/entity/CraftingTableBlockEntity` | 工作台配方匹配 |
| `block/entity/FurnaceBlockEntity` | 熔炉配方匹配 |

## 容易踩的坑

1. **有序配方的镜像匹配**
   - 有序配方默认支持水平镜像匹配
   - 如果配方不应该镜像（如特殊图案），需要在配方定义中禁用

2. **空 Ingredient 的行为**
   - 空 Ingredient 只匹配空物品堆（`stack.isEmpty() == true`），不匹配任何非空物品
   - 如果配方某个位置需要留空，不要添加 Ingredient 而是减少配方尺寸

3. **配方 ID 冲突**
   - 相同 ID 的配方注册会失败（返回 `false`）
   - 建议在加载前调用 `clear()` 或设置 `setClearBeforeLoad(true)`

4. **物品标签依赖**
   - `Ingredient::fromTag()` 需要 `ItemTags` 系统支持，会尝试立即解析标签
   - 如果标签在创建时未注册，`isSimple()` 返回 `false`，标签会在首次 `test()` 时延迟解析
   - 延迟解析后 `isSimple` 会自动更新，因此 `m_isSimple` 为 `mutable`

5. **ItemRegistry 依赖**
   - JSON 解析时如果物品未在 ItemRegistry 中注册，会返回空 Ingredient 或错误
   - 确保在加载配方前完成物品注册

6. **线程安全**
   - `RecipeManager` 的所有公共方法都是线程安全的
   - 但 `RecipeLoader` 不是线程安全的，不要在多线程中共享实例

7. **配方匹配顺序**
   - `findMatchingRecipe()` 先检查有序配方，再检查无序配方
   - 如果有多个配方匹配，返回第一个找到的

8. **特殊配方的动态特性**
   - 特殊配方的 `isDynamic()` 返回 true，不会出现在配方书中
   - `assemble()` 返回的结果可能每次不同（如染色、修复）
   - `ServerRecipeBook::add()` 自动过滤动态配方，不会将其加入解锁列表
   - `RecipeBook::isBookRecipe()` 对动态配方返回 false（即使被手动 unlock）
   - `ServerRecipeBook::isDynamicRecipe()` 静态方法用于判断配方是否为动态配方

9. **熔炼配方继承关系**
   - `BlastingRecipe`（高炉）继承 `SmeltingRecipe`，烹饪时间 100 tick
   - `SmokingRecipe`（烟熏炉）继承 `SmeltingRecipe`，烹饪时间 100 tick
   - `CampfireCookingRecipe`（营火）继承 `SmeltingRecipe`，烹饪时间 600 tick

10. **配方书状态同步**
    - `ServerRecipeBook::consumeNewRecipes()` 会清空新配方列表
    - 客户端同步后需要正确处理状态更新

11. **NBT 数据解析**
    - RecipeSerializers 支持 Mojangson 字符串和 JSON 对象两种 NBT 格式
    - 结果物品的 NBT 数据通过 `ItemStack::mergeTag()` 合并

12. **锻造台配方的 NBT 复制**
    - `SmithingRecipe` 会复制基础装备的 NBT 数据到结果物品
    - 附魔、耐久度等属性会被保留

13. **Ingredient::isSimple 语义**
    - `isSimple()` 返回 `true` 表示原料不包含可损坏物品，用于配方书优化
    - 包含可损坏物品（如工具）的 Ingredient 返回 `false`
    - 未解析的标签 Ingredient 返回 `false`（保守策略，无法确定标签内容）
    - 解析后为空的标签也返回 `false`

14. **Ingredient::getAllMatchingItems vs getMatchingStacks**
    - `getMatchingStacks()` 仅返回显式指定的物品堆，不包含标签解析后的物品
    - `getAllMatchingItems()` 返回包含标签解析物品的完整列表（延迟解析）
    - 需要完整物品列表时（如 `merge()`）应使用 `getAllMatchingItems()`

15. **MC 1.21+ 配方 JSON 格式兼容**
    - **字符串 ingredient**：MC 1.21+ 中 `ingredient` 字段支持直接使用字符串（如 `"minecraft:raw_iron"`），而非必须为对象（如 `{"item": "minecraft:raw_iron"}`）。`RecipeSerializers::parseIngredient()` 现在同时支持字符串、对象和数组三种格式。
    - **result 中的 id 字段**：MC 1.21+ 中 `result` 使用 `"id"` 替代 `"item"`。`RecipeSerializers::parseResult()` 同时支持两种字段名，当两者同时存在时 `"item"` 优先（向后兼容）。
    - **key 中的字符串 ingredient**：MC 1.21+ 中 `crafting_shaped` 配方的 `key` 值也支持字符串格式。
    - **ingredients 中的字符串元素**：MC 1.21+ 中 `crafting_shapeless` 配方的 `ingredients` 数组元素支持字符串格式。

16. **MC 1.21+ 数据包目录命名**
    - MC 1.21+ 数据包使用单数目录名 `recipe/`，旧版使用复数 `recipes/`。
    - `RecipeLoader` 的路径过滤和 `pathToRecipeId()` 同时支持两种目录名，确保兼容性。
    - 测试用例覆盖：`RecipeFormatTest` 测试了所有 MC 1.21+ 格式的解析正确性。

17. **MC 1.21+ 物品转化配方（TransmuteRecipe）**
    - 转化配方接受两个输入（input + material），输出指定物品，并保留 input 的 NBT 数据。
    - 典型用例：收纳袋染色（bundle + dye → colored_bundle，保留 BundleContents）。
    - JSON 格式：`"type": "minecraft:crafting_transmute"`，包含 `input`、`material`、`result` 字段。
    - `input` 和 `material` 字段支持字符串形式（含 `#` 前缀的标签引用，如 `"#minecraft:bundles"`）。
    - 匹配规则：网格中恰好 2 个非空物品，一个匹配 input，一个匹配 material，且转化结果不能与原物品相同。
    - NBT 保留：`ItemStack::transmuteCopy()` 替换物品类型，保留 customName、lore、customData（含 BundleContents）、enchantments 等。
    - 转化配方为特殊配方（`isSpecial = true`，不出现在配方书）和动态配方（`isDynamic = true`，结果依赖 input NBT）。
