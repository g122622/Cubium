# 旋风人 (Breeze)

MC 1.21 新增的敌对生物，在试炼密室中生成。

## 目录结构

```
breeze/
├── BreezeEntity.hpp/cpp   # 旋风人实体（风弹攻击、滑行、长跳）
└── README.md
```

## 内部模块关系

```
MonsterEntity (敌对生物基类)
  └── BreezeEntity — 旋风人
        ├── shootWindCharge() — 发射风弹
        ├── shouldDeflectProjectile() — 偏转投射物（风弹除外）
        └── AI 行为目标（注册在 registerGoals()）
```

## 上下游外部依赖关系

**依赖本模块的地方：**
- `VanillaEntities::registerAll()` — 注册旋风人实体类型
- `EntityTypeIdNumber` — 旋风人实体类型ID缓存

**本模块依赖：**
- `MonsterEntity` — 敌对生物基类
- `WindChargeEntity` — 风弹弹射物实体
- `ProjectileEntity` — 弹射物基类（shouldDeflectProjectile 参数类型）
- `SoundEvents` / `SoundCategory` — 音效播放
- `IWorld` — 世界接口（spawnEntity、playSound）

## 容易踩的坑

1. **旋风人发射位置偏移**：`shootWindCharge()` 中发射 Y 坐标为 `y() + height() * 0.5f + 0.3f`（身体中心偏上 0.3 格），对齐 MC 原版 `Breeze.getFiringYPosition()`。

2. **风弹不被偏转**：`shouldDeflectProjectile()` 对 `WindChargeEntity` 返回 `false`，其他投射物返回 `true`。这确保旋风人不会偏转自己发射的风弹。

3. **AI 行为目标尚未完整**：`registerGoals()` 中 BreezeShootGoal、BreezeLongJumpGoal、BreezeSlideGoal、BreezeShootWhenStuckGoal 均标记为 TODO，当前使用基础 AI 目标替代。`shootWindCharge()` 方法已实现但需要 AI 目标系统调用来触发。
