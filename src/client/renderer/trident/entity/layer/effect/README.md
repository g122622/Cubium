# 效果层渲染器

本目录包含效果相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `EnergyGlintLayer.hpp/cpp` | 附魔光效层渲染器 |
| `EyesLayer.hpp/cpp` | 发光眼睛层渲染器 |

## EnergyGlintLayer

渲染附魔物品的紫色光效：
- 滚动动画
- 叠加混合模式

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
