# 核心层渲染器

本目录包含层渲染器系统的核心基类模板。

## 目录结构

```
core/
├── LayerRenderer.hpp    # 层渲染器基类模板（定义渲染层接口）
└── README.md
```

## 内部模块关系

`LayerRenderer` 是一个模板基类，定义了层渲染器的接口：
- 提供 `render()` 方法（CPU路径，已废弃，默认空实现）
- 提供 `renderPipeline()` 方法（GPU管线路径，主要渲染方法）
- 提供 `shouldRender()` 方法（条件渲染检查，默认返回 true）

## 上下游外部依赖关系

**被谁依赖（下游）：**
- `LivingRenderer` - 生物渲染器基类，管理 `LayerRenderer` 实例列表并调用渲染
- 所有具体层渲染器实现：
  - `equipment/` - ArmorLayer, HeldItemLayer, HeadLayer
  - `cosmetic/` - CapeLayer, ElytraLayer
  - `entity/` - SaddleLayer, SheepWoolLayer, VillagerLayer, ArrowLayer, HeldBlockLayer, WolfCollarLayer
  - `effect/` - EnergyGlintLayer, EyesLayer

**依赖了谁（上游）：**
- `AnimationContext` - 动画上下文，包含渲染所需的动画参数
- `EntityPipeline` - 实体渲染管线（前向声明）
- `EntityModel` / `ModelRenderer` - 模型相关类型

## 容易踩的坑

1. **CPU 路径已废弃**：`render()` 方法保留用于向后兼容，新代码应实现 `renderPipeline()` 方法使用 GPU 管线路径。

2. **默认实现陷阱**：`render()` 和 `renderPipeline()` 都有默认空实现。如果子类只实现了 `render()` 但外部调用 `renderPipeline()`，会通过默认实现转发到 `render()`。但最佳实践是直接实现 `renderPipeline()`。

3. **shouldRender 条件检查**：层渲染器应该重写 `shouldRender()` 来检查是否满足渲染条件（如装备槽位是否有物品、是否开启了特定选项等），避免不必要的渲染调用。

## 参考

- MC 1.16.5 LayerRenderer
