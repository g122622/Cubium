# 差分能力：`block_transform`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 114 个能力，49 个变换用例全部一致。`Rotation` / `Mirror` 的坐标变换此前完全未覆盖
（[`mth_rot`](./mth_rot.md) 只覆盖了角度插值）。

## 四个关键点

**1. `Rotation.rotate(Direction)` 对 `Y` 轴方向原样返回。**

```java
public Direction rotate(Direction dir) {
    if (dir.getAxis() == Direction.Axis.Y) { return dir; }
    ...
}
```

| 单元 | 结果 |
|---|---|
| `dirrot\|CLOCKWISE_90\|up` | **`up`** |
| `dirrot\|CLOCKWISE_90\|down` | **`down`** |
| `dirrot\|CLOCKWISE_180\|up` | `up` |
| `dirrot\|COUNTERCLOCKWISE_90\|down` | `down` |

**水平旋转永远不会改变 `UP`/`DOWN`。** 旋转只发生在 XZ 平面上。

**2. `Mirror` 翻转的轴与名字的直觉相反。**

```java
public Direction mirror(Direction dir) {
    if (this == FRONT_BACK && dir.getAxis() == Axis.X) { return dir.getOpposite(); }
    return this == LEFT_RIGHT && dir.getAxis() == Axis.Z ? dir.getOpposite() : dir;
}
```

| 镜像 | 翻转的轴 | 实测 |
|---|---|---|
| `FRONT_BACK` | **`X`** | `east` → **`west`** |
| `LEFT_RIGHT` | **`Z`** | `north` → **`south`** |
| `NONE` | 无 | 原样 |

**`FRONT_BACK`（前后）翻转的是东西方向，`LEFT_RIGHT`（左右）翻转的是南北方向。** 按名字直
觉会得到完全相反的结论。

其余方向原样返回：`FRONT_BACK` 作用于 `north` 仍是 `north`，`LEFT_RIGHT` 作用于 `east` 仍是
`east`。

**3. `Mirror.getRotation(axis)` 是「对该轴的等效旋转」。**

```java
public Rotation getRotation(Direction dir) {
    Axis axis = dir.getAxis();
    return (this != LEFT_RIGHT || axis != Z) && (this != FRONT_BACK || axis != X)
        ? Rotation.NONE : Rotation.CLOCKWISE_180;
}
```

| 单元 | 结果 |
|---|---|
| `mirrorrot\|LEFT_RIGHT\|z` | **`CLOCKWISE_180`** |
| `mirrorrot\|LEFT_RIGHT\|x` | `NONE` |
| `mirrorrot\|FRONT_BACK\|x` | **`CLOCKWISE_180`** |
| `mirrorrot\|FRONT_BACK\|z` | `NONE` |
| `mirrorrot\|LEFT_RIGHT\|y` | `NONE`（Y 轴永不翻转） |

只有当镜像**会翻转该轴**时才返回 `CLOCKWISE_180`。这个函数用于把「镜像 + 旋转」归一成纯旋转。

**4. `OctahedralGroup` 的坐标旋转是矩阵作用。**

`Rotation` 与 `OctahedralGroup` 的映射：

| `Rotation` | `OctahedralGroup` | 矩阵 |
|---|---|---|
| `NONE` | `IDENTITY` | 单位阵 |
| `CLOCKWISE_90` | `ROT_90_Y_NEG` | `[[0,0,-1],[0,1,0],[1,0,0]]` |
| `CLOCKWISE_180` | `ROT_180_FACE_XZ` | `[[-1,0,0],[0,1,0],[0,0,-1]]` |
| `COUNTERCLOCKWISE_90` | `ROT_90_Y_POS` | `[[0,0,1],[0,1,0],[-1,0,0]]` |

| 单元 | 结果 |
|---|---|
| `posrot\|CLOCKWISE_90\|1\|2\|3` | `-3,2,1` |
| `posrot\|COUNTERCLOCKWISE_90\|1\|2\|3` | `3,2,-1` |
| `posrot\|CLOCKWISE_90\|-1\|0\|0` | `0,0,-1`（西 → 北） |
| `posrot\|CLOCKWISE_90\|0\|0\|1` | `-1,0,0`（南 → 西） |

**我凭直觉写的这三个矩阵，实测与原版 `OctahedralGroup.rotate(Vector3i)` 完全一致**——这是一次
真正的差分验证，不是自我印证。

## 镜像的矩阵

| `Mirror` | `OctahedralGroup` | 效果 |
|---|---|---|
| `NONE` | `IDENTITY` | 不变 |
| `LEFT_RIGHT` | `INVERT_Z` | `(x,y,z)` → `(x,y,-z)` |
| `FRONT_BACK` | `INVERT_X` | `(x,y,z)` → `(-x,y,z)` |

`posmirror|FRONT_BACK|1|2|3` 得 `-1,2,3`，`posmirror|LEFT_RIGHT|1|2|3` 得 `1,2,-3`。

## 组合与逆

| 单元 | 结果 | 说明 |
|---|---|---|
| `compose\|ROT_90\|ROT_90` | `-1,2,-3` | 两次 90° = 180° ✓ |
| `compose\|ROT_90\|IDENTITY` | `-3,2,1` | 单位阵是左单位元 ✓ |
| `compose\|IDENTITY\|ROT_90` | `-3,2,1` | 也是右单位元 ✓ |
| `inverse\|ROT_90_Y_NEG` | `3,2,-1` | 等于 `COUNTERCLOCKWISE_90` ✓ |
| `inverse\|ROT_180_FACE_XZ` | `-1,2,-3` | 180° 自逆 ✓ |
| `inverts\|INVERT_X\|y` | `false` | 只声明翻转 X 轴 ✓ |

## 本轮踩坑记录

1. **C++ 数组不能整体赋值** —— `m.m = {{...}}` 对 `i32[3][3]` 成员非法，改用聚合初始化
   `Mat3{{{...}}}`。

2. **`inverse` 单元只有 2 段，但两侧都要求至少 3 段** —— 修正为 2 段。**这个检查在探针和
   oracle 各写了一次，两边都要改**，第一次只改了 oracle。

3. **`Mirror.getRotation` 需要 `BlockState`** —— 原版签名是 `getRotation(BlockState)`，取的是
   `state.getValue(...)` 的轴。为避开注册表依赖，oracle 改为按轴直接推导同一逻辑，并在注释
   里标明原实现。

## 未覆盖范围

- `Rotation.getRotated`（旋转的复合，等价于 `OctahedralGroup.compose`）
- `FrontAndTop` 的旋转
- 各枚举的序列化标识（`"none"` / `"clockwise_90"` / `"left_right"` 等）
- `Rotation` 与 `Mirror` 作用于 `BlockState` 的完整流程（依赖方块属性的具体实现）