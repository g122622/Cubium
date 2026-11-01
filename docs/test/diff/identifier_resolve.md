# 差分能力：`identifier_resolve`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 160 个能力，27 个用例全部一致。这是 [`identifier_parse`](./identifier_parse.md) /
[`identifier_path`](./identifier_path.md) 的延续——那两个覆盖了字符集校验与路径语义，
本能力覆盖**三个构造入口之间的行为差异**。

## 核心发现一：`withDefaultNamespace` 不解析 `:`，整个串当路径

```java
public static Identifier withDefaultNamespace(String path) {
    return new Identifier("minecraft", assertValidPath("minecraft", path));
}
```

**没有 `indexOf(':')`，没有拆分**——输入整体被当作路径。

| 输入 | `parse` | `withDefaultNamespace` |
|---|---|---|
| `stone` | `minecraft:stone` | `minecraft:stone` |
| **`:stone`** | **`minecraft:stone`** | **`!IdentifierException`** |
| **`foo:bar`** | **`foo:bar`** | **`!IdentifierException`** |
| `block/stone` | — | `minecraft:block/stone` |

**这是本能力最核心的差异**。`withDefaultNamespace` 只负责「补默认命名空间」，
**不负责拆分已带命名空间的输入**。传入 `"foo:bar"` 会因为 `:` 不是合法路径字符而抛异常。

若 Cubium 让 `withDefaultNamespace` 复用 `parse` 的逻辑，`"foo:bar"` 会静默变成 `foo:bar`
而不是报错——**掩盖了调用方的错误**。

## 核心发现二：`parse` 只在「无冒号」或「冒号在 0 位」时补默认命名空间

```java
public static Identifier bySeparator(String text, char separator) {
    int i = text.indexOf(separator);
    if (i >= 0) {
        String path = text.substring(i + 1);
        if (i != 0) {
            String namespace = text.substring(0, i);
            return createUntrusted(namespace, path);
        } else {
            return withDefaultNamespace(path);     // 冒号在 0 位
        }
    } else {
        return withDefaultNamespace(text);         // 无冒号
    }
}
```

| 输入 | 结果 | 分支 |
|---|---|---|
| `stone` | `minecraft:stone` | 无冒号 |
| **`:stone`** | **`minecraft:stone`** | **冒号在 0 位** |
| `foo:bar` | `foo:bar` | 正常拆分 |
| **`a:b:c`** | **`!IdentifierException`** | 第一个冒号拆分，路径 `b:c` 含 `:` |
| `''`（空串） | **`minecraft:`** | 无冒号，空路径合法 |
| `Stone` | `!IdentifierException` | 大写字母非法 |

**注意 `bySeparator` 用 `indexOf`（第一个）而不是 `lastIndexOf`。**
`a:b:c` 拆分后路径是 `b:c`，含冒号 → 非法。

## 核心发现三：`tryParse` 与 `parse` 的分支结构不同

```java
public static @Nullable Identifier tryBySeparator(String text, char separator) {
    int i = text.indexOf(separator);
    if (i >= 0) {
        String path = text.substring(i + 1);
        if (!isValidPath(path)) return null;        // ← 先查路径
        else if (i != 0) {
            String namespace = text.substring(0, i);
            return isValidNamespace(namespace) ? new Identifier(namespace, path) : null;
        } else {
            return new Identifier("minecraft", path);
        }
    } else {
        return isValidPath(text) ? new Identifier("minecraft", text) : null;
    }
}
```

`parse` 走 `createUntrusted(namespace, path)`，里面**先 `assertValidNamespace` 再 `assertValidPath`**。

**两者的校验顺序相反，但返回 `null` 还是抛异常的结果一致**：

| 输入 | `parse` | `tryParse` |
|---|---|---|
| `a:b:c` | `!IdentifierException` | **`null`** |
| `a/b:stone` | `!IdentifierException` | **`null`** |
| `minecraft:a:b` | — | **`null`** |
| `Stone` | `!IdentifierException` | **`null`** |
| `foo:Bar` | `!IdentifierException` | — |

**区别只在失败的表现形式**：`parse` 抛 `IdentifierException`，`tryParse` 返回 `null`。

## 核心发现四：命名空间与路径的字符集只差一个 `/`

```java
private static boolean validNamespaceChar(char c) {
    return c == '_' || c == '-' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '.';
}
public static boolean validPathChar(char c) {
    return c == '_' || c == '-' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9'
        || c == '/' || c == '.';
}
```

| 字符 | 命名空间 | 路径 |
|---|---|---|
| `_` `-` `.` `0-9` `a-z` | ✓ | ✓ |
| **`/`** | **✗** | **✓** |
| `:` | ✗ | ✗ |
| `A-Z` | ✗ | ✗ |

`a/b:stone` 的命名空间含 `/` → 非法（`parse` 抛异常、`tryParse` 返回 `null`）。

## 本轮实测汇总

| 单元 | 结果 |
|---|---|
| `parse\|stone` | `minecraft:stone` |
| `parse\|:stone` | `minecraft:stone` |
| **`default\|:stone`** | **`!IdentifierException`** |
| `parse\|foo:bar` | `foo:bar` |
| **`default\|foo:bar`** | **`!IdentifierException`** |
| `parse\|a:b:c` | `!IdentifierException` |
| `tryParse\|a:b:c` | `null` |
| `parse\|`（空串） | `minecraft:` |
| `parse\|Stone` | `!IdentifierException` |
| `tryParse\|Stone` | `null` |
| `parse\|minecraft:block/stone` | `minecraft:block/stone` |
| `tryParse\|block/stone` | `minecraft:block/stone` |
| `default\|block/stone` | `minecraft:block/stone` |
| `parse\|a/b:stone` | `!IdentifierException` |
| `tryParse\|a/b:stone` | `null` |
| `toString\|foo\|block/bar` | `foo:block/bar` |

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`identifier_parse`](./identifier_parse.md) | 字符集校验与 `parse` 的完整语义 |
| [`identifier_path`](./identifier_path.md) | 路径相关的字符串操作 |
| [`command_argument`](./command_argument.md) | 命令参数中的资源定位符解析 |

## 未覆盖范围

- `Identifier.bySeparator(text, sep)` 的**任意分隔符**参数（本能力只用 `':'`）
- `Identifier.read` / `CODEC` / `STREAM_CODEC`（依赖 `DynamicOps`）
- `Identifier.withPath` / `withPrefix` / `withSuffix`（实例方法）
- `Identifier.createUntrusted` 的 `assert` 断言（默认 JVM 下 `assert` 不生效）
- `toString` 之外的字符串形式（如 `toShortString`，若存在）