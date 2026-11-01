# 差分能力：`identifier_parse`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 144 个能力，33 个用例全部一致。`Identifier`（资源定位符）此前**完全未被覆盖**——它是整个资源
系统（方块、物品、音效、配方、数据包）的命名基础。

## 核心发现一：命名空间与路径的白名单只差一个 `/`

```java
public static boolean validPathChar(char c) {
    return c == '_' || c == '-' || c >= 'a' && c <= 'z'
        || c >= '0' && c <= '9' || c == '/' || c == '.';
}

private static boolean validNamespaceChar(char c) {
    return c == '_' || c == '-' || c >= 'a' && c <= 'z'
        || c >= '0' && c <= '9' || c == '.';        // 注意：没有 '/'
}
```

| 字符 | 命名空间 | 路径 |
|---|---|---|
| `_` `-` `.` `0-9` `a-z` | 允许 | 允许 |
| **`/`** | **禁止** | **允许** |
| `A-Z` | 禁止 | 禁止 |
| 其他（空格、`:` 等）| 禁止 | 禁止 |

| 单元 | 结果 |
|---|---|
| `path\|foo/bar` | `True` |
| `path\|foo/bar.txt` | `True` |
| **`ns\|foo/bar`** | **`False`** |
| `ns\|foo` | `True` |

`minecraft:foo/bar` 合法，`foo/bar:baz` **非法**。

## 核心发现二：空串是合法的，原版没有任何非空校验

```java
public static boolean isValidPath(String s) {
    for (int i = 0; i < s.length(); i++) { ... }   // 空串循环零次
    return true;
}
```

| 单元 | 结果 |
|---|---|
| **`path\|`** | **`True`** |
| **`ns\|`** | **`True`** |
| **`parse\|foo:`** | **`foo:`**（命名空间 `foo` + 空路径，**不抛异常**）|

**这是本能力最反直觉的结论**：`Identifier.parse("foo:")` 得到一个路径为空串的合法标识符。

若 Cubium 加上 `if (path.isEmpty()) throw new IdentifierException(...)`，就会与原版产生差异。

## 核心发现三：`bySeparator` 只看**第一个**分隔符

```java
public static Identifier bySeparator(String s, char sep) {
    int i = s.indexOf(sep);
    if (i >= 0) {
        String path = s.substring(i + 1);
        if (i != 0) {
            String namespace = s.substring(0, i);
            return createUntrusted(namespace, path);
        }
        return withDefaultNamespace(path);
    }
    return withDefaultNamespace(s);
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `parse\|minecraft:stone` | `minecraft:stone` | |
| `parse\|foo/bar:baz` | `<invalid>` | 命名空间含 `/` |
| **`parse\|foo:bar:baz`** | **`<invalid>`** | 路径 `bar:baz` 含 `:` |
| **`parse\|:foo`** | **`minecraft:foo`** | `i == 0` 走默认命名空间 |
| `parse\|foo` | `minecraft:foo` | 无分隔符 |
| **`bySep\|foo.bar\|.`** | **`foo:bar`** | 自定义分隔符 |
| **`bySep\|.bar\|.`** | **`minecraft:bar`** | 分隔符在位置 0 |

**`foo:bar:baz` 非法**：`substring(i + 1)` 把 `bar:baz` 全当成路径，而 `:` 不在路径白名单里。

## 核心发现四：`parse` 与 `tryParse` 只差错误处理

```java
public static Identifier parse(String s) { return bySeparator(s, ':'); }        // 非法抛异常
public static @Nullable Identifier tryParse(String s) { return tryBySeparator(s, ':'); }  // 返回 null
```

| 单元 | `parse` | `tryParse` |
|---|---|---|
| `minecraft:stone` | `minecraft:stone` | `minecraft:stone` |
| `Foo:bar` | `<invalid>`（抛异常）| `<invalid>`（`null`）|
| `foo:bar:baz` | `<invalid>` | `<invalid>` |

**两者在合法输入上行为完全一致**，区别只在非法时是抛异常还是返回 `null`。

本能力用 `"<invalid>"` 字符串**统一表示**异常与 `null`——两侧一致，避免 oracle 因未捕获异常
整体失败。

## 其余实测

### `tryBuild`

| 单元 | 结果 |
|---|---|
| `tryBuild\|foo\|bar` | `foo:bar` |
| **`tryBuild\|Foo\|bar`** | `<invalid>`（命名空间大写）|
| **`tryBuild\|foo\|Bar`** | `<invalid>`（路径大写）|

### `withDefaultNamespace`

| 单元 | 结果 |
|---|---|
| `withDefault\|foo/bar` | `minecraft:foo/bar` |
| **`withDefault\|Foo`** | `<invalid>` |

### `validPathChar`

| 单元 | 结果 |
|---|---|
| `pathChar\|97`（`a`）| `True` |
| `pathChar\|65`（`A`）| **`False`** |
| `pathChar\|47`（`/`）| `True` |
| `pathChar\|58`（`:`）| **`False`** |
| `pathChar\|95`（`_`）| `True` |

## 本轮踩坑：`namespace` 是 C++ 关键字

首次编译失败：

```
error: expected member name or ';' after declaration specifiers
```

原因是我在 C++ 里写了：

```cpp
struct Parsed {
    std::string namespace;   // 错误：namespace 是关键字
    std::string path;
};
```

**Java 的字段名 `namespace` 在 C++ 里是保留字**。改名为 `nameSpace` 即可。

`_patterns.md` 里已有「语言级差异不该改」的条目，这是同一类的实例：**标识符级别的关键字冲突
必须靠改名解决**，不能指望编译器放过。

### 顺带发现的 `path` 无冲突

`path` 在 C++ 里若同时 `using namespace std;` 会与 `std::filesystem::path` 冲突，但本项目
**不使用 `using namespace std`**，所以 `std::string path;` 是安全的。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_state_names`](./block_state_names.md) | 方块状态名的序列化（依赖 `Identifier`）|
| [`item_summary`](./item_summary.md) | 物品标识的汇总 |
| [`block_tags`](./block_tags.md) | 标签键（`Identifier` 列表）|
| [`string_util`](./string_util.md) | 字符串工具（本能力处理标识符语义）|

## 未覆盖范围

- `Identifier.read`（返回 `DataResult`，涉及 `DynamicOps`）
- `Identifier.withPath` / `withPrefix` / `withSuffix`（实例方法，行为是字符串拼接 + 校验）
- `Identifier.createUntrusted` 的**断言**（Java 的 `assert` 在默认 JVM 下不生效，因此非法输入
  只在实际构造时才抛 `IdentifierException`——本能力通过 `parse` 覆盖了这条路径）
- 数据包加载时对 `Identifier` 的完整解析链（属 world 型）