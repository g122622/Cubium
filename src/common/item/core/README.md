# Item 核心模块

本目录包含物品系统的核心类型和接口。

## 文件说明

| 文件 | 职责 |
|------|------|
| `Item.hpp/cpp` | 物品基类，所有物品类型的父类 |
| `ItemStack.hpp/cpp` | 物品堆，表示游戏中的一个物品实例（包含物品类型、数量、耐久、附魔和结构化自定义标签） |
| `ItemRegistry.hpp/cpp` | 物品注册表，管理所有物品的注册和查找 |
| `ItemProperties.hpp/cpp` | 物品属性构建器，流畅接口模式 |
| `ItemGroup.hpp/cpp` | 创造模式物品组（标签页） |
| `Rarity.hpp/cpp` | 物品稀有度枚举 |
| `UseAction.hpp` | 物品使用动作枚举（EAT、DRINK、BLOCK等） |

## 依赖关系

```
core/
  ├── 依赖 common/core/Types.hpp
  ├── 依赖 common/resource/ResourceLocation.hpp
  ├── 依赖 common/network/packet/PacketSerializer.hpp (ItemStack序列化)
  └── 依赖 item/enchantment/EnchantmentContainer.hpp (ItemStack附魔)
```

## 使用示例

```cpp
// 注册物品
auto& stick = ItemRegistry::instance().registerItem(
    ResourceLocation("minecraft:stick"),
    ItemProperties().maxStackSize(64)
);

// 创建物品堆
ItemStack stack(stick, 32);

// 检查物品堆
if (!stack.isEmpty()) {
    std::cout << stack.getDisplayName() << " x" << stack.getCount();
}
```

## 注意事项

- Item类是抽象基类，不应直接实例化
- ItemStack是不可变值类型，修改操作返回新的ItemStack
- ItemStack 现在支持结构化自定义标签，JSON 序列化会通过 `Tag` 字段保存这些数据，便于染色、药水和未来扩展
- ItemRegistry是单例，在游戏初始化时注册所有物品

## ItemStack 堆叠逻辑

`canMergeWith()` 方法用于判断两个物品堆是否可以合并，与 MC 1.16.5 对齐：

**比较项**：
1. 物品类型 (`m_item`)
2. 耐久度 (`m_damage`) - 仅对可损坏物品
3. 修复成本 (`m_repairCost`) - 铁砧操作次数
4. 自定义名称 (`m_customName`)
5. Lore 描述 (`m_lore`)
6. 药水 ID (`m_potionId`)
7. 附魔 (`m_enchantments`) - **相同附魔可以堆叠**
8. 自定义数据 (`m_customData`)

**重要**：附魔物品堆叠逻辑在 MC 1.16.5 中是基于 NBT 标签完全相等判断的。如果两个物品有相同的附魔，它们可以堆叠。
