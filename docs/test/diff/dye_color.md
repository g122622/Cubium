# 差分能力：`dye_color`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 122 个能力，37 个用例全部一致。`DyeColor` 此前完全未被覆盖。

## 核心发现：四个颜色值互不相同

| 字段 | 含义 | WHITE 的值 | 十六进制 |
|---|---|---|---|
| `rgb` | 原始 RGB（无 alpha） | `16383998` | `0x00F9FFFE` |
| `textureDiffuseColor` | `ARGB.opaque(rgb)` | `-393218` | `0xFFF9FFFE` |
| `fireworkColor` | 烟花火箭颜色（**独立**） | `15790320` | `0x00F0F0F0` |
| `textColor` | 聊天文本颜色（**独立**，也 opaque） | `-1` | `0xFFFFFFFF` |

**`fireworkColor` 与 `textColor` 都不是从 `rgb` 派生的。** 例如 WHITE 的 `rgb` 是 `0xF9FFFE`
（微偏黄的白），但 `textColor` 是纯白 `0xFFFFFFFF`。

完整实测（`id,rgb,diffuse,firework,textColor,name`）：

```text
info|WHITE      -> 0,16383998,-393218,15790320,-1,white
info|ORANGE     -> 1,16351261,-425955,15435844,-38881,orange
info|LIGHT_BLUE -> 3,3847130,-12930086,6719955,-6635315,light_blue
info|RED        -> 14,11546150,-5231066,11743532,-65536,red
info|CYAN       -> 9,1481884,-15295332,2651799,-16711681,cyan
info|BLACK      -> 15,1908001,-14869215,1973019,-16777216,black
```

## 其余三个关键点

**1. `byId` 越界返回 `WHITE`（id 0），不返回 `null`。**

```java
private static final IntFunction<DyeColor> BY_ID =
    ByIdMap.continuous(DyeColor::getId, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
// ZERO: id >= 0 && id < 16 ? at[id] : at[0]
```

| 单元 | 结果 |
|---|---|
| `byId\|0` | `WHITE` |
| `byId\|15` | `BLACK` |
| **`byId\|-1`** | **`WHITE`** |
| **`byId\|16`** | **`WHITE`** |
| **`byId\|999`** | **`WHITE`** |
| **`byId\|-999`** | **`WHITE`** |

**这与 [`chat_formatting`](./chat_formatting.md) 的 `getById` 行为完全不同**：那边负数返回
`RESET`、正数越界返回 `null`；这边**全部越界都返回 `WHITE`**。两套枚举的越界策略不一致，
混用会出错。

**2. `byName` 严格匹配 `serializedName`，大小写敏感。**

```java
public static DyeColor byName(String name, DyeColor fallback) {
    DyeColor found = CODEC.byName(name);
    return found != null ? found : fallback;
}
```

`CODEC` 用 `StringRepresentable.fromEnum`，按 `getSerializedName()`（小写名）匹配。

| 单元 | 结果 |
|---|---|
| `byName\|white` | `WHITE` |
| `byName\|light_blue` | `LIGHT_BLUE` |
| **`byName\|WHITE`** | **`null`**（大写不匹配） |
| **`byName\|Light_Blue`** | **`null`** |
| `byName\|not_a_color` | `null` |

**与 `ChatFormatting.getByName` 的宽松 `cleanName`（去除非字母、转小写）完全不同**——
那边 `"Dark-Blue"` 能匹配到 `DARK_BLUE`，这边 `"Light_Blue"` 匹配不到。

**3. `byFireworkColor` 按值查表，找不到返回 `null`。**

| 单元 | 结果 |
|---|---|
| `byFirework\|15790320` | `WHITE` |
| `byFirework\|15435844` | `ORANGE` |
| **`byFirework\|16777215`** | **`null`**（这是 WHITE 的 `textColor`，不是 fireworkColor） |
| `byFirework\|0` | `null` |
| `byFirework\|12345` | `null` |

`byFirework|16777215 → null` 是个好的反例：`16777215`（`0xFFFFFF`）是 WHITE 的 **textColor**，
但 WHITE 的 fireworkColor 是 `15790320`，所以查不到。**证明两套颜色值不能混用。**

## 本轮踩坑：两个错误都是被差分测试抓出来的

第一次跑就出了 **10 处差异**，两类错误：

**错误一：`byName` 用枚举名比较，而不是序列化名。**

我把探针写成 `if (name == color.name)`，即拿 `"white"` 与 `"WHITE"` 比。正确的是与
`serializedName`（`"white"`）比。4 处差异。

**错误二：`textColor` 漏了 `ARGB.opaque` 包装。**

构造器里是 `this.textColor = ARGB.opaque(p_41051_);`，我照抄了原始 RGB 常量却没做 opaque。
6 处差异。

**这两个错误都不是设计层面的误解，而是抄写遗漏**——差分测试正好擅长捕获这类问题。这说明
**即使 oracle 用真实原版数据，探针侧的机械性错误仍需要逐单元核对输出才能发现**（如实现在跑
之前只看「通过/失败」，这两处会被当成「已知差异」放过去）。

## 未覆盖范围

- `getMixedColor`（依赖 `ServerLevel` 与配方系统，是染料混合的核心逻辑）
- `getMapColor()` 返回的 `MapColor` 对象（需要 `MapColor` 的 id 映射数据）
- `CODEC` / `STREAM_CODEC` / `LEGACY_ID_CODEC` 的序列化形式
- `toString()`（返回 `name`，与 `getSerializedName()` 相同，已被 `info` 隐含验证）