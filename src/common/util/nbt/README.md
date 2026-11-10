# NBT (Named Binary Tag) 模块

Minecraft NBT 格式序列化库，支持 Java Edition、Bedrock Edition 和 Mojangson 文本格式。

## 目录结构

```
src/common/util/nbt/
├── LICENSE             # MIT 许可证（原作者 Ktlo）
├── Nbt.hpp             # 主头文件 - 公共 API（TagId 枚举、Context、标签类型、类型别名）
├── NbtInternal.hpp     # 内部头文件 - VarNum/Zint 模板实现（仅 Nbt.cpp 使用）
├── Nbt.cpp             # 实现文件 - 所有函数的具体实现
├── NbtJsonUtils.hpp    # NBT-JSON 转换工具（nbtToJson、jsonToNbt、parseMojangson）
├── NbtJsonUtils.cpp    # NBT-JSON 转换工具实现
└── README.md           # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      用户代码                                │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include "util/nbt/Nbt.hpp"
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                        Nbt.hpp                               │
│  - TagId 枚举、Context 结构、预定义上下文                     │
│  - 所有标签类型声明、类型别名、工具函数                        │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include (仅 Nbt.cpp)
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    NbtInternal.hpp                           │
│  - VarNum/Zint 模板实现、内部编码函数                         │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                        Nbt.cpp                               │
│  - 所有函数的具体实现、文本格式解析、标签类型的 read/write/copy │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本模块依赖

**外部依赖**：
- C++20 标准库（`<iostream>`, `<memory>`, `<vector>`, `<map>`, `<type_traits>`）

**项目内部依赖**：
- `common/core/Types.hpp` - 基础类型定义（`i8`, `i16`, `i32`, `i64`, `u8`, `u32`, `u64`）
- `common/util/assert/AssertAll.hpp` - 断言库（仅 Nbt.cpp 使用）

### 被依赖

本模块是核心序列化基础设施，被广泛使用：

**世界存储**：
- `world/storage/reader/java/JavaChunkReader.cpp`, `JavaLevelDatReader.cpp`, `JavaWorldReader.cpp`, `JavaColumnReader.cpp`
- `world/storage/reader/bedrock/BedrockChunkReader.cpp`, `BedrockLevelDatReader.cpp`
- `world/storage/backend/JavaAnvilBackend.cpp`, `BedrockLDBBackend.cpp`
- `world/storage/entity/EntityStorageManager.cpp`, `world/storage/core/SaveFormat.cpp`, `LevelDatCodec.hpp`

**实体系统**：
- `entity/core/Entity.hpp`, `entity/serialization/EntityDeserializer.hpp`, `entity/serialization/NbtHelper.hpp`
- `entity/effect/EffectInstance.cpp`, `entity/core/BoostHelper.hpp`

**方块实体**：
- `world/blockentity/BlockEntity.hpp`, `world/blockentity/core/BlockEntityDeserializer.hpp`
- `world/blockentity/interactive/BannerEntity.cpp`

**物品系统**：
- `item/core/ItemStack.hpp`, `item/potion/PotionUtils.cpp`
- `item/enchantment/EnchantmentContainer.cpp`, `item/loot/functions/SetCustomDataFunction.cpp`
- `item/crafting/RecipeSerializers.cpp`, `item/crafting/RecipeBook.cpp`

**世界生成**：
- `world/gen/feature/template/Template.hpp`, `world/gen/feature/template/TemplateLoader.hpp`
- `world/gen/feature/template/RuleTest.hpp`

**存档与数据**：
- `world/storage/player/PlayerSaveData.hpp`
- `world/gamerule/GameRules.hpp`
- `world/map/MapData.hpp`, `MapBanner.hpp`, `MapDecoration.hpp`, `MapFrame.hpp`, `MapIdTracker.hpp`
- `world/village/Village.cpp`, `VillageManager.cpp`, `VillageGossip.cpp`
- `world/village/poi/PointOfInterest.cpp`, `PointOfInterestStorage.cpp`
- `world/village/trade/MerchantOffer.cpp`, `MerchantOffers.cpp`

**命令系统**：
- `server/command/commands/DataCommand.hpp`, `server/command/commands/DataCommand.cpp`
- `server/command/data/DataAccessor.hpp`, `server/command/support/PlayerResolver.cpp`
- `common/command/arguments/EntityArgument.hpp`, `common/command/arguments/NbtPath.hpp`

**计分板与统计**：
- `scoreboard/storage/ScoreboardSaveData.hpp`, `scoreboard/storage/ScoreboardDataManager.cpp`
- `server/stats/StatisticsManager.hpp`

**进度系统**：
- `advancement/trigger/conditions/NBTPredicate.hpp`
- `advancement/AdvancementDisplay.cpp`

**战利品系统**：
- `item/loot/functions/SetCustomDataFunction.cpp`

**其他**：
- `server/bossbar/CustomServerBossInfo.hpp`, `CustomServerBossInfoManager.hpp`

## 容易踩的坑

### 1. 忘记设置上下文

读取或写入时未设置正确的上下文会导致数据解析错误。使用 `contexts::java`（Java Edition）、`contexts::bedrock_net`（基岩版网络）、`contexts::bedrock_disk`（基岩版磁盘）、`contexts::mojangson`（文本格式）。

### 2. 列表类型不匹配

NBT 列表只能包含相同类型的元素。写入时元素类型与声明的元素 ID 不一致属于容器构造错误，
由 `MC_ASSERT_RELEASE` 断言崩溃。

### 3. 读取入口一律返回 `Result`

`tags::compound_tag::read` / `tags::read` / `parseMojangson` 等所有读取入口都返回 `Result`，
库内**不再抛出任何 C++ 异常**：长度越界、非法 tag id、截断流、SNBT 语法错误都转成 `Error`
（多为 `ErrorCode::InvalidData`）。调用方必须检查 `failed()` 再取 `value()`。

- 二进制复合标签：`tags::compound_tag::read(stream)`（先 `stream >> contexts::xxx`）
- 任意标签：`tags::read(TagId, stream)`
- SNBT 文本：`parseMojangson(text)`
- 覆盖写入已存在对象：`readCompoundInto(stream, compound)`（取代旧的 `operator>>`）

### 4. 动态类型访问错误

使用 `get<T>()` 时类型不匹配会抛出 `std::bad_cast`（写路径的既有实现）。不确定类型时，先检查标签 ID：

```cpp
auto it = compound.value.find("someKey");
if (it != compound.value.end() && it->second->id() == TagId::Int) {
    auto& value = dynamic_cast<tags::int_tag&>(*it->second).value;
}
```

### 5. 空列表的类型推断

空列表在二进制格式中需要指定元素类型，写入时会使用 `TagId::End`。

### 6. 根复合标签的特殊处理

根复合标签在写入时不需要结束标记：`tags::compound_tag root(true);  // is_root = true`

### 7. gzip 压缩

Java Edition 的 NBT 文件（如 level.dat）通常使用 gzip 压缩，需要使用 zlib 或其他库先解压。

### 8. NbtInternal.hpp 不应被外部包含

`NbtInternal.hpp` 仅供 `Nbt.cpp` 内部使用，包含 VarNum/Zint 模板实现细节，外部代码不应直接引用。

### 9. compound_tag 拷贝赋值是“整体覆盖”

`compound_tag::operator=` 会先清空目标再逐键深拷贝，目标上源标签没有的键会被移除。标准容器把 `unique_ptr<tag>` 槽位整体替换时依赖这一点（例如复合列表 `erase` 后的元素搬移），不要改回 `emplace`（`emplace` 命中已有键时不会覆盖，会导致搬移后的元素保留旧值）。

### 10. 深度比较标签时使用 equals 方法

`tag` 基类提供了 `virtual bool equals(const tag& other) const` 虚方法，支持所有标签类型的深度递归比较。对于 `compound_tag`，会递归比较所有键值对；对于 `list_tag`，按顺序比较所有元素；对于基本类型标签，直接比较值。同时提供了 `operator==` 和 `operator!=` 自由函数。

```cpp
// 通过基类引用比较多态标签
const tag& a = ...;
const tag& b = ...;
if (a.equals(b)) { ... }
if (a == b) { ... }

// 直接比较 CompoundTag
compound_tag a, b;
if (a == b) { ... }
if (a != b) { ... }

// 比较 unique_ptr<CompoundTag>
auto ptr1 = std::make_unique<compound_tag>();
auto ptr2 = std::make_unique<compound_tag>();
if (*ptr1 == *ptr2) { ... }
```

注意：`NBTPredicate::matchNBT` 是子集匹配语义（expected 是 actual 的子集），不等同于 `equals` 的精确相等语义。

### 11. SNBT 的 `\uXXXX` 是 UTF-16 码元，不是码点

SNBT 字符串支持的转义有 `\" \' \\ \/ \b \f \n \r \t \uXXXX`。其中 `\uXXXX` 按 Java 语义是
**一个 UTF-16 码元**，所以基本平面之外的字符写成一对代理（如 😀 是 `\uD83D\uDE00`），
解析时必须把高低代理合并成一个码点再编码成 UTF-8，否则会产出非法字节序列。

孤立代理（只有高位或只有低位）无法编码成合法 UTF-8，因此解析**明确报错**而不是静默替换；
这是与原版 Java 的唯一差异点——Java 的 String 可以暂存孤立代理，本项目用 UTF-8
`std::string` 表示，表示不了。

## 序列化格式对照

| 上下文 | 字节序 | 格式 | 用途 |
|-------|-------|------|-----|
| `contexts::java` | 大端序 | 二进制 | Java Edition level.dat、player.dat |
| `contexts::bedrock_net` | 小端序 | Zigzag+VarInt | Bedrock 网络协议 |
| `contexts::bedrock_disk` | 小端序 | 二进制 | Bedrock 磁盘存档 |
| `contexts::mojangson` | - | 文本 | 调试输出、命令 |

## 许可证

原始库采用 MIT 许可证，原作者为 Ktlo (2020)。详见 `LICENSE` 文件。
