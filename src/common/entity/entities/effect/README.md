# 效果实体

本目录包含非生物、非物品的效果类实体。

## 目录结构

```
effect/
├── EffectEntities.hpp/cpp   # 效果实体定义
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 特性 |
|------|------|------|
| EnderCrystalEntity | 末影水晶 | 治愈末影龙、光束、爆炸 |
| LightningBoltEntity | 闪电 | 伤害实体、生成火焰 |
| AreaEffectCloudEntity | 区域效果云 | 滞留药水效果 |
| ExperienceOrbEntity | 经验球 | 玩家拾取获得经验 |
| ArmorStandEntity | 盔甲架 | 展示盔甲、可摆姿势 |

## 末影水晶

- 治愈末影龙
- 显示底部基岩选项
- 光束指向传送门
- 被攻击时爆炸

### 光束粒子效果

当末影水晶有光束目标（指向末地传送门）时，客户端每 tick 生成 EndRod 粒子：
- 粒子类型：`ParticleTypeId::EndRod`
- 粒子位置：水晶中心上方（y+1），带随机偏移
- 粒子速度：向光束目标方向移动（速度 0.1~0.15）
- 旋转值：`m_innerRotation` 在构造时随机初始化（0-99999），每 tick 递增用于渲染动画

### 爆炸

当末影水晶被摧毁时（`explode()` 方法）：
- 爆炸半径：6.0 格
- 爆炸模式：`Destroy`（破坏方块并掉落物品）
- 不生成火焰
- 爆炸位置：水晶当前位置
- 参考 MC 1.16.5: `this.world.createExplosion((Entity)null, this.getPosX(), this.getPosY(), this.getPosZ(), 6.0F, Explosion.Mode.DESTROY);`

## 闪电

- 伤害范围3格内实体
- 伤害值5点
- 可能点燃实体（8秒）
- 可能生成火焰
- 存在时间30tick

## 区域效果云

### MC 1.16.5 对齐

AreaEffectCloudEntity 已完整实现以下功能：

| 功能 | 状态 |
|------|------|
| 效果列表存储 | ✅ 完成 |
| 效果应用到范围内实体 | ✅ 完成 |
| 半径随时间衰减 | ✅ 完成 |
| 半径使用时衰减 | ✅ 完成 |
| 等待时间机制 | ✅ 完成 |
| 重应用延迟映射 | ✅ 完成 |
| 颜色自动计算 | ✅ 完成 |
| 拥有者追踪 | ✅ 完成 |

### 关键参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| radius | 3.0F | 初始半径 |
| duration | 600 ticks | 持续时间（30秒） |
| waitTime | 20 ticks | 等待时间（1秒） |
| reapplicationDelay | 20 ticks | 重应用延迟 |
| radiusOnUse | 0.0F | 每次应用效果时半径变化 |
| radiusPerTick | 0.0F | 每tick半径变化 |
| durationOnUse | 0 | 每次应用效果时持续时间变化 |

### 苦力怕药水云参数

当苦力怕身上有药水效果时，爆炸后会生成滞留药水云：

| 参数 | 值 |
|------|-----|
| 初始半径 | 2.5F |
| radiusOnUse | -0.5F |
| waitTime | 10 ticks |
| duration | 300 ticks |
| radiusPerTick | -2.5/300 |

参考 MC 1.16.5 CreeperEntity.spawnLingeringCloud()

## 实现状态

| 经验值范围 | 颜色 |
|-----------|------|
| 1-5 | 黄色 |
| 6-20 | 绿色 |
| 21-100 | 青色 |
| 100+ | 红色 |

- 追踪附近玩家
- 自动消失时间6000tick
- 最大经验值2477

## 盔甲架

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| hasGravity | true | 是否受重力 |
| marker | false | 标记模式 |
| basePlate | true | 是否有底座 |
| arms | false | 是否显示手臂 |
| small | false | 是否小型 |

### 身体部位旋转

- 头部
- 身体
- 左臂
- 右臂
- 左腿
- 右腿

## 实现状态

| 组件 | 状态 |
|------|------|
| EnderCrystalEntity | ✅ 完成 - 光束粒子效果、爆炸实现 |
| LightningBoltEntity | ✅ 完成 - 伤害实体、点燃方块、音效 |
| AreaEffectCloudEntity | ✅ 完成 - 效果应用、半径衰减、苦力怕药水云 |
| ExperienceOrbEntity | ⚠️ 框架完成，TODO需填充 |
| ArmorStandEntity | ⚠️ 框架完成，TODO需填充 |
