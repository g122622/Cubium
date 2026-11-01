# 差分能力：`shapes_boolean`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 107 个能力，18 个布尔组合用例全部一致。这是上一轮 [`collide_sweep`](./collide_sweep.md)
明确留下的「多盒形状」缺口的前置部分。

## 核心发现：16 种 `BooleanOp` 只有 8 种能用于 `Shapes.join`

`Shapes.joinUnoptimized` 开头有一条断言：

```java
public static VoxelShape joinUnoptimized(VoxelShape a, VoxelShape b, BooleanOp op) {
    if (op.apply(false, false)) {
        throw new IllegalArgumentException();
    }
    ...
}
```

含义是「**两个空形状的组合必须仍是空**」。凡是 `apply(false, false) == true` 的运算，无论
`a`、`b` 是什么形状，都会立即抛 `IllegalArgumentException`。

逐项计算 `apply(false, false)`：

| 运算 | 公式 | `(false,false)` | 可用于 join |
|---|---|---|---|
| `FALSE` | `false` | false | ✓ |
| `NOT_OR` | `!a && !b` | **true** | ✗ |
| `ONLY_SECOND` | `b && !a` | false | ✓ |
| `NOT_FIRST` | `!a` | **true** | ✗ |
| `ONLY_FIRST` | `a && !b` | false | ✓ |
| `NOT_SECOND` | `!b` | **true** | ✗ |
| `NOT_SAME` | `a != b` | false | ✓ |
| `NOT_AND` | `!a \|\| !b` | **true** | ✗ |
| `AND` | `a && b` | false | ✓ |
| `SAME` | `a == b` | **true** | ✗ |
| `SECOND` | `b` | false | ✓ |
| `CAUSES` | `!a \|\| b` | **true** | ✗ |
| `FIRST` | `a` | false | ✓ |
| `CAUSED_BY` | `a \|\| !b` | **true** | ✗ |
| `OR` | `a \|\| b` | false | ✓ |
| `TRUE` | `true` | **true** | ✗ |

**可用 8 种：`FALSE`、`ONLY_FIRST`、`ONLY_SECOND`、`NOT_SAME`、`AND`、`FIRST`、`SECOND`、
`OR`。**

**被拒 8 种：`TRUE`、`SAME`、`NOT_OR`、`NOT_FIRST`、`NOT_SECOND`、`NOT_AND`、`CAUSES`、
`CAUSED_BY`。**

这个约束非常反直觉：`SAME`（"两者相同"）和 `NOT_OR`（"两者皆无"）看起来都是合理的集合
运算，但因为它们在「两个空集」上为真（空集等于空集、空集的补集非空），被断言直接拒绝。

### 本轮踩坑记录

这个结论是**逐步试错**得出的，前后错了 4 次：

1. 最初只预判 `TRUE` 被拒 → 运行报 `CAUSED_BY`
2. 移除 `CAUSED_BY` → 报 `CAUSES`
3. 移除 `CAUSES` → 报 `NOT_AND`
4. 移除 `NOT_AND` 并换回 `NOT_OR` → 报 `NOT_OR`
5. 移除 `NOT_OR` 与 `SAME` 后才通过

**正确做法是先把 `apply(false, false)` 的公式逐项算完再写单元表**，而不是靠运行失败逐个排查。
这正是 `_patterns.md` 第 5 条「读源码片段下结论」的又一种表现——我读了 `BooleanOp` 的完整
定义，却没有先把断言的条件代入计算。

## 其余关键点

**1. `FIRST` / `SECOND` 完全忽略一个参数。**

| 单元 | `count` |
|---|---|
| `FIRST\|lower\|upper` | 256（等于 `lower`） |
| `SECOND\|lower\|upper` | 256（等于 `upper`） |

`FIRST` 只看 `a`，`SECOND` 只看 `b`。所以 `join(x, y, FIRST)` 与 `y` 无关。

**2. `NOT_SAME` 在互补形状上返回满方块。**

| 单元 | `count` |
|---|---|
| `NOT_SAME\|lower\|upper` | **512**（全部格子） |
| `NOT_SAME\|center\|center` | **0** |

`lower` 与 `upper` 完全互补，每格都满足 `a != b`，结果为满方块 512 格。两个相同的形状则
一格都不返回。

**3. `OR` 与 `AND` 的互补关系。**

| 单元 | `count` |
|---|---|
| `OR\|lower\|upper` | 512 |
| `AND\|lower\|upper` | 0 |
| `AND\|full\|center` | 8（中央 2×2×2） |
| `OR\|full\|center` | 512 |

## 关于表示差异的处理

原版 `toAabbs()` 返回的是**合并过相邻格子**的盒列表（`optimize()` 的结果），而探针逐格
输出。若直接比较盒列表，长度会对不上。

解决办法：**两侧都输出规范化的 512 位位图**。oracle 侧遍历每个格子中心，用「点是否落在
`toAabbs()` 的任一盒内」判定该格实心。这样比较的是「哪些格子实心」，与内部表示无关。

## 未覆盖范围

- `IndexMerger` 的坐标压缩（两个形状网格不同时的合并）
- `optimize()` 的相邻盒合并策略
- `ArrayVoxelShape` 的内部索引结构（`findIndex` / `isFullWide`）
- 8 种被断言拒绝的运算（无法通过 `Shapes.join` 触发）