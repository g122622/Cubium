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
- UV 滚动动画效果（通过 `calculateGlintOffset` 计算）
- **叠加混合模式（Additive Blending）实现发光效果**

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

### renderPipeline 渲染流程

```cpp
// 1. 计算光效滚动偏移
f32 glintOffset = calculateGlintOffset(static_cast<f32>(context.ageInTicks));

// 2. 构建光效网格（立方体包围盒，UV 随 offset 滚动）
buildGlintMesh(glintOffset, vertices, indices);

// 3. 切换到叠加混合模式
pipeline.bind(cmd, pipeline::BlendMode::Additive);

// 4. 绘制光效网格（紫色叠加效果）
pipeline.drawMesh(cmd, mesh, glintTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

// 5. 恢复 Alpha 混合模式
pipeline.bind(cmd, pipeline::BlendMode::Alpha);
```

### 混合模式说明

| 模式 | 公式 | 用途 |
|------|------|------|
| Additive | `src * srcAlpha + dst * 1` | 发光效果、附魔光效 |
| Alpha | `src * srcAlpha + dst * (1 - srcAlpha)` | 标准半透明 |

### 光效参数

- **颜色**: 紫色 `(0.5, 0.0, 1.0, 0.5)` - 参考 MC 1.16.5 附魔光效
- **缩放**: 1.01 倍避免 z-fighting
- **滚动速度**: `offset = fmod(ageInTicks * 0.01, 1.0)`

### 参考

- MC 1.16.5 `ItemStack.hasEffect()` -> `ItemStack.isEnchanted()`
- MC 1.16.5 `BipedArmorLayer` - 盔甲附魔光效
- MC 1.16.5 `ElytraLayer` - 鞘翅附魔光效
- MC 1.16.5 `RenderType.getEnergySwirl()` - 能量光效渲染类型

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
