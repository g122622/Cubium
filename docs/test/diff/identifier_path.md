# 差分能力：`identifier_path`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 145 个能力，17 个用例全部一致。这是 [`identifier_parse`](./identifier_parse.md) 的补充——
后者覆盖静态解析与校验，本能力覆盖**实例方法的字符串派生**。

## 核心发现一：`toLanguageKey()` 只把 `:` 换成 `.`，路径里的 `/` 保持不变

```java
public String toLanguageKey() {
    return this.namespace + "." + this.path;
}
```

**注意这里没有任何 `replace` 调用**——它只是把分隔符 `:` 换成 `.` 并拼接。

| 单元 | 结果 |
|---|---|
| **`lang\|minecraft\|block/stone`** | **`minecraft.block/stone`** |
| `lang\|foo\|bar/baz` | `foo.bar/baz` |
| `lang\|minecraft\|stone` | `minecraft.stone` |

**`minecraft:block/stone` 得到 `minecraft.block/stone`，不是 `minecraft.block.stone`。**

这与直觉相反——很多人以为翻译键会把 `/` 也换成 `.`。实际**只有命名空间与路径之间的那个 `:` 变成
`.`**，路径内部的分隔符原样保留。

因此方块石头的完整翻译键是 `"block.minecraft.block/stone"` 这类形式（具体前缀由调用方拼接），
而不是 `"block.minecraft.block.stone"`。

### `toLanguageKey(String prefix)`

| 单元 | 结果 |
|---|---|
| `langPrefix\|minecraft\|stone\|block` | `block.minecraft.stone` |
| `langPrefix\|foo\|bar\|item` | `item.foo.bar` |

```java
public String toLanguageKey(String prefix) { return prefix + "." + this.toLanguageKey(); }
```

前缀后面**总有一个 `.`**，即使 `toLanguageKey()` 的结果里已经含有 `/`。

## 核心发现二：`toShortLanguageKey()` / `toShortString()` 只对 `minecraft` 生效

```java
public String toShortLanguageKey() {
    return this.namespace.equals("minecraft") ? this.path : this.toLanguageKey();
}
public String toShortString() {
    return this.namespace.equals("minecraft") ? this.path : this.toString();
}
```

| 单元 | 结果 |
|---|---|
| **`shortLang\|minecraft\|block/stone`** | **`block/stone`** |
| `shortLang\|foo\|bar/baz` | `foo.bar/baz` |
| **`shortStr\|minecraft\|block/stone`** | **`block/stone`** |
| `shortStr\|foo\|bar/baz` | `foo:bar/baz` |

**`equals("minecraft")` 是精确匹配**——不接受大小写变体。不过命名空间本身只允许 `a-z`，所以大写
命名空间在构造时就会失败。

**注意两者的简写结果都是 `path`，完全相同**：`shortLang` 与 `shortStr` 在 `minecraft` 命名空间下
返回同一个值；只有在非 `minecraft` 命名空间下才分道扬镳（一个用 `.`，一个用 `:`）。

## 核心发现三：`toDebugFileName()` 把 `/` 与 `:` 换成 `_`，但 `.` 保留

```java
public String toDebugFileName() {
    return this.toString().replace('/', '_').replace(':', '_');
}
```

| 单元 | 结果 |
|---|---|
| **`debugFile\|minecraft\|block/stone`** | **`minecraft_block_stone`** |
| **`debugFile\|foo\|bar/baz.txt`** | **`foo_bar_baz.txt`** |

**`.` 不在替换列表里**——`bar/baz.txt` 变成 `bar_baz.txt`，扩展名保留。

先替换 `/` 再替换 `:` 与顺序无关（两个字符互不影响）。

**用途**：生成合法的文件名（`:` 与 `/` 在 Windows 文件系统上非法）。

## 核心发现四：`withPrefix` / `withSuffix` 不是纯字符串拼接，会重新校验

```java
public Identifier withPath(String path) {
    return new Identifier(this.namespace, assertValidPath(this.namespace, path));
}
public Identifier withPrefix(String prefix) { return this.withPath(prefix + this.path); }
public Identifier withSuffix(String suffix) { return this.withPath(this.path + suffix); }
```

| 单元 | 结果 |
|---|---|
| **`withPrefix\|minecraft\|stone\|block/`** | `minecraft:block/stone` |
| **`withPrefix\|minecraft\|stone\|Block/`** | **`<invalid>`** |
| **`withSuffix\|minecraft\|stone\|_top`** | `minecraft:stone_top` |
| **`withSuffix\|minecraft\|stone\|_TOP`** | **`<invalid>`** |
| `withPath\|minecraft\|stone\|block/stone` | `minecraft:block/stone` |
| **`withPath\|minecraft\|stone\|Stone`** | **`<invalid>`** |

**`withPrefix("Block/")` 会抛 `IdentifierException`**——因为拼接结果 `Block/stone` 含大写。
若 Cubium 把 `withPrefix` 实现成纯字符串拼接并直接构造，就会得到一个非法标识符而不报错。

**这是「构造器归一化」的反面**：`withPath` 走的是**校验后构造**，不做任何大小写归一化。

## 本轮情况：一次通过

写代码前完整读取了 `withPath` / `withPrefix` / `withSuffix` / `toLanguageKey` 系列 /
`toDebugFileName` / `toShortString` 的实现。

关键是把 `toLanguageKey` 正确理解为「只换分隔符」而不是「全替换成 `.`」——这一点通过实测
`lang|minecraft|block/stone → minecraft.block/stone` 得到确认。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`identifier_parse`](./identifier_parse.md) | 静态解析与字符校验（本能力是实例方法层）|
| [`block_state_names`](./block_state_names.md) | 方块状态名序列化 |
| [`item_summary`](./item_summary.md) | 物品标识汇总 |
| [`string_util`](./string_util.md) | 通用字符串工具 |

## 未覆盖范围

- `Identifier.compareTo`（按 `namespace` 再按 `path` 的字典序，可以单独覆盖）
- `Identifier.equals` / `hashCode`（`hashCode` 是 `31 * namespace.hashCode() + path.hashCode()`）
- `Identifier.toShortLanguageKey` 在非 `minecraft` 且路径含 `/` 时的完整形态（已由
  `shortLang|foo|bar/baz` 覆盖）
- `withPath(UnaryOperator<String>)` 重载（需要 lambda，行为与 `withPath(String)` 一致）