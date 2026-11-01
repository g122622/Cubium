# 差分能力：`blockpos_bfs`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 110 个能力，14 个遍历用例全部一致。方块更新的广度优先遍历此前完全未被覆盖。

## 四个关键点

**1. 去重发生在状态函数之前。**

```java
long key = pos.asLong();
if (visited.add(key)) {              // 先去重
    status = nodeStatus.apply(pos);  // 再去判状态
    ...
}
```

`visited.add` 为假时**连 `nodeStatus` 都不会调用**。所以同一个位置被多个父节点指向时，只有
第一次能触发状态判断与扩散。

更关键的是：**`SKIP` 的节点已经被标记为访问过**。后续再遇到它不会重试——`SKIP` 是永久的，
不是「这次跳过，下次再说」。

**2. 计数是先自增再比较。**

```java
if (++count >= maxCount) { return count; }
```

| 单元 | `maxCount` | 计数 |
|---|---|---|
| `2\|1\|free` | 1 | **1** |
| `2\|2\|free` | 2 | 2 |
| `2\|7\|free` | 7 | 7 |
| `1\|7\|free` | 7 | **7**（恰好等于总数，不截断） |

`maxCount = 1` 时访问第一个节点后立即返回。写成 `count++ >= maxCount` 会多访问一个。

**3. `depth < maxDepth` 是严格小于。**

| 单元 | `maxDepth` | 计数 |
|---|---|---|
| `0\|100\|free` | 0 | **1**（只有起点） |
| `1\|100\|free` | 1 | **7**（起点 + 6 邻居） |
| `2\|100\|free` | 2 | **25** |
| `3\|1000\|free` | 3 | **63** |

深度 N 的节点**被访问**但不再扩散。能触达的最大深度恰好等于 `maxDepth`。

计数规律：`1, 7, 25, 63` = `1 + 6·(1 + 2 + ... + N)`... 实际是球体的层数累加（含去重）。

**4. `SKIP` 与 `STOP` 语义完全不同。**

| 单元 | 规则 | 计数 | 说明 |
|---|---|---|---|
| `2\|100\|skipX0` | `x == 0` 跳过 | **0** | **起点被 SKIP，整个遍历立即结束** |
| `2\|100\|skipOdd` | `x` 为奇数跳过 | 13 | 跳过部分节点，遍历继续 |
| `2\|100\|stopX1` | `x == 1` 停止 | **6** | `STOP` 中断，少一个节点 |

`skipX0` 得 0 是因为**起点本身被跳过**——既不计数也不扩散，队列随即为空。

`stopX1` 得 6 而 `1|100|free` 得 7，差的就是 `(1,0,0)` 这个触发 `STOP` 的节点。

两者都**不会**把当前节点计入。

## 邻居展开顺序

```java
for (Direction direction : Direction.values()) { consumer.accept(...); }
```

`Direction.values()` 的顺序是 **`DOWN, UP, NORTH, SOUTH, WEST, EAST`**，实测遍历顺序：

```
(0,0,0) (0,-1,0) (0,1,0) (0,0,-1) (0,0,1) (-1,0,0) (1,0,0)
```

前三个是 Y 轴（DOWN、UP），接着是 Z 轴（NORTH、SOUTH），最后是 X 轴（WEST、EAST）。
**这不是 X→Y→Z 的直觉顺序。**

## 本轮踩坑记录：`BlockPos` 是可变的

首次运行 `count` 全部正确，但 `order` 只记录到 1 个或 7 个元素——**`BlockPos` 在遍历中被
复用**，我把引用 `add` 进列表后，它们全都指向最后一次写入的坐标。

即使调 `pos.immutable()` 也没用：`pos` 传进 `BiConsumer` 时其内容可能已被改写。

正解是**在 `nodeStatus` 回调里记录**——它在 `consumer` 之前调用，此刻 `pos` 尚未被复用：

```java
pos -> {
    var status = evaluate(rule, pos);
    if (status != SKIP && status != STOP) {
        order.add(pos.immutable());   // 此刻 pos 是正确的
    }
    return status;
}
```

**这一类问题在任何「回调接收可变对象」的 API 里都会重演**，值得记入 `_patterns.md`。

## 未覆盖范围

- `asLong()` 的位打包细节（见 [`blockpos_codec`](./blockpos_codec.md)）
- 真实方块的邻居函数
- `TraversalNodeStatus` 在红石线、流体里的具体判定
- 队列的 `ArrayDeque` 与 `LongOpenHashSet` 的实际性能特征