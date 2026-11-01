# 差分能力：`bounding_box`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 115 个能力，29 个判定用例全部一致。结构生成的包围盒判定此前未被覆盖。

## 四个关键点

**1. 所有判定都用闭区间，边界相接算相交。**

```java
public boolean intersects(BoundingBox other) {
    return this.maxX >= other.minX && this.minX <= other.maxX
        && this.maxZ >= other.minZ && this.minZ <= other.maxZ
        && this.maxY >= other.minY && this.minY <= other.maxY;
}
```

| 单元 | 结果 |
|---|---|
| `[0,0,0,1,1,1]` vs `[1,0,0,2,1,1]` | **`true`**（X 轴相接） |
| `[0,0,0,1,1,1]` vs `[0,1,0,1,2,1]` | **`true`**（Y 轴相接） |
| `[0,0,0,1,1,1]` vs `[0,0,1,1,1,2]` | **`true`**（Z 轴相接） |
| `[0,0,0,1,1,1]` vs `[2,0,0,3,1,1]` | `false`（分离） |

**边界相接的两个盒子算相交**，因为 `1 >= 1` 成立。这与 [`aabb_ops`](./aabb_ops.md) 里浮点
`AABB` 的开区间语义**不同**——那边相接不算重叠。两者不能混用。

**2. 六个条件之间是 `&&`，任一轴完全分离即不相交。**

`[0,0,0,5,5,5]` vs `[4,10,10,9,15,15]`：X 轴重叠（`4 <= 5`），但 Y 轴 `10 > 5` 完全分离，
结果为 `false`。**只在部分轴上重叠不算相交。**

**3. 构造器对反向边界自动归一化，不抛异常。**

```java
public BoundingBox(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
    ...
    if (maxX < minX || maxY < minY || maxZ < minZ) {
        // 记录日志后自动交换
        this.minX = Math.min(minX, maxX); ...
        this.maxX = Math.max(minX, maxX); ...
    }
}
```

| 单元 | 结果 |
|---|---|
| `ctor\|5\|0\|0\|0\|0\|0` | **`0,0,0,5,0,0`** |
| `ctor\|5\|5\|5\|0\|0\|0` | **`0,0,0,5,5,5`** |
| `ctor\|1\|2\|3\|4\|5\|6` | `1,2,3,4,5,6`（正常输入不变） |

`fromCorners` 同样归一化，但它是**每个轴独立取 min/max**：
`corners(5,0,3 → 0,5,1)` 得 `0,0,1,5,5,3`。

**4. `move` 是原地修改并返回 `this`，不是返回新对象。**

```java
public BoundingBox move(int dx, int dy, int dz) {
    this.minX += dx; ... this.maxZ += dz;
    return this;
}
```

```java
BoundingBox moved = box.move(1, 0, 0);   // box 自身也被改了
```

若调用方以为它是纯函数，原对象会被一并修改。这是结构生成代码里常见的坑。

## 完整实测

| 单元 | 结果 |
|---|---|
| `encap\|0,0,0,1,1,1\|5,5,5,6,6,6` | `0,0,0,6,6,6` |
| `encap\|0,0,0,10,10,10\|2,2,2,3,3,3` | `0,0,0,10,10,10`（包含关系） |
| `move\|0,0,0,5,5,5\|1\|2\|3` | `1,2,3,6,7,8` |
| `move\|0,0,0,5,5,5\|-10\|-10\|-10` | `-10,-10,-10,-5,-5,-5` |
| `inside\|0,0,0,5,5,5\|2\|2\|6` | `false`（Z 越界） |
| `inside\|0,0,0,5,5,5\|2\|-1\|2` | `false`（Y 越界） |

`isInside` 的三个轴**顺序是 X、Z、Y**（源码里 `z` 的条件写在 `y` 之前），但因为是 `&&` 连接，
顺序不影响结果。

## 本轮严重事故：`CMakeLists.txt` 被清空

在注册 `bounding_box` 时，由于 PowerShell 脚本里 `New-Object System.Collections.Generic.List[string]`
写法错误（应为 `List[string]` 或 `[System.Collections.Generic.List[string]]`），后续的
`Set-Content` 对着空数组执行，**把 467 行的 `CMakeLists.txt` 写成了 0 字节**。

修正过程：

1. `git checkout -- tests/diff/CMakeLists.txt` 恢复
2. 改用 `@()` 数组 + `$lines.Find(...)` 的方式重做三处修改
3. 校验三张列表长度：`SUFFIXES=111`、`SOURCES=111`（不含共享的 `runner_main.cpp`）、
   `DOMAINS=111`，对齐正确

**教训**：对关键索引文件（`CMakeLists.txt`、`index.ts`）的任何批量修改，必须**先备份或在
git 干净状态下操作**，且修改后立即校验长度。这与 `_patterns.md` 第十条「三张 CMake 列表必须
同长同序」配套。

## 未覆盖范围

- `orientBox`（结构旋转与镜像，依赖 `Rotation`/`Mirror`，见 [`block_transform`](./block_transform.md)）
- `encapsulatingPositions`（`Iterable<BlockPos>` 的包围盒）
- `inflate`
- `intersects(int, int, int, int)` 的四参数重载（只比 X/Z，忽略 Y）