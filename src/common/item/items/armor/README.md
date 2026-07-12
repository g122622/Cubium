# 盔甲物品目录

本目录保存所有盔甲相关的具体物品实现，负责装备槽位、属性统计、染色、渲染颜色和右键穿戴逻辑。

## 目录结构

```text
armor/
├── ArmorItem.hpp/cpp        # 盔甲基类：防御值、韧性、击退抗性、穿戴逻辑
├── DyeableArmorItem.hpp/cpp # 可染色盔甲：display.color 读写
├── ElytraItem.hpp/cpp       # 鞘翅：滑翔功能、耐久消耗
├── HorseArmorItem.hpp/cpp   # 马铠：护甲值、材质路径
├── WolfArmorItem.hpp/cpp    # 狼铠：可染色、犰狳鳞甲修复、64点耐久
├── NautilusArmorItem.hpp/cpp# 鹦鹉螺铠甲：不可损坏、5种材质护甲值
└── README.md                # 本文件
```

## 内部模块关系

`ArmorItem` 是所有玩家盔甲的基类，依赖 `armor/ArmorMaterial.hpp` 提供材质数值（防御、韧性、耐久、修复材料等）。`DyeableArmorItem` 继承 `ArmorItem`，额外依赖 `ItemStack` 的自定义标签能力实现染色。`ElytraItem` 直接继承 `Item`，占用胸甲槽位但不提供护甲值。`HorseArmorItem` 独立于玩家盔甲体系，仅供马类实体使用，通过构造参数直接指定护甲值和材质纹理路径。`WolfArmorItem` 继承 `DyeableArmorItem`，使用 `ArmadilloScuteArmorMaterial` 和 `ArmorSlot::Body` 槽位，可染色（默认颜色 0xA06540），64 点耐久，防御值 11（由材质的 Body 槽位防御值提供），支持使用犰狳鳞甲修复。`NautilusArmorItem` 独立于玩家盔甲体系（继承 `Item` 而非 `ArmorItem`），仅供鹦鹉螺类实体使用，不可损坏（无耐久度），护甲值由 `ArmorMaterial::getDefense(ArmorSlot::Body)` 推导（铜=4, 铁=5, 金=7, 钻石=11, 下界合金=19），装备音效和修复材料从 `ArmorMaterial` 获取。

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
- **下界合金鹦鹉螺铠甲防火**：下界合金鹦鹉螺铠甲同样通过 `ItemTags::FIRE_RESISTANT` 标签实现防火效果（免疫火焰/岩浆伤害），与下界合金马铠机制一致，与 MC Java 通过 `Item.Properties.fireResistant()` 机制不同。
- **鹦鹉螺铠甲护甲值**：鹦鹉螺铠甲的护甲值由 `ArmorMaterial::getDefense(ArmorSlot::Body)` 推导，与 MC 1.21.11 `Item.Properties.nautilusArmor(ArmorMaterial)` 通过 `ArmorMaterial.createAttributes(ArmorType.BODY)` 取护甲值的语义一致。各材质护甲值为：铜=4, 铁=5, 金=7, 钻石=11, 下界合金=19，与马铠护甲值一致。`NautilusArmorItem` 继承自 `Item` 而非 `ArmorItem`（与 `HorseArmorItem` 设计一致），不参与玩家盔甲装备系统、不可破坏，但护甲值仍从材质的 Body 槽位防御值统一获取，避免双份维护。
- **狼铠默认颜色**：`WolfArmorItem::getDefaultColor()` 返回 0xA06540（犰狳鳞甲棕色），对应 MC Java 中 `DyeableArmorItem.getDefaultColor()` 的狼铠默认色。
- **狼铠防御值**：`WolfArmorItem` 使用 `ArmorSlot::Body` 槽位，基类 `ArmorItem::getDefense()` 通过 `ArmadilloScuteArmorMaterial::getDefense(ArmorSlot::Body)` 返回防御值 11。属性修饰符在基类构造函数中通过 `_buildAttributeModifiers(getDefense())` 自动构建，使用 `ARMOR_MODIFIER_UUID_BODY` 和 `EquipmentSlot::Body`。

## ArmorItem 属性修饰符系统

`ArmorItem` 在构造时通过 `_buildAttributeModifiers(i32 defense)` 预构建属性修饰符，存储在 `m_attributeModifiers` 成员中。该方法为 `protected`，子类可在构造函数中调用以重建属性修饰符（如 `WolfArmorItem` 使用非标准防御值时）。

### 新增方法

- **`getAttributeModifiers(i32 equipmentSlot)`** (重写 `Item::getAttributeModifiers(i32)`)
  - 当槽位匹配盔甲槽位时（如 Head 槽位的头盔），返回预构建的修饰符
  - 不匹配时返回空修饰符
  - 对应 MC 原版 `ArmorItem.getAttributeModifiers(EquipmentSlot)`
  - 修饰符条目使用属性注册名字符串（如 `"generic.armor"`）而非 `const Attribute*` 指针，避免悬挂指针问题

### 修饰符内容

每个盔甲物品根据材质提供以下修饰符：
1. **护甲值** (`generic.armor`) — 加法操作，值为 `getDefense()`
2. **护甲韧性** (`generic.armor_toughness`) — 加法操作，值为 `getToughness()`（铁/皮革/链甲/金/铜的韧性为 0）
3. **击退抗性** (`generic.knockback_resistance`) — 加法操作，仅当 `getKnockbackResistance() > 0` 时添加（下界合金 0.1，龟壳无）
