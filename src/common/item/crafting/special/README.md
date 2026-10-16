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
├── DecoratedPotRecipe.hpp      # 饰纹陶罐配方（十字形：陶片/砖块→陶罐）
├── DecoratedPotRecipe.cpp
├── FireworkStarRecipe.hpp      # 烟花之星配方（火药+染料+可选形状/荧石粉/钻石）
├── FireworkStarRecipe.cpp
├── FireworkRocketRecipe.hpp    # 烟花火箭配方（纸+1~3 火药+0~N 烟花之星）
├── FireworkRocketRecipe.cpp
├── FireworkStarFadeRecipe.hpp  # 烟花之星渐变色配方（烟花之星+染料）
├── FireworkStarFadeRecipe.cpp
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
┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
│ShieldDecoration   │  │ TippedArrowRecipe │  │DecoratedPotRecipe │
│     Recipe        │  │                   │  │                   │
└───────────────────┘  └───────────────────┘  └───────────────────┘
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
| `item/items/DyeItemHelper.hpp` | 染料物品判定与颜色（ArmorDye/FireworkStar/FireworkStarFade 共用） |
| `item/component/FireworkExplosion.hpp`、`Firework.hpp` | 烟花爆炸/火箭数据组件载荷 |
| `item/tag/ItemTags.hpp` | 物品标签（DecoratedPotRecipe 使用 DECORATED_POT_INGREDIENTS） |
| `entity/inventory/CraftingInventory.hpp` | 合成容器接口 |
| `world/blockentity/interactive/DecoratedPotBlockEntity.hpp` | 饰纹陶罐图案与物品创建（DecoratedPotRecipe） |
| `util/color/DyeColor.hpp` | 染料颜色枚举（BannerDuplicateRecipe / 烟花色 `dyeFireworkColor`） |

**下游依赖（使用本模块的模块）**:

| 模块 | 用途 |
|------|------|
| `server/core/MinecraftServer` | 注册特殊配方 |
| `item/crafting/RecipeManager` | 配方注册表管理 |

## 容易踩的坑

1. **物品检测**：使用 `Items::XXX` 指针比较，而非字符串比较。染料判定统一走
   `item::DyeItemHelper`（16 种 *_dye + 墨囊 + 可可豆，与 vanilla 的 DyeItem 实例集合一致），
   不要在配方里各写一份染料名单。

2. **剩余物品处理**：不同配方的剩余物品行为不同：
   - `BookCloningRecipe`、`MapCloningRecipe`：原物品保留（通过 `getRemainingItems()` 返回）
   - `ArmorDyeRecipe`、`TippedArrowRecipe`：所有物品被消耗

3. **NBT 复制**：复制 NBT 数据时注意深拷贝，避免多个物品共享同一 NBT 对象。

4. **动态配方特性**：特殊配方的 `isDynamic()` 返回 true，不会出现在配方书中。

5. **配方匹配顺序**：`RecipeManager::findMatchingRecipe()` 会按注册顺序检查，特殊配方应在普通配方之后注册。

6. **代数限制**：`BookCloningRecipe` 最大代数为 2（第三代无法再复制），`MapExtendingRecipe` 最大缩放级别为 4。

7. **旗帜图案层数限制**：`BannerDuplicateRecipe` 要求源旗帜图案不超过 6 层。

8. **盾牌装饰前置条件**：`ShieldDecorationRecipe` 要求盾牌无现有图案（无 BlockEntityTag）。

9. **饰纹陶罐十字形匹配**：`DecoratedPotRecipe` 要求 3x3 网格，十字形四个位置（上、左、右、下）必须是 `DECORATED_POT_INGREDIENTS` 标签中的物品，其余五个位置必须为空。物品到图案的映射通过 `getPatternFromItem()` 完成。

10. **烟花三配方的匹配门槛是"非空物品数 >= 2"**：vanilla 用 `CraftingInput.ingredientCount()`，
    本项目按槽位逐个统计非空堆（一次消耗 1 个，不按堆叠数量加权）。三者还各有"至多一个"的
    唯一性门控（形状物品/荧石粉/钻石/火药/纸/烟花之星），漏掉任一即会把重复投放误判为可合成。

11. **烟花数据走数据组件而非 NBT**：星形带 `firework_explosion`，火箭带 `fireworks`
    （`flight_duration` = 火药数，`explosions` = 各星形的爆炸）。火箭的飞行等级是
    `1 + flight_duration`（vanilla `FireworkRocketEntity` 构造函数），别把组件里的
    `flight_duration` 直接当作飞行等级。

12. **渐变色配方须保留星形原有字段**：vanilla 是 `update(FIREWORK_EXPLOSION, DEFAULT, list, withFadeColors)`
    —— 星形无组件时以 `FireworkExplosion::defaultExplosion()` 为初值，**只替换 `fade_colors`**，
    形状/主色/拖尾/闪烁原样保留。且 `assemble` 必须返回 `stack.copy()` 的副本，不得就地改动网格内的源物品。
