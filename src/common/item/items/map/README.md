# 地图物品模块 (Map Items)

地图物品模块实现了 Minecraft 1.16.5 的地图系统物品层，包括空地图和已填充地图。

## 目录结构

```
map/
├── AbstractMapItem.hpp/cpp   # 地图物品抽象基类，标记为复杂物品
├── EmptyMapItem.hpp/cpp      # 空地图物品，右键创建已填充地图
└── FilledMapItem.hpp/cpp     # 已填充地图物品，包含地形数据和玩家追踪，重写 onCraftedPostProcess
```

## 内部模块关系

```
Item
  └─ AbstractMapItem         (isComplex = true，需要服务端主动推送更新)
       ├─ EmptyMapItem       (minecraft:map)
       └─ FilledMapItem      (minecraft:filled_map)
```

**调用关系**：
- `EmptyMapItem::onItemRightClick()` 调用 `FilledMapItem::setupNewMap()` 创建新地图
- `FilledMapItem` 通过 `MapDataManager` 访问和修改 `MapData`
- `FilledMapItem::inventoryTick()` 更新玩家位置标记和地形数据

## 上下游外部依赖关系

**上游依赖（本目录依赖）**：
- `mc::world::map::MapData` - 地图数据存储
- `mc::world::map::MapDataManager` - 地图数据管理器（CRUD、持久化）
- `mc::world::map::MapDecoration` / `DecorationType` - 装饰物定义
- `mc::world::map::MaterialColorId` - 地图颜色系统
- `mc::item::core::Item` / `ItemStack` / `ItemRegistry` - 物品基类和注册
- `mc::entity::Player` / `IWorld` - 玩家和世界接口

**下游依赖（依赖本目录）**：
- `MapCloningRecipe` - 地图复制配方
- `MapExtendingRecipe` - 地图扩展配方
- `CartographyMenu` - 制图台菜单
- `ExplorationMapFunction` - 战利品表中的探险地图生成

## 容易踩的坑

### 1. 地图物品不可堆叠

`FilledMapItem` 的 `getMaxStackSize()` 返回 1，每张地图都是独特的。创建地图物品堆时需注意。

### 2. 地图ID存储在物品NBT中

`FilledMapItem` 通过 `stack.getTag()["map"]` 存储地图ID，关联到 `MapDataManager` 中的 `MapData`。访问地图数据前必须检查NBT和MapDataManager是否有效。

### 3. inventoryTick 仅在服务端更新地形

`FilledMapItem::inventoryTick()` 在客户端直接返回，地形更新和玩家追踪仅在服务端执行。客户端地图同步走 IR `ir::play::MapItemData`（当前 Phase6 TODO，opaque no-op）。

### 4. 锁定地图创建新副本

`lockMap()` 不是简单设置标志，而是创建一个新的锁定副本（新地图ID）。锁定操作会改变物品的地图ID。

### 5. onCraftedPostProcess 处理合成后操作

`FilledMapItem::onCraftedPostProcess()` 重写了 `Item` 基类的虚方法，在物品被合成时处理以下 NBT 标签：
- `map_scale_direction`：调用 `scaleMap()` 执行地图缩放，处理后移除标签
- `map_lock`：调用 `lockMap()` 执行地图锁定，处理后移除标签

这些标签由 `MapExtendingRecipe`（地图扩展配方）和 `CartographyContainer`（制图台容器）在 `assemble()`/`updateResult()` 中设置，在玩家取出合成结果时通过 `ServerPlayer::onItemCrafted()` → `ItemStack::onCraftedBy()` → `Item::onCraftedBy()` → `Item::onCraftedPostProcess()` 调用链触发处理。
