# 外观层渲染器

本目录包含外观相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `CapeLayer.hpp/cpp` | 斗篷层渲染器 |
| `ElytraLayer.hpp/cpp` | 鞘翅层渲染器 |

## CapeLayer

渲染玩家的斗篷：
- 根据玩家移动产生摆动动画
- 支持自定义斗篷纹理

## ElytraLayer

渲染玩家装备的鞘翅：
- 飞行时展开
- 根据飞行角度调整姿态

## 参考

- MC 1.16.5 CapeLayer
- MC 1.16.5 ElytraLayer
