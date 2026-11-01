# 差分能力：`relative_mask`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 154 个能力，29 个用例全部一致。这是 [`portal_relative`](./portal_relative.md) 的延续——
`Relative` 是 `/tp` 命令与 `TeleportTransition` 共用的坐标模式描述，此前完全未覆盖。

## 核心发现一：`rotation(boolean yRot, boolean xRot)` 首个参数是 `Y_ROT`

```java
public static Set<Relative> rotation(boolean yRot, boolean xRot) {
    Set<Relative> set = EnumSet.noneOf(Relative.class);
    if (yRot) set.add(Y_ROT);        // 第一个参数
    if (xRot) set.add(X_ROT);        // 第二个参数
    return set;
}
```

| 单元 | 结果 |
|---|---|
| **`rotation\|true\|false`** | **`y_rot`** |
| **`rotation\|false\|true`** | **`x_rot`** |
| `rotation\|true\|true` | `y_rot,x_rot` |
| `rotation\|false\|false` | `-` |

**这与 `position(x, y, z)` 的「X 在前」不一致**——`rotation` 是 **Y 在前**。

这与 `/tp` 命令的语法一致：`/tp <目标> <x> <y> <z> <yaw> <pitch>`，**yaw（Y 旋转）在 pitch（X 旋转）之前**。

## 核心发现二：`Y_ROT` 的位是 `8`，`X_ROT` 的位是 `16`

```java
public enum Relative {
    X(0), Y(1), Z(2),
    Y_ROT(3), X_ROT(4),                    // Y_ROT 序数是 3，X_ROT 是 4
    DELTA_X(5), DELTA_Y(6), DELTA_Z(7),
    ROTATE_DELTA(8);
}
```

| 枚举 | bit | `1 << bit` |
|---|---|---|
| `X` | 0 | 1 |
| `Y` | 1 | 2 |
| `Z` | 2 | 4 |
| **`Y_ROT`** | **3** | **8** |
| **`X_ROT`** | **4** | **16** |
| `DELTA_X` | 5 | 32 |
| `DELTA_Y` | 6 | 64 |
| `DELTA_Z` | 7 | 128 |
| `ROTATE_DELTA` | 8 | 256 |

**`Y_ROT` 的位比 `X_ROT` 小**——这与名字里的 X/Y 顺序相反。

| 单元 | 结果 |
|---|---|
| `pack\|x` | `1` |
| `pack\|y_rot` | **`8`** |
| `pack\|x_rot` | **`16`** |
| `pack\|x,y` | `3` |
| `pack\|x,y,z` | `7` |
| **`pack\|x,y,z,y_rot,x_rot,delta_x,delta_y,delta_z,rotate_delta`** | **`511`**（全 9 位 = `2^9 - 1`）|

## 核心发现三：`unpack` 忽略高于 bit 8 的位

```java
public static Set<Relative> unpack(int mask) {
    Set<Relative> set = EnumSet.noneOf(Relative.class);
    for (Relative r : values()) if (r.isSet(mask)) set.add(r);
    return set;
}
```

`values()` 只有 9 个元素，bit 9 及以上的位**永远不会被检查**，因此被静默丢弃。

| 单元 | 结果 |
|---|---|
| `unpack\|0` | `-` |
| `unpack\|1` | `x` |
| `unpack\|8` | `y_rot` |
| `unpack\|16` | `x_rot` |
| `unpack\|256` | `rotate_delta` |
| `unpack\|511` | `x,y,z,y_rot,x_rot,delta_x,delta_y,delta_z,rotate_delta` |
| **`unpack\|1048576`** | **`-`**（`1 << 20`，高位忽略）|

**`unpack(1048576)` 返回空集而不是抛异常**——这在网络解码时意味着「非法掩码被静默当成空集」。

## 核心发现四：`position` / `direction` 是「X 在前」的三参数

```java
public static Set<Relative> position(boolean x, boolean y, boolean z) {
    if (x) set.add(X);
    if (y) set.add(Y);
    if (z) set.add(Z);
}
public static Set<Relative> direction(boolean x, boolean y, boolean z) {
    if (x) set.add(DELTA_X);
    if (y) set.add(DELTA_Y);
    if (z) set.add(DELTA_Z);
}
```

| 单元 | 结果 |
|---|---|
| `position\|true\|false\|false` | `x` |
| `position\|false\|true\|false` | `y` |
| `position\|true\|true\|true` | `x,y,z` |
| `position\|false\|false\|false` | `-` |
| `direction\|true\|false\|false` | **`delta_x`** |
| `direction\|true\|true\|true` | `delta_x,delta_y,delta_z` |

**`position` 与 `direction` 参数顺序相同，只是映射到不同的枚举**：

- `position` → `X` / `Y` / `Z`（位 0/1/2）
- `direction` → `DELTA_X` / `DELTA_Y` / `DELTA_Z`（位 5/6/7）

## 其余实测

### `union`

```java
public static Set<Relative> union(Set<Relative>... sets) {
    HashSet<Relative> hashset = new HashSet<>();
    for (Set<Relative> set : sets) hashset.addAll(set);
    return hashset;
}
```

| 单元 | 结果 |
|---|---|
| `union\|x\|y` | `x,y` |
| `union\|x,y\|-` | `x,y` |
| `union\|x,y\|y,z\|z` | `x,y,z` |
| `union\|-\|-` | `-` |

**返回的是 `HashSet`，不是 `EnumSet`**——集合类型不同，但本能力按枚举序数排序输出，掩盖了差异。

## 本轮踩坑：`union` 的泛型可变参数需要显式转换

首次编译失败：

```
error: method union in enum Relative cannot be applied to given types
```

`union` 声明为 `@SafeVarargs public static Set<Relative> union(Set<Relative>... sets)`，
传入 `Set<?>[]` 不匹配。修正：

```java
@SuppressWarnings("unchecked")
Set<Relative>[] array = sets.toArray(new Set[0]);
Relative.union(array);
```

这是 Java 泛型数组与可变参数的老问题，**不是原版语义问题**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`portal_relative`](./portal_relative.md) | 传送门的相对位置计算（`Relative` 描述传送模式）|
| [`command_argument`](./command_argument.md) | 命令参数解析（`/tp` 的坐标参数）|
| [`direction_plane`](./direction_plane.md) | `Direction` 的轴与平面 |

## 未覆盖范围

- `Relative.ALL` / `ROTATION` / `DELTA` 常量集合本身（已在文档中列出，但未作为用例）
- `Relative.SET_STREAM_CODEC`（网络编解码，依赖 `ByteBuf`）
- `Relative.isSet(int)` / `getMask()`（实例方法，已被 `pack` / `unpack` 隐含覆盖）
- `PositionMoveRotation` 应用 `Relative` 集合的完整流程（需要实体状态）