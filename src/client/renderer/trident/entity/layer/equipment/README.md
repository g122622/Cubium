# 装备层渲染器

本目录包含装备相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `ArmorLayer.hpp/cpp` | 盔甲层渲染器 |
| `HeldItemLayer.hpp/cpp` | 手持物品层渲染器 |
| `HeadLayer.hpp/cpp` | 头部物品层渲染器 |

## ArmorLayer

渲染实体穿戴的盔甲：
- 头盔 (Head)
- 胸甲 (Chest)
- 护腿 (Legs)
- 靴子 (Feet)

### 特性

- 支持皮革染色盔甲
- 网格缓存机制避免每帧创建
- 盔甲模型缓存（延迟初始化）

### 盔甲槽位可见性（MC 1.16.5）

| 槽位 | 显示的部件 |
|------|-----------|
| Head | head, headwear |
| Chest | body, leftArm, rightArm |
| Legs | body, leftLeg, rightLeg |
| Feet | leftLeg, rightLeg |

### 网格缓存

`ArmorLayer` 使用 `ArmorMeshCache` 结构缓存每个槽位的网格：
- 当装备物品变化时自动更新缓存
- 使用 `EntityPipeline::createMesh()` 和 `updateMesh()` 管理网格
- 避免每帧重新创建 GPU 缓冲区

## HeldItemLayer

渲染实体手中的物品：
- 主手物品
- 副手物品

## HeadLayer

渲染实体头部的物品：
- 南瓜
- 玩家头颅
- 其他可穿戴头部物品

## 参考

- MC 1.16.5 BipedArmorLayer
- MC 1.16.5 HeldItemLayer
- MC 1.16.5 HeadLayer
