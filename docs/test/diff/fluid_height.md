# 差分能力：`fluid_height`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 131 个能力，16 个用例全部一致。流体高度公式此前未被覆盖
（[`fluid_spread`](./fluid_spread.md) 与 [`fluid_spread_dir`](./fluid_spread_dir.md) 覆盖的是流动
方向与扩散距离）。

## 核心发现一：分母是 9，但满值只有 8

```java
// FluidState
public static final int AMOUNT_MAX  = 9;
public static final int AMOUNT_FULL = 8;

// FlowingFluid
public float getOwnHeight(FluidState state) {
    return state.getAmount() / 9.0F;
}
```

**分母 `9` 来自 `AMOUNT_MAX`，而 `getAmount()` 的最大值是 `AMOUNT_FULL = 8`。**

| 单元 | 结果 |
|---|---|
| `ownHeight\|1` | `0.111111112` |
| `ownHeight\|2` | `0.222222224` |
| `ownHeight\|3` | `0.333333343` |
| `ownHeight\|4` | `0.444444448` |
| `ownHeight\|5` | `0.555555582` |
| `ownHeight\|6` | `0.666666687` |
| `ownHeight\|7` | `0.777777791` |
| **`ownHeight\|8`** | **`0.888888896`** |

**源头（`getAmount()` 返回 8）的高度是 `8/9 ≈ 0.8889`，不是 `1.0`。** 这就是流体渲染时
「水面比方块顶低一点点」的原因。

**只有 8 个离散值，没有 0 也没有 1。**

## 核心发现二：`float` 精度损失在末位可见

上表中 `4/9` 是 `0.444444448` 而不是 `0.444444444`，`8/9` 是 `0.888888896` 而不是 `0.888888889`。

**这些末位差异是 `float`（24 位尾数）与 `double` 的真实分野**，不是格式化误差。

**因此本能力必须用 `%.9g` 而非 `%.6g`**：

| 输入 | `%.9g` | `%.6g` |
|---|---|---|
| `1/9` | `0.111111112` | `0.111111` |
| `8/9` | `0.888888896` | `0.888889` |

若用 `%.6g`，两侧的末位差异会被**格式化截断掩盖**，导致 `float` 与 `double` 实现的差异
（`0.888888896` vs `0.888888889`）无法被发现——**假一致**。

这是本能力在格式化上的唯一特殊之处，其余浮点能力仍用 `%.6g`。

## 核心发现三：`LEVEL_FLOWING` 的范围是 `[1, 8]`，没有 0 级

```java
LEVEL_FLOWING = IntegerProperty.create("level", 1, 8);
```

**流体没有「0 级」**——最小的流动液体是 1 级（高度 `1/9`）。若 Cubium 用 `[0, 7]`，所有
非源头流体的高度都会差 `1/9`。

注意与 `LEVEL = IntegerProperty.create("level", 0, 15)` 区分——那是炼药锅、堆肥桶、蜂巢等
方块的通用 `level`，范围完全不同。

## 核心发现四：`getHeight` 的 `1.0` 是特判，不是算出来的

```java
public float getHeight(FluidState state, BlockGetter level, BlockPos pos) {
    return hasSameAbove(state, level, pos) ? 1.0F : state.getOwnHeight();
}
```

上方是同类流体时，当前格「满格」返回 `1.0`。**这个 `1.0` 不是 `getAmount() / 9.0F` 的结果**
——没有任何 `getAmount()` 能算出 1.0（最大 8/9）。

因此 `getHeight` 的取值范围是 `{1/9, 2/9, ..., 8/9, 1}`，共 **9 个值**，而 `getOwnHeight` 只有
**8 个值**。

## 本轮的重大局限：无法构造任意 level 的流体状态

我最初想让 oracle 直接调用原版的 `FluidState.getAmount()` / `getOwnHeight()`，用
`Blocks.WATER.defaultBlockState().setValue(LEVEL, n)` 构造真实状态。

**第一次运行就失败**：

```text
IllegalArgumentException: Cannot set property IntegerProperty{name=level, ..., values=[1..8]}
as it does not exist in Block{minecraft:water}
```

原因：**`LEVEL`（即 `FlowingFluid.LEVEL` = `BlockStateProperties.LEVEL_FLOWING`）挂在流体状态
上，不在方块状态上**。`Blocks.WATER` 的方块状态里根本没有 `level` 属性。

而 `FluidState` 是 `StateHolder`，**没有公开的、可指定 level 的工厂方法**——流体状态只能由世界
在实际流动中产生。

**因此本能力无法用「调用原版实现」的方式验证**，这与其它纯计算能力的处理方式不同。

### 采用的替代方案：以原版常量为基准

oracle 改用两个**公开常量**作为计算基准：

```java
case "amount"     -> amount;                                    // 回显，等价于 state.getValue(LEVEL)
case "ownHeight"  -> amount / (float) FluidState.AMOUNT_MAX;    // 分母取自原版常量
```

**分母取自 `FluidState.AMOUNT_MAX` 而非字面量 `9`**——若原版把该常量改成别的值，oracle 会立即
跟随，探针侧则会暴露差异。

这个方案的价值边界：

- **能锁定**：公式的数值结果、`AMOUNT_MAX` / `AMOUNT_FULL` 的取值、8 个离散高度的精确 `float` 表示
- **不能锁定**：`getAmount()` 本身在真实世界中的取值逻辑（源头返回 8、流动返回 LEVEL）——这部分
  只能靠读源码确认，或留待 world 型能力

**这是「原版可实现范围」的真实限制，应当在状态表中如实标注。**

## 未覆盖范围

- `getAmount()` 在真实世界中的取值（源头 8 / 流动 LEVEL）——需 world 上下文
- `getHeight` 的 `hasSameAbove` 分支——需 `BlockGetter`
- `getShape` 返回的 `VoxelShape`（依赖 `getHeight`）
- 水与岩浆的高度是否有差异（源码上完全相同，都是 `/ 9.0F`，但无法用 oracle 证实）
- `getFlow` / `getSlopeDistance`（属于流动逻辑，已被 `fluid_spread` 覆盖）