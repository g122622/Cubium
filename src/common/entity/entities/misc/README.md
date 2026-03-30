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

- 默认引信80tick（4秒）
- 默认爆炸半径4.0
- 受重力影响
- 可设置爆炸半径

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
| TNTEntity | ⚠️ 框架完成，TODO需填充 |
| EyeOfEnderEntity | ⚠️ 框架完成，TODO需填充 |
| ConduitEntity | ⚠️ 框架完成，TODO需填充 |
| EvokerFangsEntity | ⚠️ 框架完成，TODO需填充 |
| WardenWarningEffect | ⚠️ 框架完成，TODO需填充 |
