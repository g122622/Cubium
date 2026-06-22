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
- 提供 `renderPipeline()` 方法（GPU管线路径，主要渲染方法，子类必须实现）
- 提供 `render()` 方法（CPU路径，已废弃，基类保留空实现以兼容旧代码路径）
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

1. **CPU 路径已移除**：所有子类的 `render()` override 已移除，所有渲染逻辑均在 `renderPipeline()` GPU 管线路径中实现。基类 `render()` 保留空实现以兼容 `LivingRenderer::render()` 旧路径。

2. **renderPipeline() 必须实现**：子类必须直接实现 `renderPipeline()` 方法，基类的默认实现为空（不再回退到 `render()`）。

3. **shouldRender 条件检查**：层渲染器应该重写 `shouldRender()` 来检查是否满足渲染条件（如装备槽位是否有物品、是否开启了特定选项等），避免不必要的渲染调用。

## 参考

- MC 1.16.5 LayerRenderer
