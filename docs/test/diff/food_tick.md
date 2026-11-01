# 差分能力：`food_tick`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 78 个能力，21 种「状态 × 难度」组合全部一致。上一轮的 `food_stats` 覆盖了
`add`/`eat`/`addExhaustion`，本能力补上 `FoodData.tick` 的状态机。

#### 分支结构

```
1) exhaustionLevel > 4.0F  ->  -=4.0F，然后扣饱和（有）或扣饥饿（无饱和且非 PEACEFUL）
2) regen && sat>0 && hurt && food>=20  -> 每 10 tick 回 min(sat,6)/6 点，加等量 exhaustion
3) regen && food>=18 && hurt           -> 每 80 tick 回 1 点，加 6 exhaustion
4) food<=0                             -> 每 80 tick 判定一次是否造成 1 点饥饿伤害
5) 以上都不满足                        -> tickTimer 归零
```

三个容易写错的地方：

**1. exhaustion 消耗在所有分支之前，且只判定一次。** 它不是 `while` 循环——即使
`exhaustionLevel` 是 100，一次 tick 也只减 4。

**2. 饥饿伤害是否真的扣血取决于生命值与难度的组合**：

```java
health > 10.0F || difficulty == HARD || (health > 1.0F && difficulty == NORMAL)
```

实测对照：生命 1 + NORMAL 得 `damage=0`，生命 1 + HARD 得 `damage=1`。这个组合条件
写成「生命低于某值就扣血」会全错。

**3. 默认分支会把 tickTimer 归零。** 条件不满足时计时器**不保留**——这意味着再生
条件一旦中断，之前的累积就白费。实测 `15|2|0|50` 输入得到 `tickTimer=0`。

#### 一个跨分支的时序陷阱

快再生的 `addExhaustion(min(sat, 6))` 会立刻抬高 exhaustion，**下一次 tick** 就可能
触发最上面的消耗分支，从而把饱和扣掉。也就是说「回血」与「扣饱和」在连续两个 tick
里是联动的。场景表最后一条（`20|8|3|9`）就是压这种同一 tick 内多分支叠加的情形。

#### 为什么用复算而不是直接调用原版

`FoodData.tick` 需要真实的 `ServerPlayer`（读难度、游戏规则、受伤状态、生命值，并调用
`heal`/`hurtServer`），而 `exhaustionLevel` 只有私有字段没有公开写访问器。因此 oracle
按原版方法体逐行复算，把玩家侧的可变因素作为输入显式传入。
