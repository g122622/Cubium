# 差分能力：`fluid_spread`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 108 个能力，30 个流体用例全部一致。流体扩散此前完全未被覆盖。

## 四个关键点

**1. 流体高度是 `amount / 9.0F`，源方块也不是满高。**

```java
public float getOwnHeight(FluidState state) { return state.getAmount() / 9.0F; }
public float getHeight(FluidState state, ...) {
    return hasSameAbove(state, ...) ? 1.0F : getOwnHeight(state);
}
```

| 单元 | 高度 |
|---|---|
| `water\|1` | 0.111111112（1/9） |
| `water\|8` | **0.888888896（8/9）** |
| `water\|1` 且上方有同类流体 | **1** |

**源方块等级是 8，高度 8/9 而不是 1.0。** 只有「上方有同类流体」时才是满高 1.0——因为
下方那格被上方流体覆盖，视觉上是连续水体。

**2. 下落状态的流体横向扩散等级固定为 7。**

```java
int i = state.getAmount() - getDropOff(level);
if (state.getValue(FALLING)) { i = 7; }
```

| 单元 | `amount` | `dropOff` | `falling` | 扩散等级 |
|---|---|---|---|---|
| `water\|3` | 3 | 1 | false | 2 |
| `water\|3` | 3 | 1 | **true** | **7** |
| `lava\|1` | 1 | 2 | false | **0**（不扩散） |
| `lava\|3` | 3 | 2 | false | 1 |
| `lava\|1` | 1 | 2 | **true** | **7** |

**1 级的下落岩浆横向扩散等级是 7，比 3 级的不下落岩浆（1）高得多。** `if (FALLING) { i = 7; }`
完全覆盖了 `amount - dropOff` 的计算。所以从高处落下的流体会在地面以接近满强度向四周扩展。

**3. 三种流体参数的差异。**

| 流体 | `getSlopeFindDistance` | `getDropOff` |
|---|---|---|
| 水 | 4 | 1 |
| 岩浆（主世界） | **2** | **2** |
| 岩浆（下界，fast） | **4** | **1** |

岩浆在主世界与下界的扩散行为**完全不同**——主世界只流 2 格、每格衰减 2，下界能流 4 格、
每格衰减 1。这是维度依赖的参数，不能硬编码。

**4. 坡度搜索的上限是严格小于。**

```java
if (depth < getSlopeFindDistance(level)) {
    int j = getSlopeDistance(nb, depth + 1, ...);
}
// 找到洞时：
if (isHole(nb)) { return depth; }
```

| 单元 | 洞的距离 | 结果 |
|---|---|---|
| `slope(water, depth=1, hole=0)` | 0（紧邻） | 1 |
| `slope(water, depth=1, hole=1)` | 1 | 2 |
| `slope(water, depth=1, hole=2)` | 2 | 3 |
| `slope(water, depth=1, hole=3)` | 3 | **4** |
| `slope(water, depth=1, hole=4)` | 4 | **1000（找不到）** |
| `slope(lava, depth=1, hole=2)` | 2 | **1000** |
| `slope(lava_fast, depth=1, hole=3)` | 3 | 4 |

水最多找到距离 3 的洞（深度 4），**距离 4 就找不到了**——因为 `depth < 4` 在 `depth == 4` 时
为假，递归不再继续。返回 1000 表示「无穷远」，即没有可流去的低洼。

**找到洞就立即 `return`，不再比较其他方向**——它返回的是**首次**遇到的洞的深度，不是最小深度。

## 本轮踩坑记录（三处，都是 oracle 侧实现问题）

**踩坑 1：`Fluids.WATER` 的默认状态没有 `level` 属性。**

`Fluids.WATER` 返回的是 **Source** 内部类实例，`getAmount()` 恒为 8，状态上只有 `falling`
属性。`setValue(LEVEL, n)` 找不到属性，静默失败——所有单元都返回 8/9。

正解是用 **`Fluids.FLOWING_WATER`**（Flowing 内部类，`getAmount()` 读 `LEVEL`）。

**踩坑 2：不能 `new WaterFluid.Flowing()`。**

`Fluid` 的构造器会调用 `createIntrusiveHolder`，而注册表在启动后已冻结，直接抛
`IllegalStateException: This registry can't create intrusive holders`。必须从注册表取已有实例。

**踩坑 3：`LEVEL` 属性上限是 8，测试用例写了 9。**

`water|9` 在 oracle 侧被钳到 8 得 8/9，探针未钳制算出 9/9 = 1。这说明**探针也要复刻
`LEVEL` 属性的取值范围**，不能假设输入合法。

## 未覆盖范围

- `getSpread` 的方向选择（「就近全保留」语义，需多方向网格模型）
- `spreadTo` 的写入顺序与方块交互
- `canPassThroughWall` 的方块形状判定
- 流体与岩浆的相遇反应（`LavaFluid` 的 `fizz` 逻辑）