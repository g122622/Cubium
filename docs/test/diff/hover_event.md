# 差分能力：`hover_event`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 159 个能力，14 个用例全部一致。这是 [`click_event`](./click_event.md) 的对偶——
两者都是 `Style` 的事件字段，结构高度相似，但有几处关键差异。

## 核心发现一：只有 3 个动作，且**全部** `allowFromServer = true`

```java
public static enum Action implements StringRepresentable {
    SHOW_TEXT("show_text", true, HoverEvent.ShowText.CODEC),
    SHOW_ITEM("show_item", true, HoverEvent.ShowItem.CODEC),
    SHOW_ENTITY("show_entity", true, HoverEvent.ShowEntity.CODEC);
```

| 动作 | 序列化名 | `allowFromServer` | `filter` |
|---|---|---|---|
| `SHOW_TEXT` | `show_text` | `true` | `True` |
| `SHOW_ITEM` | `show_item` | `true` | `True` |
| `SHOW_ENTITY` | `show_entity` | `true` | `True` |

**`count` 单元输出 `3`**，`names` 输出 `show_text,show_item,show_entity`。

### 与 `click_event` 的对比

| | `HoverEvent.Action` | `ClickEvent.Action` |
|---|---|---|
| 动作数 | **3** | **8** |
| `allowFromServer = false` 的 | **无** | **`OPEN_FILE`** |
| `filterForSerialization` 可能失败 | **从不** | 对 `OPEN_FILE` 失败 |

## 核心发现二：`filterForSerialization` 是一条**恒真的校验**

```java
private static DataResult<Action> filterForSerialization(Action action) {
    return !action.isAllowedFromServer()
        ? DataResult.error(() -> "Action not allowed: " + action)
        : DataResult.success(action, Lifecycle.stable());
}
```

代码结构与 `ClickEvent` 完全对称，但因为**所有动作都 `allowFromServer = true`**，
**这条分支在当前数据下永不触发**。

**这是一个陷阱**：若 Cubium 据此认为「悬停事件来自服务端，也需要禁用某些动作」并添加了禁用，
就会与原版不同——原版**明确允许全部三个**。

`filter|SHOW_TEXT` / `SHOW_ITEM` / `SHOW_ENTITY` 全部输出 `True`，钉住了这一点。

## 核心发现三：`toString()` 输出 `<action show_text>`，不是枚举名

```java
@Override
public String toString() { return "<action " + this.name + ">"; }
```

| 动作 | `toString()` |
|---|---|
| `SHOW_TEXT` | **`<action show_text>`** |
| `SHOW_ITEM` | **`<action show_item>`** |
| `SHOW_ENTITY` | **`<action show_entity>`** |

**两个细节**：

1. **是 `HoverEvent.Action` 独有的**——`ClickEvent.Action` 没有覆盖 `toString`，
   返回的是枚举名 `OPEN_URL`。
2. **用的是序列化名 `show_text`**，不是枚举名 `SHOW_TEXT`。

若 Cubium 直接依赖 `std::format("{}", action)` 输出枚举名，就会与原版不同。

## 核心发现四：codec 工厂方法名不同，但语义等价

```java
// HoverEvent
public static final Codec<Action> UNSAFE_CODEC = StringRepresentable.fromValues(Action::values);

// ClickEvent
public static final Codec<Action> UNSAFE_CODEC = StringRepresentable.fromEnum(Action::values);
```

**`fromValues` vs `fromEnum`**——两者都按 `values()` 建序列化名映射，输出上无差异。

记录在此仅为避免误判：看到方法名不同不会产生行为差异。

## 本轮踩坑：同样需要 bootstrap

首次运行：

```
Exception in InitializerError
```

`HoverEvent.Action` 的静态字段 `UNSAFE_CODEC` / `CODEC` 引用了 `HoverEvent.ShowText.CODEC` 等，
这些 codec 会触及 `BuiltInRegistries`。

修复（与 [`click_event`](./click_event.md) / [`chunk_region`](./chunk_region.md) 同一模式）：

```java
net.minecraft.SharedConstants.tryDetectVersion();
net.minecraft.server.Bootstrap.bootStrap();
```

这已经是**第三次**在同一类能力上踩到这个坑。`_patterns.md` 中已记录该模式，但本次仍未预先加上——
**教训：凡是能力涉及 `Component` / `Style` / 事件的 codec，oracle 一律先 bootstrap**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`click_event`](./click_event.md) | 对偶：`ClickEvent.Action`（8 个动作，1 个禁用）|
| [`style_apply`](./style_apply.md) | `Style` 的格式应用（`hoverEvent` 是其字段）|
| [`text_color`](./text_color.md) | `Style` 的颜色值 |
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举 |

## 未覆盖范围

- `HoverEvent.ShowText` / `ShowItem` / `ShowEntity` 的 record 结构与 codec 往返
  - `ShowItem` 需要 `ItemStack`（依赖注册表与组件系统）
  - `ShowEntity` 需要 `EntityType` / `UUID` / `Component`
- `HoverEvent.EntityTooltipInfo.CODEC`（`BuiltInRegistries.ENTITY_TYPE.byNameCodec()`）
- `HoverEvent.CODEC` 的 `dispatch("action", ...)` 派发
- `HoverEvent.ItemStackInfo` 的 `asTooltipComponent`（需要 tooltip 系统）