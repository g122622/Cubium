# 地图物品 (Map Items)

地图物品模块实现了 Minecraft 1.16.5 的地图系统物品层。

## 文件说明

| 文件 | 描述 |
|------|------|
| `AbstractMapItem.hpp/cpp` | 地图物品基类，标记为复杂物品（需要网络同步） |
| `EmptyMapItem.hpp/cpp` | 空地图物品，右键使用创建已填充地图 |
| `FilledMapItem.hpp/cpp` | 已填充地图物品，包含地形数据和玩家追踪 |

## 地图物品层次

```
Item
  └─ AbstractMapItem         (isComplex = true)
       ├─ EmptyMapItem       (minecraft:map)
       └─ FilledMapItem      (minecraft:filled_map)
```

## 核心功能

### EmptyMapItem（空地图）
- 右键使用：消耗空地图，在玩家当前位置创建 `FilledMapItem`
- 初始缩放级别为 0（1:1）
- 创造模式不消耗物品
- 可堆叠（64个）

### FilledMapItem（已填充地图）
- `inventoryTick()`：更新玩家位置标记和地形数据
- `onItemRightClick()`：打开地图界面
- `onItemUse()`：支持旗帜标记（在旗帜方块上使用）
- 不可堆叠（maxStackSize = 1）
- 支持缩放（0-4级）、锁定、探险地图装饰

## 物品NBT

### FilledMapItem
- `map` (int): 地图ID，关联到 MapDataManager 中的 MapData
- `map_scale_direction` (int): 缩放方向（1=放大，仅在合成时临时存在）
- `Decorations` (list): 装饰列表（探险地图用）
- `display.MapColor` (int): 物品栏显示颜色

## 相关配方

| 配方 | 类 | 输入 | 输出 |
|------|-----|------|------|
| 地图复制 | `MapCloningRecipe` | 已填充地图 + 空地图 | 复制的地图 |
| 地图扩展 | `MapExtendingRecipe` | 已填充地图 + 纸 | 缩放+1的地图 |

## 相关系统

- **地图数据**: `src/common/world/map/` - MapData, MapDataManager, MapDecoration 等
- **制图台**: `CartographyMenu` - 扩展/锁定/复制地图
- **探险地图**: `ExplorationMapFunction` - 战利品表中生成探险地图
- **网络同步**: `MapDataPacket` - 服务端→客户端地图数据同步
