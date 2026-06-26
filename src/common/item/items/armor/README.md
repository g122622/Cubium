# 盔甲物品目录

本目录保存所有盔甲相关的具体物品实现，负责装备槽位、属性统计、染色、渲染颜色和右键穿戴逻辑。

## 目录结构

```text
armor/
├── ArmorItem.hpp/cpp        # 盔甲基类：防御值、韧性、击退抗性、穿戴逻辑
├── DyeableArmorItem.hpp/cpp # 可染色盔甲：display.color 读写
├── ElytraItem.hpp/cpp       # 鞘翅：滑翔功能、耐久消耗
├── HorseArmorItem.hpp/cpp   # 马铠：护甲值、材质路径
└── README.md                # 本文件
```

## 内部模块关系

`ArmorItem` 是所有玩家盔甲的基类，依赖 `armor/ArmorMaterial.hpp` 提供材质数值（防御、韧性、耐久、修复材料等）。`DyeableArmorItem` 继承 `ArmorItem`，额外依赖 `ItemStack` 的自定义标签能力实现染色。`ElytraItem` 直接继承 `Item`，占用胸甲槽位但不提供护甲值。`HorseArmorItem` 独立于玩家盔甲体系，仅供马类实体使用。

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）：**
- `item/core/` - 物品基类、物品堆、物品注册表
- `item/armor/ArmorMaterial.hpp` - 盔甲材质定义
- `item/attribute/ItemAttributeModifiers.hpp` - 属性修饰符
- `entity/core/LivingEntity.hpp` - 活体实体（用于属性汇总）
- `entity/inventory/PlayerInventory.hpp` - 玩家背包（用于装备槽位）

**下游依赖（依赖本目录的模块）：**
- `Items.hpp/cpp` - 原版物品注册
- `entity/passive/HorseEntity.hpp` - 马铠装备逻辑
- 渲染系统 - 盔甲材质、染色颜色渲染
- 背包/装备GUI - 右键装备交互

## 容易踩的坑

- **颜色标签管理**：不要把颜色直接写到别的字段里，统一使用 `display.color`。清除颜色后要把空掉的 `display` 子标签一并删除，否则物品堆比较会认为元数据不同，导致无法堆叠。
- **属性汇总范围**：汇总装备属性时只应该看四个护甲槽，不要把主手、副手混进去。`ArmorItem` 的总属性统计应忽略非盔甲物品，避免把无关物品算进防御值。
- **CHUNK_HEIGHT vs MAX_BUILD_HEIGHT**：若涉及高度判断，注意这两个常量值不同（384 vs 320）且语义不同。
- **马铠装备判断**：`HorseArmorItem` 仅对 `HorseEntity` 有效，其他实体（如驴、骡）使用不同的装备槽逻辑。
- **铜马铠护甲值**：MC 1.21.11 中铜马铠的护甲值为 4（不是 5），与铁马铠（5）不同。其他马铠护甲值：皮革(3)、金(7)、钻石(11)、下界合金(19)。
- **下界合金马铠防火**：下界合金马铠通过 `ItemTags::FIRE_RESISTANT` 标签实现防火效果（免疫火焰/岩浆伤害），与 MC Java 通过 `Item.Properties.fireResistant()` 机制不同。
