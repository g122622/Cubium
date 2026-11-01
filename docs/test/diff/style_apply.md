# 差分能力：`style_apply`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 156 个能力，14 个用例全部一致。这是 [`chat_formatting`](./chat_formatting.md) 的延续——
后者覆盖 `ChatFormatting` 枚举本身，本能力覆盖 `Style` 的**格式应用与合并**。

## 核心发现一：`applyFormat` 与 `applyLegacyFormat` 的唯一区别是颜色分支

两个方法的 switch 结构完全相同，只有 `default`（颜色码）分支不同：

```java
// applyFormat
default:
    textcolor = TextColor.fromLegacyFormat(format);
    // 其余五个格式标志保持不变

// applyLegacyFormat
default:
    obool4 = false;   // obfuscated
    obool  = false;   // bold
    obool2 = false;   // strikethrough
    obool3 = false;   // underlined
    obool1 = false;   // italic
    textcolor = TextColor.fromLegacyFormat(format);
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`applyFormat\|ff0000\|1\|1\|-\|-\|-\|-\|green`** | **`55ff55\|1\|1\|-\|-\|-\|-`** | **只换色，格式保留** |
| **`applyLegacyFormat\|ff0000\|1\|1\|1\|1\|1\|-\|green`** | **`55ff55\|0\|0\|0\|0\|0\|-`** | **换色并清零格式** |

**语义**：`applyLegacyFormat` 模拟 `§` 传统格式码「换色即重置」（`§a` 会清掉之前的 `§l`），
而 `applyFormat` 不会。

### 格式码分支两者相同

| 单元 | 结果 |
|---|---|
| `applyFormat\|ff0000\|-\|-\|-\|-\|-\|-\|bold` | `ff0000\|1\|-\|-\|-\|-\|-` |
| `applyLegacyFormat\|ff0000\|-\|-\|-\|-\|-\|-\|bold` | `ff0000\|1\|-\|-\|-\|-\|-` |

`OBFUSCATED` / `BOLD` / `STRIKETHROUGH` / `UNDERLINE` / `ITALIC` 五个分支只把对应位设为 `true`，
两种方法一致。

## 核心发现二：清零的是 `false`，不是 `null`

```java
obool4 = false;      // 不是 null
```

**这一区分极其重要**：

| 值 | 含义 | `applyTo` 时的行为 |
|---|---|---|
| `null` | **未设置**（继承） | 回落到 `other` 的值 |
| `false` | **显式关闭** | **覆盖** `other` 的 `true` |

若把 `applyLegacyFormat` 实现成「设为 `null`」，则在 `applyTo` 链式合并时会错误地继承上层样式
——**本能力开发时正是这样写错，被测试抓出**：

```
oracle: 55ff55|0|0|0|0|0|-
probe : 55ff55|-|-|-|-|-
```

修正为 `0`（`false`）后一致。

## 核心发现三：`RESET` 直接返回 `EMPTY`，丢弃所有字段

```java
case RESET:
    return EMPTY;
```

| 单元 | 输入 | 结果 |
|---|---|---|
| **`applyFormat\|ff0000\|1\|1\|1\|1\|1\|hello\|reset`** | 所有字段非空 | **`-\|-\|-\|-\|-\|-\|-`** |
| **`applyLegacyFormat\|ff0000\|1\|1\|1\|1\|1\|hello\|reset`** | 同上 | **`-\|-\|-\|-\|-\|-\|-`** |

**不是**「把某几个字段设为 `false`」——`EMPTY` 的所有字段都是 `null`，**包括 `insertion`**。

注意 `hello`（insertion）也被丢弃了。

## 核心发现四：`applyTo` 的优先级是「`this` 胜利」

```java
public Style applyTo(Style other) {
    if (this == EMPTY) return other;
    if (other == EMPTY) return this;
    return new Style(
        this.color != null ? this.color : other.color,
        ... 每个字段都是「this 非空则取 this，否则取 other」);
}
```

`this` 是**覆盖层**，`other` 是**基础层**。

| 单元 | `this` | `other` | 结果 |
|---|---|---|---|
| **`ff0000\|1\|-\|-\|-\|-\|-\|0\|1\|-\|-\|-\|-`** | color=`ff0000`, bold=`1` | bold=`0`, italic=`1` | `ff0000\|1\|1\|...`（bold 取 `this` 的 `1`，italic 回落 `1`）|
| `ff0000\|0\|-\|-\|-\|-\|-\|...` | color, bold=`0` | 全空 | `ff0000\|0\|...`（`0` 不是 `null`，所以保留）|

**`bold=false` 会覆盖 `other` 的 `true`**——这证明 `null` 与 `false` 语义不同。

### `insertion` 也遵循同一规则

| 单元 | `this.insertion` | `other.insertion` | 结果 |
|---|---|---|---|
| **`-\|-\|-\|-\|-\|-\|aaa` / `-\|-\|-\|-\|-\|-\|bbb`** | `aaa` | `bbb` | **`aaa`** |

### 两个短路

| 单元 | 结果 |
|---|---|
| `applyTo` 双方都空 | `-\|-\|-\|-\|-\|-\|-` |
| `applyTo` `this` 空、`other` 有色 | `ff0000\|1\|-\|-\|-\|-\|-`（完全等于 `other`）|

## 本轮踩坑（四处）

### 一：`isBold()` 返回 `boolean`，无法区分 `null` 与 `false`

`Style` 的字段是 `Boolean`（可 `null`），但公开 getter 返回 `boolean`。oracle 必须用**反射**读取
包级私有字段：

```java
Field field = Style.class.getDeclaredField("bold");
field.setAccessible(true);
return (Boolean) field.get(style);
```

这是**「API 不足以表达语义」**的典型例子。

### 二：单元段数计算错误

样式是 **7 段**（color, bold, italic, underlined, strikethrough, obfuscated, insertion）。

- `applyFormat|<style 7 段>|<formatting>` = **9 段**（我最初写成 8 段）
- `applyTo|<style 7 段>|<style 7 段>` = **15 段**

因为段数错误，`formatting` 读到了 `parts[7]`（样式最后一格 `insertion`）而非 `parts[8]`，
报 `No enum constant ChatFormatting.-`。

### 三：`applyTo` 用例缺少模式前缀

我用行重建脚本修正段数时，脚本按 `"applyTo|` 匹配，但实际文件里这些行**不含** `applyTo|`
前缀（被上一次错误的替换删掉了），导致替换 0 条、用例变成无前缀的纯样式串。

### 四：`applyFormat` / `applyLegacyFormat` 用例也缺前缀

前 9 条用例最初只有样式 + 格式名，没有模式前缀，oracle 报「无法识别的单元」。

**这四条踩坑的共同教训**：**用「按行号/按模式重建」的方式批量改单元表极易出错**，
且错误会层层叠加。`_patterns.md` 中「批量改索引文件先备份、改完逐项校验」的条目在此再次生效——
本能力最终是靠「打印每条的段数并对照预期」才收敛的。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举本身（本能力消费它）|
| [`text_encode`](./text_encode.md) | 方块状态的文本表示 |
| [`string_util`](./string_util.md) | `§` 颜色码的剥离（共享颜色码表）|

## 未覆盖范围

- `Style.withColor` / `withBold` / … 等单字段 setter（行为是直接替换，已被 `applyTo` 隐含覆盖）
- `Style.getClickEvent` / `getHoverEvent` / `getFont`（需要构造复杂对象）
- `Style.CODEC` / `STREAM_CODEC`（序列化，依赖 `DynamicOps`）
- `Style.EMPTY` 以外的预设样式常量
- `Component` 的样式继承链（`MutableComponent.withStyle` 的实际应用，需 world 型）