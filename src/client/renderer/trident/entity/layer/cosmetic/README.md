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
- 检测胸甲槽是否装备鞘翅物品（Items::ELYTRA）
- 使用 `Entity::isElytraFlying()` 检测滑翔状态
- 根据滑翔时的速度向量计算鞘翅展开角度
- 支持蹲伏姿态的角度调整

### 滑翔检测

```cpp
// 使用 EntityFlags::FallFlying 标志位检测
isGliding = entity.isElytraFlying();
```

### 角度计算

| 状态 | X轴角度 | Z轴角度 |
|------|---------|---------|
| 默认 | ~15° | ~-15° |
| 滑翔 | ~20° (动态) | ~-90° (动态) |
| 蹲伏 | ~40° | ~-45° |

### 平滑角度插值（待实现）

MC 1.16.5 中 `AbstractClientPlayerEntity` 有 `rotateElytraX/Y/Z` 字段用于平滑插值。
当前项目的 `ClientEntity` 已有这些字段和 `updateElytraAngles()` 方法，但渲染层使用的是
`Player/LivingEntity` 实体。完整实现需要架构调整，将 `ClientEntity` 与渲染层关联。

## 参考

- MC 1.16.5 CapeLayer
- MC 1.16.5 ElytraLayer
- MC 1.16.5 ElytraModel.setRotationAngles()
