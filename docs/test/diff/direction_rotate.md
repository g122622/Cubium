# 差分能力：`direction_rotate`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 93 个能力，43 个方向/角度输入全部一致。`Direction` 的旋转与 Y 轴角度换算此前没有被
比对过。Cubium 的 `util/Direction.hpp` 有 `getAxis`，但**没有** `getClockWise` /
`toYRot` / `fromYRot`——本能力提供的是待实现的行为基准。

#### 三个关键点

**1. 轴向旋转对「垂直于该轴的方向」返回自身，不抛异常。**

```java
case X -> this != WEST && this != EAST ? this.getClockWiseX() : this;
case Y -> this != UP && this != DOWN ? this.getClockWise() : this;
case Z -> this != NORTH && this != SOUTH ? this.getClockWiseZ() : this;
```

| 单元 | 结果 |
|---|---|
| `cwaxis\|east\|x` | **east**（返回自身） |
| `cwaxis\|up\|y` | **up**（返回自身） |
| `cwaxis\|up\|x` | north（正常旋转） |
| `cw\|up` | **ERROR**（无轴向版本对垂直方向抛异常） |

那层守卫是**必需的**，不是保险——绕 X 轴旋转不改变沿 X 轴的朝向。若直接调
`getClockWiseX()` 会在 `EAST`/`WEST` 上抛 `IllegalStateException`。

**2. `toYRot()` 对垂直方向不报错，返回 270。** `toYRot` 是 `(data2d & 3) * 90`，
垂直方向没有 2D 语义但仍然有 `data2d` 字段值——实测 `UP`/`DOWN` 都是 `270`。

这一点我最初写错了：探针里让它对垂直方向报错，而原版返回 `270`。

**3. `fromYRot` 用 `floor(x + 0.5)` 而非普通四舍五入**，然后 `& 3`。水平方向的
`data2d` 是 `SOUTH=0, WEST=1, NORTH=2, EAST=3`——**南为 0 度**，不是北。

#### 一处 `-ffast-math` 陷阱（本轮真实踩到）

`fromyrot|-45` 一开始算错：

```
gcc 默认:  -45.0 / 90.0 + 0.5 = 0.0        -> floor(0) = 0      -> SOUTH
本构建下:  -45.0 / 90.0 + 0.5 = -1.9e-17   -> floor(-1.9e-17) = -1 -> & 3 = 3 -> EAST
```

原因是构建参数里有 `-ffast-math -march=native -mavx2`：除法被改写成倒数乘法，
`-45 * (1/90)` 得到 `-0.50000000000000001908`，加 `0.5` 后是 `-1.9e-17`，**不是 0**。
Java 侧是精确的 `0`，于是 `Mth.floor` 给出 `0` 还是 `-1` 就分叉了。

我逐个排除了怀疑对象（`std::stod`、`& 3`、`mthFloor` 实现、单纯的 `-ffast-math`），
最后用探针里临时打印 `q` 的实际值定位到根因，再用**完全相同的编译参数**独立复现确认。

修复方式：把除法结果强制落成一个独立的 `volatile f64`，阻止编译器跨过它做重排。

```cpp
volatile f64 quotient = deg / 90.0;
const f64 shifted = quotient + 0.5;
const i32 idx = mthFloor(shifted) & 3;
```

这与 `explosion_ray` 那轮的 `-ffast-math` 折叠问题**同类**：**凡是「先算一个浮点中间值、
再与某个精确值比较」的逻辑，在 `-ffast-math` 下都必须固定中间值**，否则比较结果可能在
编译期被改写。这条要作为新增浮点能力时的固定检查项。

#### 验证方法

延续前几轮的做法，跑完逐条打印实际输出核对分支，43 条全部命中。

#### 未覆盖范围

`Direction.Axis.choose`、`stepX/Y/Z` 向量分量（简单表）、以及 `Direction.Plane` 的
`HORIZONTAL`/`VERTICAL` 判定。
