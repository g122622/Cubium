# 差分能力：`front_and_top`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 149 个能力，44 个用例全部一致。补全了 [`block_transform`](./block_transform.md) 文档中列出的
`FrontAndTop` 缺口。

## 核心发现一：构造函数参数顺序是 `(name, front, top)`，与常量名的直觉相反

```java
DOWN_NORTH("down_north", Direction.DOWN, Direction.NORTH),
//  name = "down_north"   front = DOWN   top = NORTH
```

**`DOWN_NORTH` 的含义是「front = DOWN，top = NORTH」**，不是「top = DOWN，front = NORTH」。

`fromFrontAndTop(front, top)` 的两个参数也是 `front` 在前：

```java
public static FrontAndTop fromFrontAndTop(Direction front, Direction top) {
    return BY_TOP_FRONT[lookupKey(front, top)];
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`front\|down\|north`** | **`down`** | `DOWN_NORTH.front() == DOWN` |
| **`top\|down\|north`** | **`north`** | `DOWN_NORTH.top() == NORTH` |
| **`front\|up\|east`** | **`up`** | `UP_EAST.front() == UP` |
| **`top\|up\|east`** | **`east`** | `UP_EAST.top() == EAST`（**不是 `up`**）|
| `front\|west\|up` | `west` | `WEST_UP.front() == WEST` |
| `top\|west\|up` | `up` | `WEST_UP.top() == UP` |

**`top|up|east → east` 是最容易读错的一条**：看到 `UP_EAST` 会以为 `front=EAST, top=UP`，
实际是 `front=UP, top=EAST`。

这个语义在 `BlockState.getStateForPlacement` 中很重要：调用 `fromFrontAndTop(facing, up)` 时，
第一个参数是**朝外的那一面**（`front`），第二个是**顶面**（`top`）。

## 核心发现二：36 种组合只有 12 种合法，其余返回 `null`

```java
private static final FrontAndTop[] BY_TOP_FRONT =
    new FrontAndTop[NUM_DIRECTIONS * NUM_DIRECTIONS];   // 6 * 6 = 36
static { for (FrontAndTop v : values()) BY_TOP_FRONT[lookupKey(v.front, v.top)] = v; }
```

查找表大小是 `6 × 6 = 36`，**只填了 12 个槽位**，其余默认 `null`。

| 类别 | 数量 |
|---|---|
| 合法组合 | **12** |
| 非法组合（返回 `null`）| **26** |

**`fromFrontAndTop` 对非法组合返回 `null`，不抛异常。**

| 单元 | 结果 |
|---|---|
| `serial\|north\|south` | `<null>`（同轴）|
| **`front\|north\|south`** | **`<null>`** |
| `top\|north\|south` | `<null>` |

## 核心发现三：12 个合法组合的完整清单

| `front` | `top` | 序列化名 |
|---|---|---|
| `down` | `east` | `down_east` |
| `down` | `north` | `down_north` |
| `down` | `south` | `down_south` |
| `down` | `west` | `down_west` |
| `up` | `east` | `up_east` |
| `up` | `north` | `up_north` |
| `up` | `south` | `up_south` |
| `up` | `west` | `up_west` |
| `west` | `up` | `west_up` |
| `east` | `up` | `east_up` |
| `north` | `up` | `north_up` |
| `south` | `up` | `south_up` |

**规律**：

- `front = DOWN` 时，`top` 可以是 **四个水平方向**（`east`/`north`/`south`/`west`）
- `front = UP` 时，`top` 也可以是**四个水平方向**
- `front` 是水平方向时，`top` 只能是 **`UP`**

**没有 `top = DOWN` 的组合**，也没有 `top` 是水平方向且 `front` 也是水平方向的组合（同轴非法）。

## 核心发现四：序列化名恒为 `front` 与 `top` 的小写拼接

```java
private FrontAndTop(String name, Direction front, Direction top) {
    this.name = name;
    ...
}
public String getSerializedName() { return this.name; }
```

12 个名字都等于 `front.getName() + "_" + top.getName()`。

**但这条规则只在合法组合上成立**——不能用来从任意 `(front, top)` 反推名字（非法组合没有名字）。

## 本轮踩坑：漏了一个枚举项

首次运行报差异：

```
oracle: {"value":"down_east"}
probe : {"value":"<null>"}
```

我的手工枚举表只抄了 11 项，**漏了 `DOWN_EAST`**。

**教训**：抄写枚举表时必须**逐行核对总数**。本能力原始表有 12 项，我先看到「12 个」这个计数
却在抄写时漏了一行——计数与内容必须一起核对。

更稳妥的做法是**让 oracle 从枚举自身枚举出合法组合**（本能力的 `buildUnits()` 就是这样做的，
36 种组合全量覆盖），这样即使抄漏也会立刻被测试抓出，而不是靠人工检查。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_transform`](./block_transform.md) | `Rotation` / `Mirror` 的坐标变换（本文档补全它的 `FrontAndTop` 缺口）|
| [`rotation_info`](./rotation_info.md) | `Rotation` 的枚举信息 |
| [`direction_plane`](./direction_plane.md) | `Direction` 的 `Axis` 与 `Plane` |
| [`block_state_names`](./block_state_names.md) | 方块状态名的序列化 |

## 未覆盖范围

- `FrontAndTop.CODEC`（序列化 codec，属 codec 型能力）
- `FrontAndTop` 作为方块属性值时 `getPossibleValues` 的枚举顺序（与 `values()` 一致）
- `FrontAndTop` 在 `CrafterBlock` / `JigsawBlock` 等具体方块中的语义
- `Rotation` 作用于 `FrontAndTop` 的变换（在 `block_transform` 中可扩展）