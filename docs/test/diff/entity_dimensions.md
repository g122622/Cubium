# 差分能力：`entity_dimensions`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 75 个能力，12 种「尺寸 × 缩放 × 位置」组合全部一致。Cubium 目前**没有**
`EntityDimensions` 也没有 `EntityAttachments`，所以探针自带一份实现——本能力覆盖的是
**公式**（眼睛高度比例、缩放规则、包围盒展开），而不是「实体的宽高表是否正确」。

#### 三条容易写错的公式

**1. 眼睛高度只按高度缩放。** 原版：

```java
public EntityDimensions scale(float w, float h) {
    return new EntityDimensions(this.width * w, this.height * h,
                                this.eyeHeight * h, ...);   // <- 乘 h，不是 w
}
```

写成 `eyeHeight * w` 在宽高比不等于 1 的实体上立刻出错。实测验证：

| 输入 | `scaledEyeHeight` |
|---|---|
| `0.6 / 1.8` 只缩宽 0.5 | **1.53**（不变） |
| `0.6 / 1.8` 只缩高 0.5 | **0.765**（减半） |

**2. 包围盒以中心为基准，不是脚底某角。** X/Z 是 `x ± width/2`，Y 是 `[y, y + height]`
（脚底在 y）。宽扁体 `4 × 0.5` 的实测结果是 `bboxMinX=-2, bboxMaxX=2`——半径是
`width/2 = 2`，若误用 `width` 会得到 ±4。

**3. 默认眼睛高度 = `height * 0.85F`**，在构造器里算好，`scale` 时再按高度乘一次。

#### 一处精度细节

`f = width / 2.0F` 是 **float**，而位置 `x` 是 **double**。`x - f` 里 `f` 先提升到 double
再相减。C++ 侧必须保持同样的提升顺序，否则在大坐标（如 `-1234.5`）下会差 ulp。
场景表里专门放了两个大坐标用例来压这一点。
