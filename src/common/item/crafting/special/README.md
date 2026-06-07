# 特殊配方模块 (Special Recipes)

本模块实现了 Minecraft 1.16.5 的特殊合成配方，这些配方没有固定的合成图案，而是基于特定的物品组合逻辑。

## 目录结构

```
src/common/item/crafting/special/
├── SpecialRecipe.hpp           # 特殊配方基类（无固定合成图案）
├── SpecialRecipe.cpp
├── RepairItemRecipe.hpp        # 物品修复配方（两相同物品合并）
├── RepairItemRecipe.cpp
├── ArmorDyeRecipe.hpp          # 盔甲染色配方（皮革盔甲+染料）
├── ArmorDyeRecipe.cpp
├── BookCloningRecipe.hpp       # 书复制配方（成书+书与笔）
├── BookCloningRecipe.cpp
├── MapCloningRecipe.hpp        # 地图复制配方（已填充地图+空地图）
├── MapCloningRecipe.cpp
├── MapExtendingRecipe.hpp      # 地图扩展配方（地图+纸）
├── MapExtendingRecipe.cpp
├── BannerDuplicateRecipe.hpp   # 旗帜复制配方（复制旗帜图案）
├── BannerDuplicateRecipe.cpp
├── ShieldDecorationRecipe.hpp  # 盾牌装饰配方（盾牌+旗帜）
├── ShieldDecorationRecipe.cpp
├── TippedArrowRecipe.hpp       # 药水箭配方（滞留药水+箭）
├── TippedArrowRecipe.cpp
└── README.md
```

## 内部模块关系

```
                    ┌─────────────────────┐
                    │   SpecialRecipe     │ 基类（继承 IRecipe）
                    │   (SpecialRecipe.*) │
                    └──────────┬──────────┘
                               │ 继承
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
│ RepairItemRecipe  │  │  ArmorDyeRecipe   │  │ BookCloningRecipe │
└───────────────────┘  └───────────────────┘  └───────────────────┘
        │                      │                      │
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
│ MapCloningRecipe  │  │ MapExtendingRecipe│  │BannerDuplicateRecipe│
└───────────────────┘  └───────────────────┘  └───────────────────┘
        │                      │                      │
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────────┐  ┌───────────────────┐
│ShieldDecoration   │  │ TippedArrowRecipe │
│     Recipe        │  │                   │
└───────────────────┘  └───────────────────┘
```

所有特殊配方类都继承自 `SpecialRecipe` 基类，基类提供：
- `isDynamic()` 返回 true（不显示在配方书中）
- 统一的 `IRecipe<CraftingInventory>` 接口

## 上下游外部依赖关系

**上游依赖（本模块使用的模块）**:

| 模块 | 用途 |
|------|------|
| `item/crafting/SpecialRecipe.hpp` | 基类定义 |
| `item/core/Item.hpp` | 物品类型判断 |
| `item/core/ItemStack.hpp` | 物品堆操作、NBT 数据 |
| `item/Items.hpp` | 物品静态引用 |
| `entity/inventory/CraftingInventory.hpp` | 合成容器接口 |
| `util/color/DyeColor.hpp` | 染料颜色枚举（BannerDuplicateRecipe） |

**下游依赖（使用本模块的模块）**:

| 模块 | 用途 |
|------|------|
| `server/core/MinecraftServer` | 注册特殊配方 |
| `item/crafting/RecipeManager` | 配方注册表管理 |

## 容易踩的坑

1. **物品检测**：使用 `Items::XXX` 指针比较，而非字符串比较。染料需要检查所有 16 种 + 墨囊 + 可可豆。

2. **剩余物品处理**：不同配方的剩余物品行为不同：
   - `BookCloningRecipe`、`MapCloningRecipe`：原物品保留（通过 `getRemainingItems()` 返回）
   - `ArmorDyeRecipe`、`TippedArrowRecipe`：所有物品被消耗

3. **NBT 复制**：复制 NBT 数据时注意深拷贝，避免多个物品共享同一 NBT 对象。

4. **动态配方特性**：特殊配方的 `isDynamic()` 返回 true，不会出现在配方书中。

5. **配方匹配顺序**：`RecipeManager::findMatchingRecipe()` 会按注册顺序检查，特殊配方应在普通配方之后注册。

6. **代数限制**：`BookCloningRecipe` 最大代数为 2（第三代无法再复制），`MapExtendingRecipe` 最大缩放级别为 4。

7. **旗帜图案层数限制**：`BannerDuplicateRecipe` 要求源旗帜图案不超过 6 层。

8. **盾牌装饰前置条件**：`ShieldDecorationRecipe` 要求盾牌无现有图案（无 BlockEntityTag）。
