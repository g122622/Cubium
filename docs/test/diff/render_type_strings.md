# 差分能力：`render_type_strings`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 175 个能力，11 个用例全部一致。**探针直接调用 Cubium 的 `mc::scoreboard::renderTypeToString` /
`renderTypeFromString`**，是第五个「对接真实实现」的能力。

**本能力未发现 Cubium 缺陷**，但发现了一条**重要的原版 API 规律**。

## 核心发现：`EnumCodec.byName` 有三个重载，语义完全不同

```java
public E byName(String);                              // 未知 → null
public E byName(String, E fallback);                  // 未知 → fallback
public E byName(String, Supplier<? extends E>);       // 未知 → supplier.get()
```

### 这不只是理论——两个相邻的能力就是相反结论

| 能力 | 原版调用的重载 | 未知输入结果 |
|---|---|---|
| [`team_rule_strings`](./team_rule_strings.md) | `byName(String)` | **`null`** |
| **`render_type_strings`** | **`byName(String, INTEGER)`** | **`INTEGER`** |

### 字节码证据

```java
// ObjectiveCriteria.RenderType.byId —— 注意这里的双参数重载
public static RenderType byId(String input) {
    return CODEC.byName(input, INTEGER);      // ← 带默认值
}
```

```
  0: getstatic     #62   // Field CODEC
  3: aload_0             // input
  4: getstatic     #64   // Field INTEGER      ← 第二个参数
  7: invokevirtual #68   // EnumCodec.byName:(Ljava/lang/String;Ljava/lang/Enum;)Ljava/lang/Enum;
```

而 [`team_rule_strings`](./team_rule_strings.md) 的 `Visibility` 走的是单参数重载，未知输入返回 `null`。

### 对 Cubium 的意义

**两个能力的 Cubium 实现都「正确」**，因为它们是**分别对齐了两个不同的重载**：

| 能力 | Cubium 实现 | 对齐的原版重载 |
|---|---|---|
| `team_rule_strings` | `teamVisibilityParse` → `optional` | `byName(String)` |
| `render_type_strings` | `renderTypeFromString` → 回退 `Integer` | `byName(String, INTEGER)` |

**这是本能力最重要的方法论教训**：上一轮我把 `team_rule_strings` 的「未知输入回退」判定为缺陷并修复，
而本能力如果照搬同样的判断，会**把正确的实现改成错误的**。

**必须逐个核对原版调用的是哪个重载，不能凭「未知输入应该返回 null」这个直觉推广。**

## 我本轮踩的坑：误判导致 5 处假差异

最初我假设原版 `byId` 走单参数 `byName`（返回 `null`），于是探针调用 Cubium 的严格入口
`renderTypeParse`：

```
parse|         oracle=INTEGER  probe=null
parse|dunno    oracle=INTEGER  probe=null
parse|health   oracle=INTEGER  probe=null
parse|HEARTS   oracle=INTEGER  probe=null
parse|INTEGER  oracle=INTEGER  probe=null
```

**5 处差异全部是假差异**——Cubium 的宽松入口才是对的。

用 `javap -p -c` 反编译 `byId` 后才看清是双参数重载。改用 `renderTypeFromString` 后 11/11 一致。

**教训**：判断反序列化失败行为时，必须反编译看到实际的 `invokevirtual` 签名，
不能只看方法名（`byId` 这个名字完全没有提示用哪个重载）。

## 核心验证：只有两项

| 枚举 | `getSerializedName()` |
|---|---|
| `INTEGER` | `integer` |
| `HEARTS` | `hearts` |

`names` 输出：`integer,hearts`

**顺序是 `INTEGER` 在前**（与 `values()` 顺序一致）。

### 未知输入全部回退到 `INTEGER`

| 单元 | 结果 |
|---|---|
| `parse\|integer` | `INTEGER` |
| `parse\|hearts` | `HEARTS` |
| **`parse\|dunno`** | **`INTEGER`** |
| **`parse\|`（空串）** | **`INTEGER`** |
| **`parse\|INTEGER`** | **`INTEGER`**（大小写敏感，未命中→回退）|
| **`parse\|HEARTS`** | **`INTEGER`**（同上）|
| **`parse\|health`** | **`INTEGER`** |

## 历史名字：`health` 已被改为 `hearts`

原版有一个数据修复类 `ObjectiveRenderTypeFix`：

```java
return objective.equals("health") ? "hearts" : "integer";
```

说明**旧版本的名字是 `health`，现在叫 `hearts`**。

`parse|health` 返回 `INTEGER` 而非 `HEARTS`——**`health` 不是合法的当前序列化名**，
只有经过数据修复（读旧存档）才会变成 `hearts`。

## Cubium 提供的严格入口

Cubium 额外提供 `renderTypeParse` 返回 `std::optional<RenderType>`，**原版没有对应入口**。

这是**超出原版的能力**，供需要区分「非法输入」的调用方使用（例如命令参数校验）。
本能力未把它纳入对比单元，因为它没有原版对应物。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`team_rule_strings`](./team_rule_strings.md) | **单参数 `byName`** 的对照案例（未知→`null`）|
| [`permission_level`](./permission_level.md) | `INT_CODEC` 的越界回退 |
| [`difficulty`](./difficulty.md) | `byName` 单参数（未知→`null`）|
| [`objective_criteria`](./objective_criteria.md) | `RenderType` 的持有者（手抄转写）|

## 未覆盖范围

- `ObjectiveCriteria.RenderType.getId()` 与 `getSerializedName()` 的关系（源码显示两者同值）
- `ObjectiveRenderTypeFix` 的数据修复管线
- `NumberFormat` 的 `styled` / `fixed` / `blank` 三种格式（另一套体系）
- 记分板总分显示时的实际渲染效果