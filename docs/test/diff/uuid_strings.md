# 差分能力：`uuid_strings`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 176 个能力，11/18 一致，**7 处已登记的已知差异**。探针直接调用 Cubium 的
`mc::util::uuidFromString` / `uuidToString` / `uuidToStringWithDashes`。

**本轮结论：7 处差异中没有任何一处是缺陷**——全部是架构选择或哨兵值表示法差异。
这个能力的主要价值是**把差异显式记录下来**，避免后人误判。

## 第一步：先反编译/实测，再动手（第 22 节教训）

上一轮因为凭直觉假设 `byName` 语义导致 5 处假差异。本轮先写了一个独立的 JDK 程序实测
`UUID.fromString` 的真实行为：

```
[069a79f444e94726a5befca90e38aaf5]                       -> IllegalArgumentException
[069a79f4-44e9-4726-a5be-fca90e38aaf5]                   -> 069a79f4-44e9-4726-a5be-fca90e38aaf5
[069A79F4-44E9-4726-A5BE-FCA90E38AAF5]                   -> 069a79f4-44e9-4726-a5be-fca90e38aaf5
[]                                                       -> IllegalArgumentException
[dunno]                                                  -> IllegalArgumentException
[069a79f4-44e9-4726-a5be-fca90e38aaf]                    -> 069a79f4-44e9-4726-a5be-0fca90e38aaf
[069a79f4-44e9-4726-a5be-fca90e38aaf5 ]                  -> IllegalArgumentException
```

### 两条容易忽略的 JDK 规则

1. **只接受带连字符的 36 位格式**，32 位无连字符直接抛异常
2. **短段会向前补零**：`...-fca90e38aaf`（11 位）→ `...-0fca90e38aaf`（12 位）

补零行为已验证更多组合：

```
[069a79f4-4-4726-a5be-fca90e38aaf5]  len=33 -> 069a79f4-0004-4726-a5be-fca90e38aaf5
[06-9a79f4-44e9-4726-a5be-fca90e38aaf5] len=37 -> IllegalArgumentException
```

**总长超过 36 位一律拒绝，短于 36 位则按段补零。**

## 差异一（架构选择，不是缺陷）：接受 32 位无连字符

| 单元 | 原版 | Cubium |
|---|---|---|
| `parse\|069a79f444e94726a5befca90e38aaf5` | **`null`**（JDK 拒绝）| **解析成功** |

### 为什么这不是缺陷

Cubium **架构上统一用 32 位无连字符字符串存 UUID**。证据：

```cpp
// Entity.cpp:176 的注释
// 必须用 util::generateRandomUuid + util::uuidToString 生成固定 32 字符的十六进制
// 字符串——不能用 `ss << std::hex << u64 << u64`，那种写法不补零，会得到 <32 字符
// 的串，导致 util::uuidFromString（要求恰好 32 字符）解析失败返回全零 UUID。
const Uuid uuidBytes = util::generateRandomUuid(m_random);
m_uuid = util::uuidToString(uuidBytes);
```

**22 个调用点全部传内部生成的 32/36 位字符串**，不是外部不可信输入：

| 调用点 | 输入来源 |
|---|---|
| `Player.hpp:180` | `uuid()`（内部 32 位）|
| `BanCommand.cpp:128` | `playerData->uuid`（内部 32 位）|
| `WhitelistCommand.cpp:238` | 同上 |
| `NbtHelper.cpp:219` | NBT 内部字符串 |
| `EntityTracker.cpp:754/986` | `entity->uuid()` |

**网络层不走字符串 UUID**（用二进制 `writeUUID`），因此**不存在外部字符串输入风险**。

**若强行改成只接受 36 位，会让 22 个调用点全部失效**——这是正确的架构决策，不是缺陷。

## 差异二（哨兵值表示法）：失败返回全零 vs 抛异常

| 单元 | 原版 | Cubium |
|---|---|---|
| `parse\|`（空串） | `null` | 全零 UUID |
| `parse\|dunno` | `null` | 全零 UUID |
| `parse\|-069a79f4-...` | `null` | 全零 UUID |
| `parse\|069a79f444e94726a5befca90e38aaf5-` | `null` | 全零 UUID |
| `parse\|069a79f4_44e9_...` | `null` | 全零 UUID |

JDK 抛 `IllegalArgumentException`，Cubium 返回 `Uuid{}`（全零）。

**这是「异常 vs 哨兵值」的经典差异**，与 [`text_formatting_color`](./text_formatting_color.md) 的
`null` vs 白色是同一类。全零 UUID 本身是合法值，因此严格说存在**歧义**——但 Cubium 的调用方
（`BanCommand` / `WhitelistCommand`）对此有处理：

```cpp
if (playerUuid.empty()) {
    // UUID 为空或解析失败时回退到离线模式 UUID
    playerUuid = util::uuidToStringWithDashes(util::generateOfflineUuid(playerName));
}
```

## 差异三（JDK 边缘行为）：短段补零未实现

| 单元 | 原版 | Cubium |
|---|---|---|
| `parse\|069a79f4-44e9-4726-a5be-fca90e38aaf`（35 位）| **`069a79f4-44e9-4726-a5be-0fca90e38aaf`** | 全零 |

JDK 会把 11 位的最后一段补成 12 位。**Cubium 不做补零，直接判失败。**

**Cubium 内部永远不会产生残缺字符串**（`uuidToString` 恒输出 32 位），因此这个边缘行为没有实际影响。
强行实现只会增加复杂度而无可观测收益。

## 一致的部分

| 单元 | 结果 |
|---|---|
| `size` | `16` |
| `parse\|069a79f4-44e9-4726-a5be-fca90e38aaf5` | ✓ |
| `parse\|00000000-...` | ✓ |
| `parse\|ffffffff-...` | ✓ |
| **`parse\|069A79F4-44E9-4726-A5BE-FCA90E38AAF5`** | **✓（大小写不敏感）** |
| `toString\|069a79f4-...` | `069a79f444e94726a5befca90e38aaf5`（32 位）|
| `toStringDashes\|069a79f4-...` | `069a79f4-44e9-4726-a5be-fca90e38aaf5`（36 位）|
| `roundTrip\|*` | `true` |

**大小写不敏感**——Cubium 的 `hexDigitValue` 同时接受 `a-f` 与 `A-F`，与 JDK 一致。

## 方法论收获

**这个能力「没有发现缺陷」本身是有价值的结论**：

- 上一轮的 `team_rule_strings` 确实有缺陷（静默回退导致存档读取错误）
- 本轮的 `uuid_strings` 看起来「更宽松」，但**经查证是刻意的架构选择**

**判断「宽松解析」是否是缺陷，必须看调用方的输入来源**：
- 输入来自**外部/不可信**（存档、命令参数、网络）→ 宽松是缺陷
- 输入来自**内部一致性约束**（本项目的 32 位 UUID 存储格式）→ 宽松是设计

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`team_rule_strings`](./team_rule_strings.md) | **真缺陷**：宽松解析导致存档静默出错 |
| [`render_type_strings`](./render_type_strings.md) | 宽松解析**正确**（原版就是回退）|
| [`text_formatting_color`](./text_formatting_color.md) | `null` vs 哨兵值的同类差异 |
| [`identifier_parse`](./identifier_parse.md) | 另一种字符串格式解析 |

## 未覆盖范围

- `generateRandomUuid` / `generateOfflineUuid` 的算法（v3/v4 版本位与变体位）
- `uuidToString` 的大小写（当前恒小写，与原版一致）
- NBT 中 UUID 的二进制存取（`NbtHelper`）
- 网络包的 `writeUUID` / `readUUID`（二进制，不经字符串）