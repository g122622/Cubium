# 差分能力：`bounding_box_encap`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 151 个能力，18 个用例全部一致。补全了 [`bounding_box`](./bounding_box.md) 与
[`structure_orient`](./structure_orient.md) 文档中列出的封装族缺口。

## 核心发现一：构造器只在 min/max 反了时归一化，且三个轴一起归一化

```java
public BoundingBox(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
    this.minX = minX; ... this.maxZ = maxZ;
    if (maxX < minX || maxY < minY || maxZ < minZ) {
        Util.logAndPauseIfInIde("Invalid bounding box data, inverted bounds for: " + this);
        this.minX = Math.min(minX, maxX);
        this.minY = Math.min(minY, maxY);
        this.minZ = Math.min(minZ, maxZ);
        this.maxX = Math.max(minX, maxX);
        ...
    }
}
```

**这是「有条件归一化」**，与 [`aabb_factory`](./aabb_factory.md) 中 `AABB` 的**无条件归一化**
不同——但对合法输入结果相同。

**关键**：`if` 是 `||` 连接的，只要**任一轴**反了，**三个轴全部归一化**。

| 单元 | 结果 | 说明 |
|---|---|---|
| `ctor\|0\|0\|0\|1\|1\|1` | `0,0,0,1,1,1` | 合法，原样 |
| **`ctor\|5\|5\|5\|0\|0\|0`** | **`0,0,0,5,5,5`** | 全反，归一化 |
| **`ctor\|1\|0\|0\|0\|1\|1`** | **`0,0,0,1,1,1`** | **只有 X 反**，但三轴都归一化 |
| `ctor\|0\|0\|0\|1\|1\|0` | `0,0,0,1,1,0` | Y 轴尺寸 0，仍合法 |
| `ctor\|0\|0\|0\|0\|0\|0` | `0,0,0,0,0,0` | 单点 |

**`ctor|1|0|0|0|1|1` 是验证这一点的关键用例**：只有 X 是反的（`minX=1 > maxX=0`），
但结果 `0,0,0,1,1,1` 说明 Y/Z 也被走了归一化分支。若实现成「只修反的那一轴」，
结果会是 `0,0,0,1,1,1`——**数值上恰好相同**，所以这个用例无法区分两种实现。

要区分需要构造「反轴与非反轴的值不对称」的用例，例如 `ctor|1|0|0|0|5|7`：
三轴全归一化得 `0,0,0,1,5,7`；只修 X 得 `0,0,0,1,5,7`——**仍然相同**。

实际上**两种实现永远等价**：`Math.min/max` 对已经有序的对是幂等的。所以这一点在数值上不可观测，
只是日志副作用不同。

## 核心发现二：`fromCorners` 自己先归一化，构造器的 `if` 永不触发

```java
public static BoundingBox fromCorners(Vec3i a, Vec3i b) {
    return new BoundingBox(
        Math.min(a.getX(), b.getX()), ..., Math.max(a.getZ(), b.getZ()));
}
```

传给构造器的永远是**已归一化**的值，所以 `if` 条件恒为假。

| 单元 | 结果 |
|---|---|
| `fromCorners\|0\|0\|0\|3\|2\|4` | `0,0,0,3,2,4` |
| **`fromCorners\|3\|2\|4\|0\|0\|0`** | **`0,0,0,3,2,4`**（端点顺序无关）|
| `fromCorners\|-1\|-2\|-3\|1\|2\|3` | `-1,-2,-3,1,2,3` |

## 核心发现三：`encapsulatingPositions` 空集合返回 `Optional.empty()`

```java
Iterator<BlockPos> iterator = positions.iterator();
if (!iterator.hasNext()) {
    return Optional.empty();
}
```

| 单元 | 结果 |
|---|---|
| **`encapPos\|-`** | **`<empty>`** |
| **`encapPos\|0,0,0`** | **`0,0,0,0,0,0`**（单点 → 点盒）|

**「零个位置」与「一个位于原点的位置」结果完全不同**——前者是 `Optional.empty()`，
后者是合法的点盒。调用方必须处理 `Optional`。

## 核心发现四：起始盒是「第一个点折叠成的点盒」，不是 `infinite()`

```java
BoundingBox box = new BoundingBox(iterator.next());   // (p, p, p, p, p, p)
iterator.forEachRemaining(box::encapsulate);
```

**若从 `infinite()` 起步，结果会被 `Integer.MIN_VALUE` / `MAX_VALUE` 撑满**——这是常见错误。

| 单元 | 结果 |
|---|---|
| `encapPos\|0,0,0;3,2,4` | `0,0,0,3,2,4` |
| **`encapPos\|3,2,4;0,0,0`** | **`0,0,0,3,2,4`**（顺序无关）|
| `encapPos\|-1,-2,-3;1,2,3;0,0,0` | `-1,-2,-3,1,2,3` |

## 其余实测

### `infinite()`

| 单元 | 结果 |
|---|---|
| **`infinite`** | **`-2147483648,-2147483648,-2147483648,2147483647,2147483647,2147483647`** |

直接用 `Integer.MIN_VALUE` / `Integer.MAX_VALUE` 六次。

### `encapsulating(BoundingBox, BoundingBox)`

| 单元 | 结果 |
|---|---|
| `encapBox\|0\|0\|0\|1\|1\|1\|2\|2\|2\|3\|3\|3` | `0,0,0,3,3,3` |
| `encapBox\|0\|0\|0\|1\|1\|1\|0\|0\|0\|1\|1\|1` | `0,0,0,1,1,1`（相同，幂等）|
| `encapBox\|-5\|-5\|-5\|0\|0\|0\|3\|3\|3\|8\|8\|8` | `-5,-5,-5,8,8,8` |

## 本轮情况：一次通过

写代码前完整读取了构造器、`fromCorners`、`encapsulating`、`encapsulatingPositions`、
`encapsulate(BlockPos)` 五个实现。

关键是把 **`encapslePositions` 的「首点折叠盒」起步**抄对——这是与「从 `infinite()` 起步」
最本质的区别，后者会产生一个被极值撑满的错误结果。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`bounding_box`](./bounding_box.md) | 包围盒判定（本能力补全它的封装族缺口）|
| [`structure_orient`](./structure_orient.md) | `BoundingBox.orientBox` 的朝向变换 |
| [`aabb_factory`](./aabb_factory.md) | `AABB` 的对应封装（浮点版，但 `AABB` 没有 `Optional` 语义）|
| [`blockpos_iterate`](./blockpos_iterate.md) | `BlockPos` 区间遍历 |

## 未覆盖范围

- `BoundingBox.encapsulatingBoxes`（`Iterable<BoundingBox>` 版，与 `encapsulatingPositions`
  同构，只是起点换成首个盒子）
- `BoundingBox.encapsulate` 的 `infinite()` 边界行为
- `BoundingBox.CODEC` / `STREAM_CODEC`（序列化）
- `BoundingBox.intersectingChunks`（依赖 `ChunkPos`）
- 构造器的 `Util.logAndPauseIfInIde` 日志副作用（本能力只比对数值）