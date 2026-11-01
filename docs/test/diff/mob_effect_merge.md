# 差分能力：`mob_effect_merge`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 76 个能力，16 种叠加组合全部一致。Cubium 的 `EffectInstance` **没有 `update()`**，
本能力因此直接按原版公式计算，输出的是待实现的行为基准。

#### 三层分支里最容易漏的两处

原版 `MobEffectInstance.update()` 的分支耦合度很高，本轮实测踩到其中一个：

**1. 等级更高时无条件采用新时长**，哪怕新的更短——此时旧效果被压进 `hiddenEffect`，
等新效果过期后重新生效。

| 输入 | 结果 | hiddenEffect |
|---|---|---|
| `1|300` ← `2|100` | `amplifier=2, duration=100` | `1|300` |
| `2|300` ← `1|500` | 不变（`changed=false`） | `1|500` |

第二行是反向情形：等级更低时**只把新值放进 hiddenEffect**，当前值完全不动。

**2. ambient 更新时也会置 `flag = true`**——这是本轮发现的一处真实偏差。

原版这段的末尾有一句 `flag = true;`：

```java
if (!incoming.ambient && this.ambient || flag) {
    this.ambient = incoming.ambient;
    flag = true;          // <- 容易看漏
}
```

我第一版按「ambient 只影响 ambient 字段」实现，结果 `1|200|true` ← `1|100|false`
（等级相同时长更短、只有 ambient 变）被判成 `changed=false`，而原版是 `true`。
差分比对直接把这行漏掉的分支指出来了。

#### 一处 Java 侧的技术处理

`hiddenEffect` 是包内可见的私有字段，没有公开访问器，oracle 里用反射读取。不读它就无法
验证上面第一条「降级回退」——而这恰恰是最容易写错的一支。
