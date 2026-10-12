# 远程攻击目标

## 目录结构树

```text
attack/
├── RangedAttackGoals.hpp # 通用远程、弓箭和弩攻击目标声明
├── RangedAttackGoals.cpp # 目标追踪、导航、蓄力与射击实现
└── README.md            # 模块关系与生命周期约束
```

## 内部模块关系

`RangedBowAttackGoal` 继承 `RangedAttackGoal`，复用目标缓存与继续执行判断。`RangedCrossbowAttackGoal` 独立维护蓄力和射击状态。

## 上下游外部依赖关系

生物注册目标后由 `GoalSelector` 调度。本目录依赖 `MobEntity`、`EntityReference`、导航器、注视控制器和远程攻击接口。

## 容易踩的坑

- 跨 tick 目标使用 `EntityReference`；实体移除或析构后引用返回空值，不能用裸指针的存活检查代替引用失效机制。
- 继续追踪导航路径的前提是缓存目标有效且存活；目标失效后必须允许选择器重置导航。
- 生命周期标记只反映移除和析构，生命值归零但尚未移除时仍须检查 `isAlive()`。
