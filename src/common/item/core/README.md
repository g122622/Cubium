# Item 核心模块

本目录包含物品系统的核心类型和接口。

## 文件说明

| 文件 | 职责 |
|------|------|
| `Item.hpp/cpp` | 物品基类，所有物品类型的父类 |
| `ItemStack.hpp/cpp` | 物品堆，表示游戏中的一个物品实例（包含物品类型、数量、耐久、附魔等） |
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
- ItemRegistry是单例，在游戏初始化时注册所有物品
