# 差分能力：`boolean_property_index`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 182 个能力，**250 处差异**（350 个单元）。**探针调用 Cubium 的
`BooleanProperty::allowedValues()` / `Property::indexOf`**。

**这是本会话发现的最大规模差异，但经完整溯源后判定为「无害差异」。**

## 差异全貌

覆盖 **50 个**双方共有的布尔属性，每个属性贡献 5 处差异：

| 单元 | 原版 | Cubium |
|---|---|---|
| `values\|<prop>` | `true,false` | **`false,true`** |
| `index\|<prop>\|true` | **`0`** | `1` |
| `index\|<prop>\|false` | **`1`** | `0` |
| `at\|<prop>\|0` | **`true`** | `false` |
| `at\|<prop>\|1` | **`false`** | `true` |

`count` 与 `name` 单元**全部一致**（都是 2 个值，名字相同）。

## 根因

### 原版

```java
public final class BooleanProperty extends Property<Boolean> {
    private static final List<Boolean> VALUES = List.of(true, false);   // ← true 在前
    private static final int TRUE_INDEX = 0;
    private static final int FALSE_INDEX = 1;

    @Override
    public List<Boolean> getPossibleValues() { return VALUES; }

    @Override
    public int getInternalIndex(Boolean b) {
        return b ? TRUE_INDEX : FALSE_INDEX;      // true -> 0, false -> 1
    }
}
```

字节码确认（`javap -c`）：

```
public int getInternalIndex(java.lang.Boolean);
  0: aload_1
  1: invokevirtual  Boolean.booleanValue:()Z
  4: ifeq           11
  7: iconst_0                     // true  -> 0
  8: goto           12
 11: iconst_1                     // false -> 1
 12: ireturn
```

### Cubium

```cpp
// src/common/util/property/BooleanProperty.hpp:118
explicit BooleanProperty(const std::string& name)
    : Property<bool>(name, {false, true})      // ← false 在前
{}
```

`Property::indexOf` 用 `m_valueToIndex` 查表，该表按 `m_values` 的顺序构建：

```cpp
// src/common/util/property/Property.hpp:106
[[nodiscard]] std::optional<size_t> indexOf(const T& value) const
{
    auto it = m_valueToIndex.find(value);
    if (it != m_valueToIndex.end()) {
        return it->second;
    }
    return std::nullopt;
}
```

**根因是 `{false, true}` 的初始化顺序与原版 `List.of(true, false)` 相反。**

## 影响分析：为什么这是无害的

### 第一步：`indexOf` 确实参与状态索引计算

```cpp
// src/common/util/property/StateHolder.hpp:147
auto optIndex = prop.indexOf(value);
// ...
const size_t targetIndex = currentIndex + ((*optIndex - currentValueIndex) * layout.stateStride);
```

布尔属性的值索引换算出的偏移量会影响 `BlockState::stateId()` 的分配。**全部 50 个布尔属性的
stateId 因此与原版不同。**

### 第二步：但 stateId 不会泄漏到任何外部格式

**网络路径（Java 协议）全部经映射表转换：**

```cpp
// src/common/network/sync/VanillaChunkWire.cpp:358
globalIds[i] = blockStateMap.toJavaGlobalId(flat[i]);

// src/common/network/backend/java/codecs/JavaPlayCodecs.hpp:858, 1416
JavaBlockStateIdMap::instance().toJavaGlobalId(state->stateId())
```

**映射表按「状态名」而非「数值」匹配：**

```cpp
// src/common/network/backend/java/mappings/JavaBlockStateIdMap.cpp:65
static std::string buildLookupKey(const std::string& name,
                                  const std::map<std::string, std::string>& props)
{
    // 形如 "minecraft:acacia_button|face=floor,facing=north,powered=true"
    // properties 按 name 字母序排列
}
```

```cpp
// 同文件 L149
::mc::Block::forEachBlockState([&](const ::mc::BlockState& state) {
    const std::string key = buildLookupKey(state.blockLocation().toString(), props);
    if (auto javaId = lookupGlobalId(key)) {      // 二分查找字符串键
        m_toJava[state.stateId()] = *javaId;      // 内部 ID → Java ID
    }
});
```

**关键**：`buildLookupKey` 用的是 `IProperty::valueToString(entry.valueIndex)`，即
**值的字符串形式**（`"true"` / `"false"`），与索引无关。所以无论 Cubium 内部索引怎么排，
生成的键都是 `powered=true`，都能正确匹配到 Java 的 globalId。

### 第三步：唯一的直接写 `stateId` 的路径是死代码

```cpp
// src/common/network/buffer/RegistryByteBuf.cpp:53
void RegistryByteBuf::writeBlockStateHolder(const BlockState* state)
{
    writeVarUInt(state != nullptr ? state->stateId() : 0);
}
```

**全仓库检索确认 `writeBlockStateHolder` 无任何调用方**——是遗留死代码。

### 结论

| 层面 | 是否受影响 |
|---|---|
| Cubium 内部 `stateId` 分配 | **是**（全部布尔属性偏移）|
| Java 协议网络包 | 否（按状态名映射）|
| 存档 `block_states` 调色板 | 否（按状态名映射）|
| 方块状态字符串（`lit=true`）| 否（`valueToString` 不受索引影响）|
| `blockStateById(id)` 回查 | 否（内部一致）|

**本项目仅支持 Java 版客户端连接**，而 Java 协议路径全部经字符串键映射，因此
**布尔属性索引顺序不影响任何外部互通**。

## 判定

**不是缺陷，是无害差异。** 理由：

1. 差异真实存在（250 处差异稳定复现）；
2. 影响范围限于 Cubium 内部的 ID 分配；
3. 所有外部路径（网络、存档）都按状态名重新映射；
4. 唯一的直接序列化路径是死代码。

**但它是一个脆弱点**：若未来有人新增一条直接写 `stateId()` 的路径（例如自定义协议、
调试导出、或复活 `writeBlockStateHolder`），就会立刻变成真缺陷。因此**建议**把
`BooleanProperty` 的初始顺序改为 `{true, false}` 以消除隐患，但那属于超出差分测试范围的决策，
本轮不做代码修改。

## 已知的程序集差异

| 属性名 | 双方都存在 |
|---|---|
| **`map`** | **仅原版**（`BlockStateProperties.MAP`）|
| **`waxed`** | **仅 Cubium**（`BlockStateProperties::WAXED`）|

### `map` 缺失不影响互通

原版的 `MAP` 只用于客户端模型的假状态：

```java
// client/resources/model/BlockStateDefinitions.java:27
return new StateDefinition.Builder<Block, BlockState>(Blocks.AIR)
    .add(BlockStateProperties.MAP).create(...);
// :31 —— ITEM_FRAME_FAKE_DEFINITION
```

**不参与任何真实方块的状态定义。** 本项目客户端已停止维护，因此缺 `map` 无影响。

### `waxed` 是 Cubium 的架构选择

原版用 `OxidationLevel` 系列属性表达铜的氧化与涂蜡，Cubium 用独立的 `waxed` 布尔属性
（`CopperChestBlock::isWaxed()` 等）。**原版没有 `waxed` 布尔属性**，因此不会与 Java
客户端的状态名冲突（只要不写出这类状态）。

## 50 个共有布尔属性

```
attached    berries     bloom        bottom       can_summon
conditional cracked     crafting     disarmed     down
drag        east        enabled      extended     eye
falling     hanging     has_book     has_bottle_0 has_bottle_1
has_bottle_2 has_record in_wall      inverted     lit
locked      natural     north        occupied     ominous
open        persistent  powered      short        shrieking
signal_fire slot_0_occupied          slot_1_occupied
slot_2_occupied          slot_3_occupied
slot_4_occupied          slot_5_occupied
snowy       south       tip          triggered    unstable
up          waterlogged west
```

**多词属性全部用下划线**（`can_summon`、`has_bottle_0`、`slot_5_occupied`），
与 [`block_state_enum_strings_2`](./block_state_enum_strings_2.md) 的规律一致。

## 踩坑：PowerShell 脚本插入 `knownDifferences` 导致文件损坏

首次尝试用脚本在 `index.ts` 里插入 250 条 `knownDifferences`，出现了两个严重问题：

1. **逗号缺失**：PowerShell 的字符串插值 `${pn}` 与 `$sep` 组合在多行构建时丢失了分隔符，
   生成 `{ key: "values|attached"` 后直接换行到 `reason:`，缺少逗号。
2. **条目边界误判**：`$end` 变量在匹配 `^\s+\] \},$` 时从未命中（该格式在文件中不存在），
   导致替换范围退化成「从起点删除到文件末尾」，**把后续 30 多个既有能力条目一起删掉**。

**恢复方式**：`git checkout -- tests/diff/harness/harnesses/index.ts`（该文件已提交，无未保存工作）。

**最终方案**：**不写 `knownDifferences`**，改为在能力声明里加一行注释指向本文档：

```ts
// 布尔属性内部值顺序：原版 true 在 false 之前，Cubium 相反（无害，见文档）。
{ name: "boolean_property_index", probe: "boolean_property_index", oracle: "BooleanPropertyIndexOracle",
    key: "unit", units: { kind: "listing" }, input: booleanPropertyIndexInput },
```

**教训：**
1. 大段结构化内容（250 条对象字面量）**不要用 PowerShell 拼接**，应直接 `write` 整块文本。
2. 修改 `index.ts` 这类关键注册文件前先确认边界匹配条件**真的命中**（打印 `$start` / `$end` 行号）。
3. 边界判断失败时应当**立即中止**，而不是继续执行删除。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_state_enum_strings`](./block_state_enum_strings.md) | 枚举属性的字符串层 |
| [`block_state_enum_strings_2`](./block_state_enum_strings_2.md) | 同上（第二组）|
| [`block_state_enum_strings_3`](./block_state_enum_strings_3.md) | 同上（第三组）|
| [`block_state_ids`](./block_state_ids.md) | 方块状态调色板索引（**本能力是它的根因层**）|
| [`block_state_names`](./block_state_names.md) | 状态名组合 |

**本能力补上了 `block_state_ids` 之下的最后一层**——前几个能力反复提到的「调色板索引隐患」
在此得到完整解释。

## 未覆盖范围

- `EnumProperty` / `IntegerProperty` 的值顺序（这些按 `values()` 或显式列表，无硬编码）
- `Property::valueToString` 对非布尔类型的输出
- `BlockState::stateId()` 的实际数值序列（本能力只验证索引语义）
- `JavaBlockStateIdMap` 映射表的完整性（这是 [`block_state_ids`](./block_state_ids.md) 的职责）