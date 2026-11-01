# 差分能力：`furnace_tick`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 86 个能力，12 种状态组合全部一致。熔炉每 tick 的状态机此完全没有被比对过。

#### 状态机骨架

```java
boolean wasLit = isLit();
if (isLit()) { litTimeRemaining--; }

if (isLit() || hasFuel && hasInput) {
    if (!isLit() && canBurn(...)) {          // 2a 点燃
        litTimeRemaining = getBurnDuration(fuel);
        litTotalTime = litTimeRemaining;
    }
    if (isLit() && canBurn(...)) {           // 2b 推进烹饪
        cookingTimer++;
        if (cookingTimer == cookingTotalTime) { cookingTimer = 0; 产出; }
    } else {
        cookingTimer = 0;                     // ← 块内：直接归零
    }
} else if (!isLit() && cookingTimer > 0) {
    cookingTimer = Mth.clamp(cookingTimer - 2, 0, cookingTotalTime);   // ← 块外：减 2
}
```

#### 四个易错点

**1. 两条退进度路径的处理不同**，这是本轮最重要的发现：

| 情形 | 走哪条 | `cookingTimer` |
|---|---|---|
| 燃烧中但条件不满足 | 块内 `else` | **归零** |
| 完全熄火 | 块外 `else if` | **减 2** |

实测 `0|0|10|200|...` 得到 `cookingTimer=8`（减 2），而 `100|200|5|200|false|...` 得到 `cookingTimer=0`（归零）。把两处合并成一处，熄火后的进度衰减会完全错掉——**减 2 意味着熄火后进度是缓慢回退，不是立刻清零**。

**2. 点燃与烹饪在同一 tick 内可连续发生。** 两个 `if` 是顺序关系而非互斥。实测刚点燃的单元得到 `litTimeRemaining=200` 且 `cookingTimer=1`——点燃当 tick 就开始计进度。

**3. 熄灭判定在读减之后。** `wasLit` 在 tick 开头记录，比较发生在末尾，用来决定方块状态是否更新。

**4. `cookingTimer == cookingTotalTime` 是相等判断**，不是 `>=`。

#### 又一次「用例写错导致分支没被覆盖」

我设计单元表时给「点燃」那条写的是 `0|0|0|200|true|true|false|200`，注释写「有燃料有原料」——但字段顺序是 `canBurn|hasInput|hasFuel`，第七位 `false` 意味着 **`hasFuel=false`**，根本不会点燃。

这条单元跑出来 `litTimeRemaining=0`、`stateChanged=false`，看起来「通过」了，实际上**点燃分支一次都没被执行**。是我核对输出时发现注释与结果对不上，才回头查出字段位置写错。

修正为 `...|true|true|true|200` 后，得到 `litTimeRemaining=200, cookingTimer=1, stateChanged=true`，点燃分支才真正被覆盖。

**这是连续第二轮遇到同一类问题**（上轮是 `99|50|99|50` 被钳到 0，这轮是字段位写错）。两次都是「用例看起来合理、实际没打到目标分支」。教训要再收紧一层：**单元表写完后必须逐条核对「这条单元预期走哪个分支」，而不是写完就算。**
