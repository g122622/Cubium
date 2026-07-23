# 收纳袋子模块 (Bundle Submodule)

收纳袋（Bundle）是 MC 1.21.11 引入的物品，允许玩家在单个物品槽位中存储多个物品堆。
本子模块实现收纳袋的核心数据结构与物品逻辑。

## 目录结构

```text
bundle/
├── README.md                    # 本文档
├── BundleContents.hpp/cpp       # 收纳袋内容物数据结构
└── BundleItem.hpp/cpp           # 收纳袋物品类（17 个变体：1 无色 + 16 色）
```

## 文件说明

### BundleContents

`BundleContents` 是收纳袋的内容物容器，对应 MC 1.21.11 的 `DataComponents.BUNDLE_CONTENTS`。

本项目中由于没有 DataComponents 架构，收纳袋内容物存储在 `ItemStack` 的 `m_customData` 中，
路径为 `BundleContents`。`BundleContents` 类提供对该数据的类型安全封装。

**核心概念：**

- **重量系统**：收纳袋有最大重量限制 `MAX_WEIGHT = 64`。
  - 普通物品：`weight = ceil(64 / maxStackSize)`（如可堆叠 64 个的物品重量为 1，可堆叠 16 个的物品重量为 4）
  - 收纳袋本身：固定重量 4 + 内部已用重量（防止无限嵌套）
  - 不可堆叠物品：固定重量 64（占满整个收纳袋）
- **选中物品**：收纳袋可以有一个"选中"的物品堆（`selectedItem`），用于右键取出操作。
- **序列化**：内容物以 JSON 数组形式存储，每个元素包含 `id`、`count`、可选的 `components`（NBT）。

**关键方法：**

| 方法 | 说明 |
|------|------|
| `empty()` | 创建空内容物 |
| `fromItemStack(stack)` | 从物品堆的 NBT 读取内容物 |
| `toItemStack(stack)` | 将内容物写回物品堆的 NBT |
| `weight()` | 计算内容物总重量 |
| `canAddItem(stack)` | 检查能否添加指定物品堆 |
| `add(stack)` | 添加物品堆到内容物 |
| `removeSelectedItem()` | 移除并返回选中物品堆 |
| `getItem(i)` | 获取指定索引的物品堆 |
| `size()` | 内容物中的物品堆数量 |
| `numberOfItemsToShow()` | 计算物品栏 UI 中应显示的物品数量（最多 12） |

### BundleItem

`BundleItem` 是收纳袋物品类，对应 MC 1.21.11 的 `net.minecraft.world.item.BundleItem`。

**变体：**

- 1 个无色收纳袋（`minecraft:bundle`）
- 16 个有色收纳袋（`minecraft:white_bundle`、`minecraft:orange_bundle`、...、`minecraft:black_bundle`）
- 颜色由 `DyeColor` 枚举指定，`DyeColor::Count` 表示无色

**交互协议（MC 1.20+ 插槽覆盖协议）：**

收纳袋使用"插槽覆盖协议"（Slot Override Protocol）实现与物品栏的特殊交互：

| 操作 | 方法 | 行为 |
|------|------|------|
| 玩家手持收纳袋左键物品栏槽位 | `overrideStackedOnOther` | 尝试将该槽位的物品放入收纳袋 |
| 玩家手持其他物品左键收纳袋槽位 | `overrideOtherStackedOnMe` | 尝试将该物品放入收纳袋 |
| 玩家右键收纳袋 | `onItemRightClick` | 取出选中物品 |

**重量限制：**

- `MAX_WEIGHT = 64`（对应 MC 1.21.11 `BundleContents.MAX_WEIGHT`）
- `BUNDLE_IN_BUNDLE_WEIGHT = 4`（收纳袋放入收纳袋的固定重量加成）

**NBT 持久化：**

- 内容物存储在 `ItemStack::m_customData` 的 `BundleContents` 字段中
- 使用 `BundleContents::fromItemStack` / `BundleContents::toItemStack` 读写

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│ 收纳袋子模块 (Bundle Submodule)                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────────┐  持有/操作   ┌──────────────────┐    │
│  │ BundleItem       │◄────────────│ BundleContents   │    │
│  │ (17 个变体)      │             │ (内容物数据)     │    │
│  └────────┬─────────┘             └────────┬─────────┘    │
│           │                                │              │
│  DyeColor 颜色                   序列化到 ItemStack NBT    │
│  17 个 Items::XXX_BUNDLE         路径: "BundleContents"    │
│                                                             │
│  插槽覆盖协议：                                             │
│  - overrideStackedOnOther (手持收纳袋点击其他槽位)          │
│  - overrideOtherStackedOnMe (手持其他物品点击收纳袋槽位)    │
│  - onItemRightClick (右键取出选中物品)                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

**内部依赖（本模块依赖）：**

- `Item` 基类、`ItemStack`（物品系统核心）
- `DyeColor`（颜色枚举，16 色 + Count）
- `Slot`、`SlotClickAction`（插槽覆盖协议）
- `Player`（玩家交互）
- `IWorld`（世界访问）

**外部依赖（谁依赖本模块）：**

- `Items` 静态注册表（注册 17 个收纳袋变体）
- `ItemTags::BUNDLES`（物品标签，包含所有 17 个变体）
- `TransmuteRecipe`（转化配方，用于收纳袋染色：bundle + dye → colored_bundle）
- `AbstractContainerMenu`（物品栏交互，通过插槽覆盖协议）
- 客户端 tooltip 渲染：kagero 体系暂未实现收纳袋图像 tooltip。`ItemTooltipBuilder` 仅构建文本 tooltip（displayName/Count/Durability/appendHoverText），不渲染内容物网格与进度条。`BundleItem::isBundleItem(stack)`/`getNumberOfItemsToShow`/`getFullnessDisplay` 仍作为公共 API 保留，供 HUD 或第三方插件查询收纳袋状态。

## 收纳袋染色配方

收纳袋可以通过转化配方（TransmuteRecipe）染色：

- 输入：任意颜色收纳袋（`#minecraft:bundles` 标签）
- 材料：染料（如 `minecraft:white_dye`）
- 结果：对应颜色的收纳袋（如 `minecraft:white_bundle`）
- NBT 保留：`transmuteCopy` 保留收纳袋内容物（`BundleContents`）

配方 JSON 文件位于数据包：`data/minecraft/recipes/` 下，共 16 个染色配方。

## 容易踩的坑

1. **重量计算**：收纳袋本身的重量是 `4 + 内部重量`，不是固定 4。空收纳袋重量为 4，装满后重量可达 64。
2. **不可堆叠物品**：不可堆叠物品（如工具）重量为 64，会占满整个收纳袋。
3. **潜影盒限制**：潜影盒不能放入收纳袋（`canFitInsideContainerItems` 返回 false），防止递归存储。
4. **NBT 路径**：内容物存储在 `m_customData` 的 `BundleContents` 字段，不是 `display` 或其他字段。
5. **选中物品索引**：选中物品索引存储在 `BundleContents` 的 `selectedItem` 字段，移除后需要重置为空。
6. **转化配方 NBT 保留**：`transmuteCopy` 保留所有 NBT（包括 `BundleContents`），所以染色后内容物不丢失。
7. **插槽覆盖协议方向**：`overrideStackedOnOther` 是收纳袋在其他槽位上叠加，`overrideOtherStackedOnMe` 是其他物品在收纳袋槽位上叠加。

## 参考

- MC 1.21.11 `net.minecraft.world.item.BundleItem`
- MC 1.21.11 `net.minecraft.world.item.component.BundleContents`
- MC 1.21.11 `net.minecraft.world.item.crafting.TransmuteRecipe`
