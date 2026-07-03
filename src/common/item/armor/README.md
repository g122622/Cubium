# 盔甲材质模块

本目录负责盔甲材质数据，包含耐久、防御、附魔权重、装备音效和修复材料等原版语义。

## 目录结构

```
armor/
├── ArmorMaterial.hpp  # 盔甲材质接口、ArmorSlot枚举及八种原版材质类定义
├── ArmorMaterial.cpp  # 原版材质数值实现与ArmorMaterials全局实例
└── README.md
```

## 内部模块关系

```
ArmorSlot (枚举) ──→ ArmorMaterial (接口)
                           ▲
        ┌──────────────────┼──────────────────┐
        │                  │                  │
  LeatherArmorMaterial  IronArmorMaterial  DiamondArmorMaterial  ... (共8种材质类)
```

- `ArmorSlot`：定义头、胸、腿、脚、身体五个槽位（Head/Chest/Legs/Feet 为玩家护甲，Body 为非玩家实体护甲如狼铠、鹦鹉螺铠甲、马铠）
- `ArmorMaterial`：抽象接口，定义耐久、防御、韧性、附魔能力等核心方法
- 八个材质类：实现具体数值，通过`ArmorMaterials`命名空间暴露全局实例

## 上下游外部依赖关系

**依赖方（上游）：**
- `common/entity/core/LivingEntity.hpp` - `EquipmentSlot`枚举（槽位索引转换）
- `common/item/crafting/Ingredient.hpp` - 修复材料配方成分
- `common/item/Items.hpp` - 物品注册（获取修复材料物品指针）
- `common/sound/SoundEvent.hpp` - 音效事件

**被依赖方（下游）：**
- `item/items/armor/ArmorItem` - 消费`ArmorMaterial`获取防御值、韧性、耐久、修复规则
- `item/items/armor/`下各盔甲物品类 - 在构造时注入材质实例
- `client/renderer/trident/entity/layer/equipment/ArmorLayer` - 使用`getAssetId()`构建盔甲纹理路径

## 容易踩的坑

- **`getName()` vs `getAssetId()`**：`getName()`返回材质逻辑ID（如`"chain"`、`"turtle"`），`getAssetId()`返回纹理资源ID（如`"chainmail"`、`"turtle_scute"`），两者不同！构建纹理路径必须用`getAssetId()`。
- **`getRepairMaterial()`依赖物品注册**：调用前必须确保`Items::initialize()`已完成，否则返回空原料。测试代码需注意启动顺序。
- **装备音效不能为空**：必须返回原版`minecraft:item.armor.equip_*`标识，空`SoundEvent`会导致运行时问题。
- **海龟壳只有头盔**：`TurtleArmorMaterial`对非`Head`槽位返回防御值0，调用方应自行过滤无效槽位。
- **Body 槽位**：`ArmorSlot::Body` 用于非玩家实体护甲（狼铠、鹦鹉螺铠甲、马铠），对应 `EquipmentSlot::Body`。玩家不可装备 Body 槽位护甲。各材质的 Body 槽位防御值参考 MC 1.21.11 `ArmorMaterials`：皮革=3, 锁链=4, 铜=4, 铁=5, 金=7, 钻石=11, 海龟=5, 下界合金=19, 犰狳鳞甲=11。