# 杂项实体

本目录包含其他类别的实体。

## 目录结构

```
misc/
├── MiscEntities.hpp/cpp     # 杂项实体定义
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 特性 |
|------|------|------|
| FallingBlockEntity | 下落方块 | 沙子、砾石下落，可造成伤害 |
| TNTEntity | TNT实体 | 倒计时爆炸 |
| EyeOfEnderEntity | 末影之眼 | 飞向要塞 |
| ConduitEntity | 潮涌核心 | 水下效果、攻击敌人 |
| EvokerFangsEntity | 唤魔者尖牙 | 地刺攻击 |

## 下落方块

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| hurtEntities | false | 是否伤害实体 |
| fallStartY | - | 下落起始Y坐标 |
| placeBlock | true | 是否放置方块 |

### 行为

- 受重力影响下落
- 落地后尝试放置方块
- 如果无法放置则掉落物品
- 可配置是否伤害实体

## TNT实体

TNT实体是被激活的TNT方块，倒计时后爆炸。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| fuse | 0 | 引信倒计时（tick） |
| explosionRadius | 4.0f | 爆炸半径 |
| exploded | false | 是否已爆炸 |
| owner | nullptr | 点燃者（用于伤害归属） |

### 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| DEFAULT_FUSE | 80 | 默认引信时间（4秒） |

### 行为

1. **点燃**：调用 `ignite()` 设置引信时间为80 tick
2. **倒计时**：每tick引信减1
3. **物理**：重力加速度0.04/tick，空气阻力0.98/tick，地面弹跳系数0.7/0.5
4. **爆炸**：引信归零时调用 `createExplosion()`，爆炸模式为 `Break`（破坏方块但不掉落物品）
5. **粒子**：客户端模式下，每tick有1/3概率在TNT上方生成Smoke粒子，粒子轻微向上飘动

### 粒子效果

客户端模式下（`isClientSide() == true`）：
- 每tick有1/3概率生成烟雾粒子
- 粒子类型：`ParticleTypeId::Smoke`
- 粒子位置：TNT上方，带随机偏移（±0.3格）
- 粒子速度：轻微向上飘动（0.02 + random*0.02）

### 工厂方法

```cpp
static std::unique_ptr<Entity> create(IWorld* world);
```

### 注册

TNTEntity 已在 `VanillaEntities::doRegisterAll()` 中注册，实体类型为 `minecraft:tnt`。

### 参考

- MC 1.16.5 `net.minecraft.entity.item.TNTEntity`
- 爆炸系统：`src/common/world/explosion/Explosion.hpp`

## 末影之眼

- 飞向最近的要塞
- 有几率碎裂掉落
- 最大飞行时间80tick

## 潮涌核心

### 效果

- 效果半径：42格
- 给予潮涌能量效果
- 攻击附近敌对生物（半径8格）

### 攻击

- 伤害值：4点
- 攻击间隔：40tick

## 唤魔者尖牙

### 行为

1. 延迟出现
2. 尖牙冒出动画
3. 攻击范围内实体
4. 消失

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| delay | 0 | 出现延迟 |
| damage | 6.0 | 伤害值 |
| warmup | 14 | 攻击准备时间 |
| lifetime | 22 | 总生命周期 |

## 实现状态

| 组件 | 状态 |
|------|------|
| FallingBlockEntity | ⚠️ 框架完成，TODO需填充 |
| TNTEntity | ✅ 完成 - 点燃、爆炸、物理、实体注册 |
| EyeOfEnderEntity | ⚠️ 框架完成，TODO需填充 |
| ConduitEntity | ⚠️ 框架完成，TODO需填充 |
| EvokerFangsEntity | ⚠️ 框架完成，TODO需填充 |
| WardenWarningEffect | ⚠️ 框架完成，TODO需填充 |
