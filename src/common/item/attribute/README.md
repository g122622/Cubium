#物品属性修饰符

管理物品在装备槽位上提供的属性修饰符（攻击伤害、护甲值、攻击速度等）。

##目录结构

```text attribute /
├── ItemAttributeModifiers.hpp /
    cpp #物品属性修饰符管理器及构建器
└── README.md #本文件
```

    ##内部模块关系

`ItemAttributeModifiers` 存储物品的属性修饰符列表，每个修饰符条目包含属性注册名、`AttributeModifier` 和装备槽位。`ItemAttributeModifiersBuilder` 提供流式接口构建修饰符。

``` ItemAttributeModifiers（修饰符列表）
    ├── Entry
{
    attributeName, modifier, equipmentSlot
}
└── ItemAttributeModifiersBuilder（流式构建器）
            ├── attackDamage() / attackSpeed()
            ├── armor() /
        armorToughness()
            ├── knockbackResistance()
            └── movementSpeed()
```

        ##上下游外部依赖关系

            ** 上游依赖（本目录依赖的模块）：* *
        - `entity / attribute / AttributeModifier.hpp` -
    属性修饰符定义 - `entity / attribute / Attributes.hpp` -
    属性注册名常量（如 `Attributes::ARMOR`）

            ** 下游依赖（依赖本目录的模块）：* *
        - `item / items / armor / ArmorItem` -
    盔甲属性修饰符（护甲值、韧性、击退抗性） - `item / items / tool / SwordItem` - 剑属性修饰符（攻击伤害、攻击速度）
    - `item / items / tool / ToolItem` - 工具属性修饰符（攻击伤害、攻击速度） - `entity / core / LivingEntity` -
    装备变化检测时遍历修饰符并应用到 `AttributeMap`

    ##容易踩的坑

    -
    **Entry
     使用属性注册名字符串而非指针**：`Entry::attributeName` 存储的是属性注册名（如 `"generic.armor"`），而非 `const
     Attribute*` 指针。这是因为 `Attributes::
         armor()` 等工厂方法返回 `unique_ptr`，其生命周期在构造结束后即销毁，存储裸指针会导致悬挂指针。消费端通过 `AttributeMap::
             addModifier(attributeName, modifier)` 直接使用字符串进行属性查找。
    -
    **槽位使用 `i32` 而非 `EquipmentSlot`**：`Entry::
         equipmentSlot` 使用 `i32` 类型存储槽位值，避免与 `EquipmentSlot` 枚举的循环依赖。调用方通过 `static_cast<i32>(
             EquipmentSlot::MainHand)` 转换。
