# 效果层渲染器

本目录包含效果相关的层渲染器。

## 目录结构

```
effect/
├── EnergyGlintLayer.hpp    # 附魔光效层渲染器模板类
├── EnergyGlintLayer.cpp    # 附魔光效层实现（仅 include）
├── EyesLayer.hpp           # 发光眼睛层渲染器模板类
├── EyesLayer.cpp           # 发光眼睛层实现（含显式实例化）
└── README.md
```

## 内部模块关系

```
EnergyGlintLayer<TEntity>
├── 继承 LayerRenderer<TEntity>
├── 依赖 EnchantmentHelper（检测附魔）
└── 依赖 EntityPipeline（叠加混合渲染）

EyesLayer<TEntity, TModel>
├── 继承 LayerRenderer<TEntity>
├── 依赖 IEntityRenderer<TEntity, TModel>（获取模型）
├── 依赖 ModelRenderer（获取头部变换）
└── 依赖 EntityPipeline（叠加混合渲染）
```

两个层渲染器都使用**叠加混合模式**实现发光效果，渲染后需要恢复 Alpha 混合模式。

## 上下游外部依赖关系

**被谁依赖（下游）：**
- `LivingRenderer` 子类在 `_setupLayers()` 中添加这些层：
  - `CreeperRenderer` → 添加 `EnergyGlintLayer`
  - `SpiderRenderer` → 添加 `EyesLayer`
  - `EndermanRenderer` → 添加 `EyesLayer`

**依赖了谁（上游）：**
- `core/LayerRenderer.hpp` - 层渲染器基类模板
- `core/AnimationContext.hpp` - 动画上下文（包含 ageInTicks）
- `pipeline/EntityPipeline.hpp` - 实体渲染管线（createMesh、bind、drawMesh）
- `model/core/ModelRenderer.hpp` - 模型渲染器（ModelVertex）
- `common/entity/core/LivingEntity.hpp` - 生物实体基类
- `common/item/enchantment/EnchantmentHelper.hpp` - 附魔检测工具（EnergyGlintLayer 使用）

## 容易踩的坑

1. **叠加混合模式必须恢复**：两个层渲染器都使用 `BlendMode::Additive`，渲染完成后必须恢复 `BlendMode::Alpha`，否则后续渲染会出现颜色异常。

2. **EnergyGlintLayer 应该只检查装备槽**：当前实现检查所有装备槽（主手、副手、头盔、胸甲、护腿、靴子），但 MC 1.16.5 中不同实体类型可能有不同的附魔光效渲染逻辑（如盔甲层的光效只渲染盔甲部分）。

3. **EyesLayer 需要父模型引用**：构造时必须传入 `IEntityRenderer` 引用以获取父模型的头部部件，否则无法正确定位眼睛位置。

4. **EyesLayer 眼睛纹理未应用**：当前实现使用叠加颜色渲染，`getEyesTexture()` 获取的纹理尚未绑定到管线，这是一个待完善的功能。

5. **CPU 路径已废弃**：`render()` 方法保留用于向后兼容，但实际渲染逻辑应在 `renderPipeline()` 中实现。

## 参考

- MC 1.16.5 `LayerRenderer`
- MC 1.16.5 `EnergyLayer` - 能量光效基类
- MC 1.16.5 `EndermanEyesLayer` - 末影人眼睛
- MC 1.16.5 `SpiderEyesLayer` - 蜘蛛眼睛
- MC 1.16.5 `RenderType.getEnergySwirl()` - 能量光效渲染类型
