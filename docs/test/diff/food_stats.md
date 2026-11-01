# 差分能力：`food_stats`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 77 个能力，20 种变化组合全部一致。

#### 差异：`add` 缺少下界钳制

原版 `FoodData.add`：

```java
this.foodLevel       = Mth.clamp(food + this.foodLevel, 0, 20);
this.saturationLevel = Mth.clamp(saturation + this.saturationLevel, 0.0F, (float) this.foodLevel);
```

Cubium 的 `FoodStats::addStats` 写的是 `std::min(m_foodLevel + food, MAX_FOOD_LEVEL)`——
**只钳上界，没有下界**。`food + foodLevel < 0` 时会得到负饥饿值，而原版是 0。

实测 `add|-25|0|10|3` 得到 `foodLevel=0, saturationLevel=0`，正是下界把它抬回来的结果。

#### 另一处差异：两次 clamp 的顺序

饱和度上界用的是**刚更新过的** `foodLevel`，不是旧的。先算饱和度再更新饥饿值会得到不同
结果。场景表里的 `add|-20|5|10|3` 专门压这条：饥饿值先被钳到 0，饱和度上界随之变成 0。

#### 一次被源码纠正的误判

我最初只读到 `addExhaustion` 的一段片段，据此写下「原版没有上限，Cubium 加了 `min(x, 40)`
是多余限制」。后来完整读 `FoodData.java` 才发现原版那一行就是：

```java
public void addExhaustion(float e) {
    this.exhaustionLevel = Math.min(this.exhaustionLevel + e, 40.0F);
}
```

**Cubium 在这里是对的**，是我的片段阅读出了错。探针注释已改正。

这是这一系列工作中第三次出现「只读片段就下结论」的问题（前两次分别是要塞的常量种子与
扫描步长）。**读源码必须读完整的方法体**，否则「发现差异」本身可能是幻觉。
