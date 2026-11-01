# 差分能力：`fluid_spread_dir`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 109 个能力，29 个方向选择用例全部一致。补上 [`fluid_spread`](./fluid_spread.md) 明确
留下的 `getSpread` 缺口。

## 核心发现：`best = dist` 是无条件赋值，导致方向选择不对称

```java
int best = 1000;
for (Direction dir : Direction.Plane.HORIZONTAL) {
    int dist = ...;
    if (dist < best) { map.clear(); }           // 更近 -> 清空
    if (dist <= best) { map.put(dir, state); }  // 不更远 -> 加入
    best = dist;                                 // 无条件赋值！
}
```

`best = dist` 写在两个 `if` **外面**，所以**即使 `dist` 比当前 `best` 大，`best` 也会被抬高**。

### 实测证据：`water|2,2,7,7` → `[north,east,west]`

| 步骤 | 方向 | 距离 | `dist < best` | `dist <= best` | 动作 | `best` |
|---|---|---|---|---|---|---|
| 1 | north | 2 | `2 < 1000` ✓ | ✓ | 清空 + 加入 north | **2** |
| 2 | east | 2 | `2 < 2` ✗ | ✓ | 加入 east | 2 |
| 3 | south | 7 | `7 < 2` ✗ | `7 <= 2` ✗ | 跳过 | **7**（被抬高） |
| 4 | west | 7 | `7 < 7` ✗ | `7 <= 7` **✓** | **加入 west** | 7 |

**`west` 距离是 7（远），却被保留了；`south` 距离也是 7，却被排除了。** 两者距离完全相同，
结果却不同——只因为 `south` 恰好是那个把 `best` 抬高的方向。

如果 `best` 只在更近时才更新（`best = min(best, dist)`），结果会是 `[north,east]`。原版不是
这样，所以**流体扩散的方向选择是不对称的**：同一组距离，换一下遍历顺序就可能得到不同结果。

### 其他实测

| 单元 | 结果 | 说明 |
|---|---|---|
| `water\|1,5,5,5` | **`[north,south,west]`** | north(1) 后 best=1；east(5) 把 best 抬到 5；south、west 因 `5<=5` 被加入 |
| `water\|5,1,1,1` | **`[east,south,west]`** | east(1) 清空后 best=1；south、west 因 `1<=1` 加入 |
| `water\|1,9,1,9` | **`[south]`** | east(9) 抬高 best=9 → south(1) 清空；west(9) 因 `9>1` 被排除（此时 best 已被 south 重置为 1） |
| `water\|0,0,0,0` | `[north,east,south,west]` | 全 0，全部保留 |
| `water\|1000,1000,1000,1000` | `[north,east,south,west]` | 全 1000，`1000 <= 1000` 全部通过 |

`water|1,9,1,9` 只保留 `south` 尤其反直觉：north 和 south 距离都是 1（并列最小），但因为
east 在中间把 `best` 抬到 9，south 触发了清空，随后 `best` 被重置为 1，west(9) 反而被排除。
**最终只留下一个方向，而它并不是唯一的最小值方向。**

## 方向遍历顺序

```java
HORIZONTAL(new Direction[]{Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST}, ...)
```

**`Plane.HORIZONTAL` 的顺序是 `NORTH, EAST, SOUTH, WEST`**，不是 `Direction` 的枚举声明顺序
（`DOWN, UP, NORTH, SOUTH, WEST, EAST`）。

由于上面的非对称行为，这个顺序**直接决定了保留哪些方向**。若按枚举声明顺序遍历（`N, S, W, E`），
`water|1,5,5,5` 会得到不同结果。

## 未覆盖范围

- `getSlopeDistance` 的二维网格递归（需要完整地形）
- `SpreadContext` 的洞缓存
- `canMaybePassThrough` 的方块形状判定
- `map` 的实际迭代顺序（`EnumMap` 按枚举顺序，与 `HORIZONTAL` 顺序不同）