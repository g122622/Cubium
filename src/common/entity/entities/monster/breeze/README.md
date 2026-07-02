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
        ├── deflection() — 偏转投射物（重写 Entity::deflection，风弹除外，播放偏转音效）
        ├── die() — 死亡掉落狂风杖（仅被玩家击杀时，1-2个，受抢夺附魔影响）
        └── AI 行为目标（注册在 registerGoals()）
```

## 上下游外部依赖关系

**依赖本模块的地方：**
- `VanillaEntities::registerAll()` — 注册旋风人实体类型
- `EntityTypeIdNumber` — 旋风人实体类型ID缓存

**本模块依赖：**
- `MonsterEntity` — 敌对生物基类
- `WindChargeEntity` — 风弹弹射物实体
- `ProjectileEntity` — 弹射物基类（deflection 参数类型）
- `ProjectileDeflection` — 弹射物偏转类型枚举
- `SoundEvents` / `SoundCategory` — 音效播放
- `IWorld` — 世界接口（spawnEntity、playSound）
- `Items::BREEZE_ROD` — 狂风杖物品（死亡掉落）
- `ItemDropHelper` — 物品掉落工具
- `EnchantmentHelper` — 抢夺附魔查询
- `Player` — 玩家实体（判断击杀者、获取武器附魔）

## 容易踩的坑

1. **旋风人发射位置偏移**：`shootWindCharge()` 中发射 Y 坐标为 `y() + height() * 0.5f + 0.3f`（身体中心偏上 0.3 格），对齐 MC 原版 `Breeze.getFiringYPosition()`。

2. **风弹不被偏转**：`deflection()` 对 `WindChargeEntity` 返回 `ProjectileDeflection::None`，其他投射物返回 `ProjectileDeflection::Reverse`（前提是实体类型属于 `DEFLECTS_PROJECTILES` 标签）。这确保旋风人不会偏转自己发射的风弹。偏转时播放 `ENTITY_BREEZE_DEFLECT` 音效。

3. **AI 行为目标**：`registerGoals()` 中已实现旋风人特有的四个 AI 目标：
   - `BreezeShootGoal`（优先级2）：向目标投掷风弹，充能15 ticks后发射，恢复4 ticks，冷却10 ticks
   - `BreezeLongJumpGoal`（优先级3）：长跳移动，吸气10 ticks后跳跃，着陆后设置射击许可
   - `BreezeShootWhenStuckGoal`（优先级4）：卡住时（水中/骑乘/飘浮）紧急射击
   - `BreezeSlideGoal`（优先级5）：地面滑行移动，内圈逃跑或中圈/目标身后移动，结束后设置射击许可
   - `shootWindCharge()` 方法由 BreezeShootGoal 调用触发
