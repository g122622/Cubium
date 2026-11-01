# 差分能力：`aabb_ops`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 94 个能力，19 个几何运算全部一致。Cubium 目前**没有 `AABB` 类**（只有
`AABBShapeComponent` 数据组件，无几何运算）——本能力提供的是待实现的行为基准。

#### 三个关键点

**1. `intersects` 用严格不等号，接触不算相交。**

```java
return minX < other.maxX && maxX > other.minX && ...;
```

| 单元 | 结果 |
|---|---|
| `1,1,1,2,2,2` vs `0,0,0,1,1,1`（恰好接触） | **false** |
| `1,0.2,0.2,2,0.8,0.8` vs `0,0,0,1,1,1`（仅 X 轴接触） | **false** |
| `1,1,1,3,3,3` vs `0,0,0,2,2,2`（部分重叠） | **true** |

用 `<=`/`>=` 会让刷怪、碰撞、漏斗判定全部放宽一格。

**2. `expandTowards` 与 `inflate` 语义相反。**

| 单元 | 结果 |
|---|---|
| `inflate(1,1,1)` | `[-1,-1,-1,2,2,2]`（四边各扩） |
| `expand(1,0,0)` | `[0,0,0,2,1,1]`（只动 maxX） |
| `expand(-1,0,0)` | `[-1,0,0,1,1,1]`（只动 minX） |
| `expand(0,0,0)` | `[0,0,0,1,1,1]`（完全不动） |

**3. `minmax` 是并集，不是交集。** 名字容易反——`intersect` 取交集（`max` 下限、
`min` 上限），`minmax` 取并集（`min` 下限、`max` 上限）。

#### 构造器归一化：本轮真实踩到的坑

我第一版让 `intersect` 在盒子完全分离时原样返回 `min=5, max=1`，但原版返回
`[1,1,1,5,5,5]`。原因是原版构造器：

```java
public AABB(double x1, double y1, double z1, double x2, double y2, double z2) {
    this.minX = Math.min(x1, x2);
    this.maxX = Math.max(x1, x2);
    ...
}
```

**构造器会归一化。** 所以任何经过 `new AABB(...)` 的返回值都被归一化——包括
`intersect` 产生的「负尺寸」结果，构造后会被翻成「正尺寸」。

这意味着「空盒」在原版里**不可能表示成 `min > max`**：分离盒子求交集得到的仍是
一个 `min < max` 的盒子，只是它恰好与两个输入都不相交。如果按 `min > max` 判空，就会
与其它 `intersects` 调用者的行为不一致。

修复方式：在探针里加 `makeBox(...)` 工厂复刻构造器归一化，并让**所有**返回 `Box` 的
函数与 `parseBox` 都走它——`inflate`、`expandTowards`、`move`、`intersect`、`minmax`
无一例外。

#### 一处格式问题（本轮修复）

双盒子单元的格式是 `<box>|<box2>|<op>`，模式在**最后**一段。我最初把判断写成了
`parts[1] == "intersect"`，实际上 `parts[1]` 是第二个盒子，导致所有双盒子单元都走进了
`throw`。探针与 oracle 两侧都写错，所以第一轮跑出来是「oracle 运行失败」而不是差异——
错误路径也会被 diff 框架抓到。

#### 验证方法

延续前几轮的做法，跑完逐条打印实际输出核对分支，19 条全部命中。

#### 未覆盖范围

`getCenter`、`getSize`、`clip`（射线求交，`explosion_ray` 已覆盖一个侧面）、
`AABB.of(BlockPos)`、`AABB.encapsulatingFullBlocks`。
