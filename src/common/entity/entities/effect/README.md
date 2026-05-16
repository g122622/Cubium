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

### MC 1.16.5 对齐

EnderCrystalEntity 已完整实现以下功能：

| 功能 | 状态 |
|------|------|
| 光束目标设置 | ✅ 完成 |
| 光束粒子效果 | ✅ 完成 |
| 底座显示控制 | ✅ 完成 |
| 爆炸机制 | ✅ 完成 |
| 治愈末影龙 | ✅ 完成 |
| 内部旋转动画 | ✅ 完成 |

#### 治愈末影龙 (healDragon)

当末影水晶调用 `healDragon()` 方法时：

1. **冷却检查**: 如果 `m_healCooldown > 0`，直接返回
2. **范围搜索**: 在 32 格范围内搜索末影龙实体
3. **距离验证**: 找到最近的存活末影龙
4. **治愈逻辑**: 对末影龙造成负伤害（`hurt(fireDamage, -1.0f)`）
5. **光束目标**: 设置光束指向末影龙位置
6. **冷却设置**: 设置 `m_healCooldown = HEAL_COOLDOWN (10 ticks)`
7. **末影龙引用**: 设置末影龙的 `closestEnderCrystal` 为当前水晶

参考 MC 1.16.5 `EnderCrystalEntity.healDragon()`

#### 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `HEAL_COOLDOWN` | 10 | 治愈冷却时间 (ticks) |
| `EXPLOSION_RADIUS` | 6.0f | 爆炸半径 (方块) |
| `HEAL_RANGE` | 32.0f | 治愈搜索范围 (方块) |

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

### MC 1.16.5 对齐

LightningBoltEntity 已完整实现以下功能：

| 功能 | 状态 |
|------|------|
| 伤害范围内实体 | ✅ 完成 |
| 点燃方块 | ✅ 完成 |
| 播放雷声音效 | ✅ 完成 |
| 生成火焰（根据难度） | ✅ 完成 |
| 随机闪烁效果 | ✅ 完成 |
| 客户端天空闪烁 | ✅ 完成 |

### 核心机制

当闪电击中时（`lightningState == 2`）：

1. **服务端**：
   - 根据难度点燃周围方块（NORMAL/HARD: 4 格，EASY/PEACEFUL: 0 格）
   - 播放雷声音效（音量 10000，音调 0.8-1.0）
   - 对 3x6x3 范围内的 LivingEntity 造成 5 点闪电伤害

2. **客户端**：
   - 调用 `world.setTimeLightningFlash(2)` 设置天空闪烁
   - 渲染器将天空颜色向白色混合，产生闪烁效果

### 闪电闪烁效果

客户端实现：
- `ClientWeather::setTimeLightningFlash(i32 time)` - 设置闪烁时间
- `ClientWeather::tickLightningFlash()` - 每 tick 递减闪烁时间
- `ClientWeather::lightningFlashBrightness()` - 返回亮度因子 (0.0 或 1.0)
- `SkyRenderer::setLightningFlashBrightness(f64)` - 设置渲染器闪烁亮度

参考 MC 1.16.5:
- `LightningBoltEntity.tick()`: `world.setTimeLightningFlash(2)`
- `Minecraft.runTick()`: `world.setTimeLightningFlash(time - 1)`
- `WorldRenderer.renderSky()`: 天空颜色向白色混合

### 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| DAMAGE_RANGE | 3.0f | 伤害范围半径 |
| DAMAGE_AMOUNT | 5.0f | 伤害值 |
| FIRE_IGNITION_NORMAL_HARD | 4 | NORMAL/HARD 难度点燃数 |
| FIRE_IGNITION_EASY_PEACEFUL | 0 | EASY/PEACEFUL 难度点燃数 |

### 随机闪烁

闪电会在生命周期内多次闪烁：
- `boltLivingTime`: 1-3 次闪烁
- `boltVertex`: 随机种子，控制闪烁间隔
- 每次"复活"时生成新的随机种子用于渲染

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
