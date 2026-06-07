# NBT (Named Binary Tag) 模块

Minecraft NBT 格式序列化库，支持 Java Edition、Bedrock Edition 和 Mojangson 文本格式。

## 目录结构

```
src/common/util/nbt/
├── LICENSE           # MIT 许可证（原作者 Ktlo）
├── Nbt.hpp           # 主头文件 - 公共 API（TagId 枚举、Context、标签类型、类型别名）
├── NbtInternal.hpp   # 内部头文件 - VarNum/Zint 模板实现（仅 Nbt.cpp 使用）
├── Nbt.cpp           # 实现文件 - 所有函数的具体实现
└── README.md         # 本文档
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
- `item/enchantment/EnchantmentContainer.cpp`, `item/loot/functions/SetNbtFunction.cpp`
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

**其他**：
- `server/bossbar/CustomServerBossInfo.hpp`, `CustomServerBossInfoManager.hpp`

## 容易踩的坑

### 1. 忘记设置上下文

读取或写入时未设置正确的上下文会导致数据解析错误。使用 `contexts::java`（Java Edition）、`contexts::bedrock_net`（基岩版网络）、`contexts::bedrock_disk`（基岩版磁盘）、`contexts::mojangson`（文本格式）。

### 2. 列表类型不匹配

NBT 列表只能包含相同类型的元素，写入时类型不一致会抛出异常。

### 3. 动态类型访问错误

使用 `get<T>()` 时类型不匹配会抛出 `std::bad_cast`。不确定类型时，先检查标签 ID：

```cpp
auto it = compound.value.find("someKey");
if (it != compound.value.end() && it->second->id() == TagId::Int) {
    auto& value = dynamic_cast<tags::int_tag&>(*it->second).value;
}
```

### 4. 空列表的类型推断

空列表在二进制格式中需要指定元素类型，写入时会使用 `TagId::End`。

### 5. 根复合标签的特殊处理

根复合标签在写入时不需要结束标记：`tags::compound_tag root(true);  // is_root = true`

### 6. gzip 压缩

Java Edition 的 NBT 文件（如 level.dat）通常使用 gzip 压缩，需要使用 zlib 或其他库先解压。

### 7. NbtInternal.hpp 不应被外部包含

`NbtInternal.hpp` 仅供 `Nbt.cpp` 内部使用，包含 VarNum/Zint 模板实现细节，外部代码不应直接引用。

## 序列化格式对照

| 上下文 | 字节序 | 格式 | 用途 |
|-------|-------|------|-----|
| `contexts::java` | 大端序 | 二进制 | Java Edition level.dat、player.dat |
| `contexts::bedrock_net` | 小端序 | Zigzag+VarInt | Bedrock 网络协议 |
| `contexts::bedrock_disk` | 小端序 | 二进制 | Bedrock 磁盘存档 |
| `contexts::mojangson` | - | 文本 | 调试输出、命令 |

## 许可证

原始库采用 MIT 许可证，原作者为 Ktlo (2020)。详见 `LICENSE` 文件。
