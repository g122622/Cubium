# 差分能力：`blockpos_iterate`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 146 个能力，10 个用例全部一致。补全了 [`blockpos_codec`](./blockpos_codec.md) 文档中列出的
`betweenClosed`（包围盒迭代顺序）缺口。

## 核心发现一：迭代顺序是 x 最快、z 最慢

```java
int i = maxX - minX + 1;      // x 跨度
int j = maxY - minY + 1;      // y 跨度
int k = maxZ - minZ + 1;      // z 跨度
int l = i * j * k;            // 总数

int i1 = index % i;               // x 偏移
int j1 = index / i;
int k1 = j1 % j;                  // y 偏移
int l1 = j1 / j;                  // z 偏移
return cursor.set(minX + i1, minY + k1, minZ + l1);
```

索引分解决定顺序：**`x` 是最内层（变化最快），`z` 是最外层（变化最慢）**。

| 单元 | 结果 |
|---|---|
| `iter\|0\|0\|0\|2\|0\|0` | `0,0,0;1,0,0;2,0,0` |
| `iter\|0\|0\|0\|0\|2\|0` | `0,0,0;0,1,0;0,2,0` |
| `iter\|0\|0\|0\|0\|0\|2` | `0,0,0;0,0,1;0,0,2` |
| **`iter\|0\|0\|0\|2\|1\|1`** | **`0,0,0;1,0,0;2,0,0;0,1,0;1,1,0;2,1,0;0,0,1;1,0,1;2,0,1;0,1,1;1,1,1;2,1,1`** |
| `iter\|0\|0\|0\|1\|1\|1` | `0,0,0;1,0,0;0,1,0;1,1,0;0,0,1;1,0,1;0,1,1;1,1,1` |

**`iter|0|0|0|2|1|1` 的顺序是 `(0,0,0) (1,0,0) (2,0,0) (0,1,0) ...`**——先是整个 x 行，再是下一行
y，最后才换 z 层。

**这与「先 z 后 x」或「z 最快」的直觉不同**。MC 的区块内遍历确实以 x 为最内层。

## 核心发现二：`betweenClosed(BlockPos, BlockPos)` 自动归一化端点

```java
return betweenClosed(
    Math.min(a.getX(), b.getX()), Math.min(a.getY(), b.getY()), Math.min(a.getZ(), b.getZ()),
    Math.max(a.getX(), b.getX()), Math.max(a.getY(), b.getY()), Math.max(a.getZ(), b.getZ()));
```

| 单元 | 结果 |
|---|---|
| `pair\|0\|0\|0\|2\|1\|1` | 12 个坐标 |
| **`pair\|2\|1\|1\|0\|0\|0`** | **与上一条完全相同** |
| `pair\|5\|5\|5\|5\|5\|5` | `5,5,5`（单点）|

**端点顺序无关**——`(2,1,1)..(0,0,0)` 会被归一化成 `(0,0,0)..(2,1,1)`。

## 核心发现三：`betweenClosed` 是闭区间

跨度计算是 `max - min + 1`，所以**两端都包含**。

| 单元 | 结果 |
|---|---|
| **`iter\|0\|0\|0\|0\|0\|0`** | **`0,0,0`**（单点仍迭代 1 次）|
| `iter\|-1\|-1\|-1\|0\|0\|0` | 8 个坐标（负坐标正常）|

## 核心发现四：六参版本不归一化，`min > max` 时**无限迭代**

```java
int i = p_121980_ - p_121977_ + 1;    // 若 minX > maxX，跨度为负
int l = i * j * k;                     // 乘积可能为负

if (this.index == l) return this.endOfData();
```

**终止条件是 `index == l`（精确相等），不是 `index >= l`。**

- 若**某个**跨度为 `0`（如 `minX = maxX + 1`），`l = 0`，迭代器**立刻结束**——空结果。
- 若**三个跨度全为负**（如 `(5,5,5)..(0,0,0)` 得 `(-5,-5,-5)`），`l = -125`。
  `index` 从 `0` 递增**永远不会等于 `-125`** → **无限迭代直到 OOM**。

**这是原版的真实缺陷**。本能力首次运行时用 `iter|5|5|5|0|0|0` 做用例，结果：

```
java.lang.OutOfMemoryError: Requested array size exceeds VM limit
```

**该用例已移除**——它无法作为差分用例（会让 oracle 进程崩溃）。

**对 Cubium 的启示**：若要复刻 `betweenClosed`，必须在 `min > max` 时**主动判空或抛异常**，
不能照搬「`index == l` 才停」的循环条件。这是一个**「不该对齐的缺陷」**。

## 本轮踩坑：OOM 用例暴露了原版缺陷

首次运行直接导致 oracle 崩溃：

```
Exception in thread "main" java.lang.OutOfMemoryError: Requested array size exceeds VM limit
```

我最初的假设是「`min > max` 时跨度为负，总数 `l <= 0`，迭代器立刻结束」——**错误**。

实际是**无限循环**，因为终止条件用了 `==` 而不是 `<=`。

**教训**：设计边界用例时，若怀疑原版存在缺陷，**先用小规模独立验证**再纳入差分框架——
一个会让 oracle 崩溃的用例无法产出可比较的结果。

`_patterns.md` 里已有「测试数据必须自校验」的条目，这是其补充：**还须保证用例本身不会让
oracle 失控**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`blockpos_codec`](./blockpos_codec.md) | `BlockPos` 的位打包与 `offset`（本文档补全它的 `betweenClosed` 缺口）|
| [`blockpos_bfs`](./blockpos_bfs.md) | 广度优先遍历（另一种访问顺序）|
| [`blockpos_geom`](./blockpos_geom.md) | 几何变换与极值 |
| [`section_pos`](./section_pos.md) | 区块坐标换算 |

## 未覆盖范围

- **`min > max` 的六参版本**（原版无限迭代 / OOM，无法作为用例）
- `betweenClosed(AABB)` 重载（内部先 `containing` 再调两参版本，`containing` 已由
  [`blockpos_geom`](./blockpos_geom.md) 覆盖）
- `betweenClosedStream` 系列（与 `betweenClosed` 同序，只是包装成 `Stream`）
- `spiralAround`（螺旋遍历，逻辑较复杂，可另立能力）
- `randomBetweenClosed`（需要随机源）