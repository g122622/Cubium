# BlockItem 模块

方块物品实现，将方块包装为可手持/放置的物品。

## 目录结构

```
block/
├── BannerItem.hpp/cpp        # 旗帜物品
├── BlockItem.hpp/cpp         # 方块物品基类（放置逻辑、NBT数据传递）
├── BlockItemRegistry.hpp/cpp # 方块物品注册表
└── WallOrFloorItem.hpp/cpp   # 墙上/地面放置物品（按钮、压力板等）
```

## 内部模块关系

```
Item (基类，item/core/)
  └── BlockItem (方块物品基类)
        ├── BannerItem (旗帜)
        └── WallOrFloorItem (墙上/地面放置)
BlockItemRegistry ──注册──→ BlockItem（方块→物品映射）
```

BlockItem 核心职责：
- 放置逻辑：碰撞检查、替换判断、方向计算
- NBT 数据传递：`applyBlockStateFromNBT` 从物品的 BlockStateTag 恢复方块状态属性，`setTileEntityNBT` 从 BlockEntityTag 恢复方块实体数据（受 `onlyOpsCanSetNbt()` 权限控制）

## 上下游外部依赖关系

**依赖上游：**
- `item/core/` - Item 基类、ItemStack
- `world/block/` - Block、BlockState、方块属性
- `world/blockentity/` - BlockEntity（NBT 写入、onlyOpsCanSetNbt 权限检查）
- `entity/` - Player（OP 权限检查）
- `util/nbt/` - NBT 读写

**被下游依赖：**
- `item/Items.hpp` - 物品注册
- `server/` - 服务端物品管理
- `client/` - 客户端渲染

## 容易踩的坑

- BlockItem 放置时会排除放置者实体进行碰撞检查，无碰撞箱方块（水、空气）跳过此检查
- `applyBlockStateFromNBT` 使用 `StateHolder::withValueIndex` 而非 `with()`，因为属性类型在反序列化时未知
- `setTileEntityNBT` 仅当玩家有 OP 权限或 `onlyOpsCanSetNbt()` 返回 false 时才写入方块实体 NBT
