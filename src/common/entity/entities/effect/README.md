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

- 由滞留药水创建
- 效果半径随时间减小
- 等待时间后开始生效
- 定期重新应用效果

## 经验球

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
| LightningBoltEntity | ⚠️ 框架完成，TODO需填充 |
| AreaEffectCloudEntity | ⚠️ 框架完成，TODO需填充 |
| ExperienceOrbEntity | ⚠️ 框架完成，TODO需填充 |
| ArmorStandEntity | ⚠️ 框架完成，TODO需填充 |
