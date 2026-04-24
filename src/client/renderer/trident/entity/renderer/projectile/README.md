# 投掷物渲染器

本目录包含投掷物类实体的渲染器实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `ItemEntityRenderer.hpp/cpp` | 物品实体渲染器 |
| `ExperienceOrbRenderer.hpp/cpp` | 经验球渲染器 |
| `ProjectileRenderers.hpp/cpp` | 箭矢/三叉戟渲染器 |

## 渲染器类

### ItemEntityRenderer（物品实体渲染器）
渲染掉落在世界中的物品实体。物品以 3D 方式浮动渲染，具有上下浮动和旋转动画。

### ExperienceOrbRenderer（经验球渲染器）
渲染世界中的经验球实体，带绿色发光效果和浮动动画。

### ArrowRenderer（箭矢渲染器）
渲染普通箭矢，带抖动动画和朝向旋转。

### SpectralArrowRenderer（光灵箭渲染器）
渲染光灵箭，带发光效果。

### TridentRenderer（三叉戟渲染器）
渲染投掷的三叉戟。

## 参考

- MC 1.16.5 ItemEntityRenderer
- MC 1.16.5 ExperienceOrbRenderer
- MC 1.16.5 ArrowRenderer
- MC 1.16.5 TridentRenderer
