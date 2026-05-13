# 效果层渲染器

本目录包含效果相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `EnergyGlintLayer.hpp/cpp` | 附魔光效层渲染器 |
| `EyesLayer.hpp/cpp` | 发光眼睛层渲染器 |

## EnergyGlintLayer

渲染附魔物品的紫色光效：
- 检查实体所有装备槽位（主手、副手、头盔、胸甲、护腿、靴子）是否有附魔物品
- 使用 `EnchantmentHelper::hasEnchantments()` 检测附魔
- 滚动动画效果
- 叠加混合模式

### shouldRender 逻辑

```cpp
// 检查所有装备槽位
if constexpr (std::is_base_of_v<LivingEntity, TEntity>) {
    // 检查主手物品
    if (!mainHand.isEmpty() && EnchantmentHelper::hasEnchantments(mainHand)) {
        return true;
    }
    // 检查副手、头盔、胸甲、护腿、靴子...
}
return false;
```

### 参考

- MC 1.16.5 `ItemStack.hasEffect()` -> `ItemStack.isEnchanted()`
- MC 1.16.5 `BipedArmorLayer` - 盔甲附魔光效
- MC 1.16.5 `ElytraLayer` - 鞘翅附魔光效

## EyesLayer

渲染实体的发光眼睛：
- 末影人（紫色）
- 蜘蛛/洞穴蜘蛛（红色）
- 幻翼（绿色）

使用叠加混合模式实现发光效果。

## 参考

- MC 1.16.5 EnergyLayer
- MC 1.16.5 AbstractEyesLayer
- MC 1.16.5 EndermanEyesLayer
- MC 1.16.5 SpiderEyesLayer
