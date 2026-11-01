# 差分能力：`vec3_math`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 95 个能力，31 个向量运算全部一致。`Vec3` 的向量运算此前没有被比对过。

## 三个关键点

**1. `normalize()` 的短路阈值是 `1.0E-5F`，且是严格小于。**

```java
public Vec3 normalize() {
    double d0 = Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    return d0 < 1.0E-5F ? ZERO : new Vec3(this.x / d0, this.y / d0, this.z / d0);
}
```

注意阈值写法是 `1.0E-5F`（`float` 字面量，比较时提升为 `double`）。实测边界：

| 单元 | 长度 | 结果 |
|---|---|---|
| `9.9e-06` | 9.9e-06 | `[0,0,0]`（归零） |
| **`1e-05`** | **1.0e-05** | **`[1,0,0]`（不归零）** |
| `1.1e-05` | 1.1e-05 | `[1,0,0]`（归一化） |

**恰好等于阈值时不归零**——严格小于才归零。把 `d0 < 1.0E-5F` 写成 `d0 <= 1.0E-5F` 会在
这一点上出错。

更重要的是：这个短路的语义不是「零向量返回零」，而是**「任何长度小于 1e-5 的向量都返回
零」**。一个方向完全正确、只是极短的向量会被静默地抹掉。

**2. 归一化用除法 `x / d0`，不是倒数乘 `x * (1/d0)`。** 两者在浮点下可能差 1 ulp。探针里
用 `volatile` 固定了除法结果，避免 `-ffast-math` 把它改写成倒数乘（见
[`_patterns.md`](./_patterns.md) 第一节）。

**3. `horizontalDistance` 完全忽略 `y`。**

| 单元 | 结果 |
|---|---|
| `hdist(3,4,0)` | 3 |
| `hdist(3,100,4)` | 5 |
| `hdist(0,5,0)` | 0 |

`hdist(3,100,4)` 的 `length` 是 100.12，`horizontalDistance` 是 5——只算 XZ 平面。

## 与 Cubium 的两处口径差异

Cubium 的实现是 `src/common/util/math/Vector3.hpp`：

```cpp
[[nodiscard]] Vector3 normalized() const noexcept
{
    const T len = length();
    if (len > static_cast<T>(EPSILON)) {
        const T invLen = static_cast<T>(1) / len;
        return {x * invLen, y * invLen, z * invLen};
    }
    return zero();
}
```

| 项 | 原版 | Cubium | 影响 |
|---|---|---|---|
| 阈值 | `1.0E-5F` | `EPSILON = 1e-6f` | **差 10 倍** |
| 判据 | `d0 < 1e-5` | `len > 1e-6` | 端点 |
| 算法 | `x / len` | `x * (1/len)` | 可能差 1 ulp |

**第一条是可观测的行为差异**：长度落在 `[1e-6, 1e-5)` 的向量，原版归零、Cubium 正常归一化。
单元 `normalize|5e-06,0,0` 正是钉住这一点——原版给 `[0,0,0]`。

这个区间在实践中并不罕见：实体速度衰减到很小、碰撞法向量的归一化、AI 方向向量在接近
目标时的分量，都可能落进去。原版把这些统统视为「没有方向」，Cubium 会给出一个有效的单位
向量。

本探针按**原版口径**复算，因此这两处差异会体现为 diff。它们是待对齐项，不是豁免项——
记录在此供后续实现对齐时参照。

## 未覆盖范围

- `clip`（与 `AABB` 求交，`explosion_ray` 已覆盖一个侧面）
- `subtract` / `add` / `multiply` 的逐分量形式
- `Vec3.directionFromRotation`（角度转方向向量，涉及三角函数约定）