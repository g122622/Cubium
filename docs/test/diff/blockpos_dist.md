# 差分能力：`blockpos_dist`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 104 个能力，26 个距离/迭代用例全部一致。补齐 [`blockpos_codec`](./blockpos_codec.md)
未覆盖的距离与包围盒迭代部分。

## 四个关键点

**1. `betweenClosed` 的迭代顺序是 X 最内层、Z 最外层。**

```java
int i = maxX - minX + 1;        // X 方向长度
int j = maxY - minY + 1;        // Y 方向长度
// computeNext:
int i1 = index % i;             // X 偏移
int j1 = index / i;
int k1 = j1 % j;                // Y 偏移
int l1 = j1 / j;                // Z 偏移
```

实测 `box(0,0,0 → 2,1,0)`：

```
(0,0,0) (1,0,0) (2,0,0) (0,1,0) (1,1,0) (2,1,0)
```

**X 变化最快**。写成 Z 最内层会让所有依赖遍历顺序的逻辑（方块更新传播次序、随机数消耗
次序）与原版错位。

**2. 角点会自动排序，迭代仍从 min 开始。**

`betweenClosed` 内部先取 `Math.min` / `Math.max`：

```java
box(2,1,0 → 0,0,0)  ==  box(0,0,0 → 2,1,0)
```

两者输出逐项相同。**传入反序不会让迭代反向**。

**3. `distManhattan` 用 `float` 累加，`distSqr` 用 `double`。**

```java
public int distManhattan(Vec3i o) {
    float f  = Math.abs(o.getX() - this.getX());
    float f1 = Math.abs(o.getY() - this.getY());
    float f2 = Math.abs(o.getZ() - this.getZ());
    return (int)(f + f1 + f2);          // float 累加后转 int
}

public double distToLowCornerSqr(double x, double y, double z) {
    double d0 = this.getX() - x;
    ...
    return d0 * d0 + d1 * d1 + d2 * d2;
}
```

`float` 只有 24 位有效位。坐标差之和超过 `2^24`（约 1680 万）时低位会丢失。实测：

| 单元 | 结果 | 说明 |
|---|---|---|
| `manhattan(0 → 16777216)` | `16777216` | 恰为 2^24，仍精确 |
| `manhattan(0 → 20000000)` | `20000000` | 超过 2^24 但该值本身可精确表示 |
| `sqr(0 → 10000000)` | `1e+14` | 双精度精确 |

想构造出 `float` 精度损失需要**三轴之和落在 2^24 附近且不能被 float 精确表示**的值
（例如 `16777217`）。`manhattan` 的参数是 `int`，单轴最大约 2^31，因此实际游戏中
（坐标范围 ±33554432）确实可能触到精度边界。

用 `int` 直接累加会在这些值上与 `float` 版不同。

**4. 包围盒体积用 `long` 计算。**

`boxsize(-5,-5,-5 → 5,5,5)` = 11³ = **1331**。大盒子（如 32×384×32 的整区块柱）会超过
`int` 的安全范围，必须用 64 位。

## 一处跨语言格式化缺陷（本轮修复）

首次运行出现一处差异，但**数值完全相同**：

```
oracle: {"value":"1.00000000e+14"}
probe : {"value":"1e+14"}
```

原因是 `%.9g` 在**指数形式**下会保留末尾零，而两侧的 `formatNumber` 只处理了非指数形式
的零裁剪。修正方式是先按 `e`/`E` 拆出尾数与指数，只裁剪尾数部分的零：

```cpp
const size_t expPos = text.find_first_of("eE");
std::string mantissa = expPos == std::string::npos ? text : text.substr(0, expPos);
const std::string suffix = expPos == std::string::npos ? std::string() : text.substr(expPos);
// 只裁剪 mantissa，再拼回 suffix
```

**这类差异与实现无关，纯粹是表示层的**。但它会让测试显示"差异"，掩盖真实的数值问题。
后续新增涉及大数值的能力时，`formatNumber` 必须同时处理两种形式。

## 未覆盖范围

- `spiralAround`（螺旋迭代，逻辑复杂需单独能力）
- `randomBetweenClosed`（依赖随机源）
- `betweenClosed(AABB)`（依赖 `Mth.floor`，已有 [`mth_rounding`](./mth_rounding.md) 覆盖）
- `distChessboard`（`BlockPos` 上未找到该方法，可能在别的类）