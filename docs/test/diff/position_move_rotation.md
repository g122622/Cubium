# 差分能力：`position_move_rotation`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 155 个能力，12 个用例全部一致。这是 [`relative_mask`](./relative_mask.md) 的延续——
`calculateAbsolute` 是 `Relative` 集合的**实际应用场所**，负责把 `/tp` 命令的相对/绝对参数
合成为最终的传送目标。

## 核心发现一：含该位是相对，不含是绝对（`p1` 被丢弃）

```java
double d0 = relatives.contains(Relative.X) ? p1.position.x : 0.0;
Vec3 pos = new Vec3(d0 + p2.position.x, ...);
```

| 单元 | `p1` 位置 | `p2` 位置 | `relatives` | 结果位置 |
|---|---|---|---|---|
| `-` | `0,0,0` | `1,2,3` | 空 | **`1,2,3`** |
| `x,y,z` | `1,2,3` | `10,20,30` | 全 | **`11,22,33`** |
| **`x`** | `5,0,0` | `10,20,30` | 仅 X | **`15,20,30`** |

**第三个是最能说明语义的**：只有 X 是相对的（`5 + 10 = 15`），Y/Z 完全取 `p2`（`20`、`30`）。

若实现成「不含该位时结果取 `p1`」或「不含时抛异常」，都会与原版不同。

## 核心发现二：只有 `xRot` 被钳到 `[-90, 90]`

```java
float f2 = f + p2.yRot;                             // yRot：不钳
float f3 = Mth.clamp(f1 + p2.xRot, -90.0F, 90.0F);  // xRot：钳
```

| 单元 | `p1` 角度 | `p2` 角度 | 和 | 结果 |
|---|---|---|---|---|
| **`x_rot`** | `70` | `40` | `110` | **`90`** |
| **`x_rot`** | `-70` | `-40` | `-110` | **`-90`** |
| **`y_rot`** | `170` | `40` | `210` | **`210`**（不钳）|
| `y_rot` | `45` | `10` | `55` | `55` |
| `-` | `45` | `10` | — | `10`（绝对，丢弃 p1）|

**物理依据**：`xRot` 是**俯仰角**（pitch），向上/向下最多 ±90°；`yRot` 是**偏航角**（yaw），
可以绕圈无限累加。

**注意**：钳制发生在 `xRotBase + p2.xRot` 上，而不是 `p1.xRot` 本身。且 `xRotBase` 在
`relatives` 不含 `x_rot` 时是 `0`——此时钳制几乎不会触发。

## 核心发现三：`ROTATE_DELTA` 只影响速度向量，角度取 `p1 - 新值`

```java
if (relatives.contains(Relative.ROTATE_DELTA)) {
    float f4 = p1.yRot - yRot;          // 注意：p1 减新值
    float f5 = p1.xRot - xRot;
    delta = delta.xRot((float) Math.toRadians(f5));   // 先 xRot
    delta = delta.yRot((float) Math.toRadians(f4));   // 再 yRot
}
```

**顺序是「先 `xRot` 再 `yRot`」**，反过来会得到不同的速度方向（三维旋转不可交换）。

角度的符号是 `p1 - 新值`（**负的差值**），若写成 `新值 - p1` 会旋转反方向。

`(float) Math.toRadians(deg)` 先以 `double` 计算（`deg / 180.0 * PI`）再降为 `float`。

| 单元 | 说明 |
|---|---|
| `rotate_delta` | `p1.delta = (1,0,0)`，`p1.yRot = 30`，新 `yRot = 0` → 按 `-30°` 绕 Y 旋转 |

### `Vec3.xRot` / `Vec3.yRot` 用查表三角函数

```java
public Vec3 xRot(float radians) {
    float c = Mth.cos(radians);
    float s = Mth.sin(radians);
    double d0 = this.x;
    double d1 = this.y * c + this.z * s;
    double d2 = this.z * c - this.y * s;
    return new Vec3(d0, d1, d2);
}
```

**`Mth.cos` / `Mth.sin` 不是 `Math.cos`/`Math.sin`**——它们是 65536 项查找表（已由
[`mth_sin`](./mth_sin.md) 覆盖）。探针复用同样的查表逻辑。

## 核心发现四：速度的三轴逻辑与位置完全一致

```java
private static double calculateDelta(double a, double b, Set<Relative> r, Relative which) {
    return r.contains(which) ? a + b : b;
}
```

| 单元 | `p1.delta` | `p2.delta` | 结果 |
|---|---|---|---|
| **`delta_x,delta_y,delta_z`** | `1,2,3` | `10,20,30` | **`11,22,33`** |
| `-` | `1,2,3` | `10,20,30` | **`10,20,30`**（绝对）|

但注意 `a` 是**旋转后的** `delta`（若 `ROTATE_DELTA` 生效），所以两个标志会叠加。

## 其余实测

### 空集合：结果完全等于 `p2`

| 单元 | 结果 |
|---|---|
| `-`（p1 全 0，p2 位置 `1,2,3`，delta `0.5,0.25,0.125`，角度 `10,20`） | **`1,2,3\|0.5,0.25,0.125\|10\|20`** |

`p1` 被完全丢弃——这是「绝对传送」的语义。

## 本轮踩坑一：局部变量遮蔽了同名函数

```cpp
void xRot(f64&, f64&, f64&, f32);       // 函数

const f32 xRot = clampF(...);            // 局部变量，同作用域

applyXRot(deltaX, ...);                  // 若函数名也是 xRot，调用会被遮蔽
```

编译报错：

```
error: called object type 'f32' (aka 'float') is not a function or function pointer
```

修复：把辅助函数改名为 `applyXRot` / `applyYRot`，避免与局部变量 `xRot` / `yRot` 冲突。

**这是 C++ 相对 Java 的额外约束**——Java 的方法名与变量名不在同一命名空间。

## 本轮踩坑二：用例未真正覆盖钳制分支

我最初把钳制用例写成 `relatives = -`（不含 `x_rot`）：

```
"0|0|0|0|0|0|0|70|0|0|0|0|0|0|0|40|-"
```

结果 `xRot` 是 `40`（只用 `p2`），**`p1` 的 `70` 被丢弃，钳制根本不触发**。

**这是「用例看似覆盖实为无效」的典型**：单元通过，但验证的是另一条分支。

修正为 `relatives = x_rot` 后：

```
"0|0|0|0|0|0|0|70|0|0|0|0|0|0|0|40|x_rot"   -> 90   （钳制生效）
```

**教训**：每个用例写完后，必须**打印实际输出并对照预期分支**，不能只看「通过」。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`relative_mask`](./relative_mask.md) | `Relative` 的位掩码（本能力消费它）|
| [`portal_relative`](./portal_relative.md) | 传送门的相对位置 |
| [`mth_sin`](./mth_sin.md) | 查表三角函数（`Vec3.xRot/yRot` 依赖）|
| [`mth_clamp`](./mth_clamp.md) | `Mth.clamp` |
| [`vec3_math`](./vec3_math.md) | `Vec3` 的基础运算 |

## 未覆盖范围

- `PositionMoveRotation.of(Entity)` / `of(TeleportTransition)`（需要实体/传送状态）
- `PositionMoveRotation.STREAM_CODEC`（网络编解码）
- `Entity.move` / `rotate` 中应用该结果的完整流程（world 型）
- `TeleportTransition` 系列（需要 `ServerLevel`）