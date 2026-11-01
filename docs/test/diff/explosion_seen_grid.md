# 差分能力：`explosion_seen_grid`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 74 个能力，10 种碰撞箱全部一致。它是上一个能力 TODO 的直接延伸：`getSeenPercent`
的**采样网格生成**是纯计算，只有射线检测那一半需要世界。

#### 网格本身值得单独钉住的理由

采样点数 = `floor(1 / step) + 1`，这个「+1」在 `f` 的浮点累加误差下可能少一次或多一次。
Cubium 的 `Explosion::_getBlockDensity` 用 `f32` 做这一步，原版用 `double`——同样的碰撞箱
会得到不同点数，可见度分母随之改变，最终伤害也跟着变。

#### 一个花了不少时间的坑

网格逻辑照抄无误，但 `offsetX` 上持续差 `2.77556e-17`：

```
oracle: {"offsetX":"0",    "firstSample":[["0",...]]}
probe : {"offsetX":"2.77556e-17","firstSample":[["2.77556e-17",...]]}
```

`offsetX` 来自 `(1.0 - floor(1/step) * step) / 2.0`。只影响 `step` 恰为 1/3、1/6 的两个
箱子（完整方块与大尺寸）。我单独写了一个小程序验证 `1.0/3.0` 的往返，得到精确 `0`——
说明表达式本身没问题，是**编译环境**的差异。

根因是项目根 CMakeLists 全局开了 `-ffast-math`（`-fno-finite-math-only` 保留 NaN 语义，
但重结合仍被允许）。它可以把 `floor(1/step) * step` 折叠成 `(1/step) * step`，后者不保证
等于 1。

试过两条路都失败：

- `#pragma float_control(precise, on, push/pop)`——`push/pop` 只能出现在文件或命名空间作用域；
- 换成函数内 `on`——clang 在 `-ffast-math` 下对该 pragma 不生效。

最终改法是把判据显式写出来：

```cpp
const f64 covered = gridCount * step;
const f64 remainder = std::abs(covered - 1.0) < 1e-12 ? 0.0 : (1.0 - covered);
```

这与原版语义一致（能整除时余数就是 0），且不依赖编译器的浮点策略。

#### 教训

**「表达式与 Java 完全一致」不足以保证结果一致**，编译选项会改变求值结果。差分测试的价值
正在于此——它不需要我预先知道哪里有陷阱，只要两侧用同一份输入跑，差异自己会浮出来。
