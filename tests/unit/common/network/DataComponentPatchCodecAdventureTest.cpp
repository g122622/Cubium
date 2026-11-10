/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file DataComponentPatchCodecAdventureTest.cpp
 * @brief can_place_on / can_break 的 BlockPredicate 线格式解码测试
 *
 * 覆盖 `_readBlockPredicateWire` 的每条路径：空的谓词列表、直接方块列表、方块标签、
 * 精确状态属性、两种方块共享同一组属性，以及三种「项目无法表达」的输入（NBT 谓词、
 * 组件匹配、区间属性）必须报错而不是静默丢弃。
 *
 * 每个用例都断言解码后**恰好消费完**输入字节：这是本改动的核心目的——此前的实现只读
 * VarInt(count) 就返回，非空列表会让后续字段整体错位。
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/network/backend/java/codecs/DataComponentPatchCodec.hpp"
#include "common/network/backend/java/mappings/JavaBlockIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"

namespace {

using namespace mc;
using mc::item::component::componentTypeId;
using mc::item::component::DataComponentPatch;
using mc::item::component::DataComponentPayload;
using mc::item::component::DataComponentType;
using mc::network::backend::java::readPatchFromWire;
using mc::network::buffer::ByteBuf;

/// 写一个 BlockPredicate：直接方块列表 + 可选精确属性，NBT 与组件匹配段留空。
void _writeBlockPredicate(ByteBuf& buf,
    const std::vector<std::string>& blockNames,
    const std::vector<std::pair<std::string, std::string>>& properties)
{
    buf.writeBool(true);                                      // 第 1 段：方块集合存在
    buf.writeVarInt(static_cast<i32>(blockNames.size()) + 1); // HolderSet: n = size + 1
    for (const auto& name : blockNames) {
        buf.writeVarInt(static_cast<i32>(network::backend::java::JavaBlockIdMap::instance().toJavaRegistryId(name)));
    }

    buf.writeBool(!properties.empty()); // 第 2 段：状态属性
    if (!properties.empty()) {
        buf.writeVarInt(static_cast<i32>(properties.size()));
        for (const auto& [key, value] : properties) {
            buf.writeString(key);
            buf.writeU8(0); // 0 = 精确匹配（ExactMatcher）
            buf.writeString(value);
        }
    }

    buf.writeBool(false); // 第 3 段：无 NBT 谓词
    buf.writeVarInt(0);   // 第 4 段：无 exact 组件匹配
    buf.writeVarInt(0);   // 第 4 段：无 partial 组件匹配
}

/// 把 can_place_on 组件的完整载荷（VarInt(count) + BlockPredicate*）包成 patch 并解码。
Result<DataComponentPatch> _decodePatch(ByteBuf& payload)
{
    ByteBuf wire;
    // DataComponentPatch 线格式：VarInt(addedCount) + VarInt(removedCount) + added 段 + removed 段
    // （两个计数都在最前，不是交错排列）。
    wire.writeVarInt(1); // addedCount
    wire.writeVarInt(0); // removedCount
    wire.writeVarInt(componentTypeId(DataComponentType::CanPlaceOn));
    wire.writeBytes(payload.data(), payload.size());
    ByteBuf reader(wire.data(), wire.size());
    return readPatchFromWire(reader);
}

/// 从解码结果里取出 can_place_on 的谓词。
const AdventureModePredicate& _canPlaceOnOf(const DataComponentPatch& patch)
{
    const i32 wanted = componentTypeId(DataComponentType::CanPlaceOn);
    for (const auto& entry : patch.added()) {
        if (entry.typeId == wanted) {
            return std::get<AdventureModePredicate>(entry.value);
        }
    }
    ADD_FAILURE() << "can_place_on payload not found in decoded patch";
    static const AdventureModePredicate EMPTY;
    return EMPTY;
}

class DataComponentPatchCodecAdventureTest : public ::testing::Test {
protected:
    // 方块注册表与 Java 映射由 tests/unit/main.cpp 的全局环境初始化，这里不需要额外准备。
};

TEST_F(DataComponentPatchCodecAdventureTest, EmptyPredicateListDecodes)
{
    ByteBuf payload;
    payload.writeVarInt(0);

    auto decoded = _decodePatch(payload);
    ASSERT_TRUE(decoded.success()) << decoded.error().message();
    EXPECT_TRUE(_canPlaceOnOf(decoded.value()).isEmpty());
}

TEST_F(DataComponentPatchCodecAdventureTest, DirectBlockListDecodesToPredicate)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    _writeBlockPredicate(payload, {"minecraft:stone"}, {});

    auto decoded = _decodePatch(payload);
    ASSERT_TRUE(decoded.success()) << decoded.error().message();
    const auto& predicates = _canPlaceOnOf(decoded.value()).getPredicates();
    ASSERT_EQ(predicates.size(), 1U);
    EXPECT_EQ(predicates[0], "minecraft:stone");
}

TEST_F(DataComponentPatchCodecAdventureTest, BlockTagDecodesToTagPredicate)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    payload.writeBool(true);
    payload.writeVarInt(0); // HolderSet: 0 = 标签引用
    payload.writeString("minecraft:stone_bricks");
    payload.writeBool(false);
    payload.writeBool(false);
    payload.writeVarInt(0);
    payload.writeVarInt(0);

    auto decoded = _decodePatch(payload);
    ASSERT_TRUE(decoded.success()) << decoded.error().message();
    const auto& predicates = _canPlaceOnOf(decoded.value()).getPredicates();
    ASSERT_EQ(predicates.size(), 1U);
    EXPECT_EQ(predicates[0], "#minecraft:stone_bricks");
}

TEST_F(DataComponentPatchCodecAdventureTest, PropertiesAttachToEveryBlockOfOnePredicate)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    _writeBlockPredicate(payload, {"minecraft:oak_stairs", "minecraft:stone_stairs"}, {{"facing", "east"}});

    auto decoded = _decodePatch(payload);
    ASSERT_TRUE(decoded.success()) << decoded.error().message();
    const auto& predicates = _canPlaceOnOf(decoded.value()).getPredicates();
    // 一个 BlockPredicate 列出两个方块 → 项目模型展开成两条谓词，且都带同一组属性
    ASSERT_EQ(predicates.size(), 2U);
    EXPECT_EQ(predicates[0], "minecraft:oak_stairs{facing=east}");
    EXPECT_EQ(predicates[1], "minecraft:stone_stairs{facing=east}");
}

TEST_F(DataComponentPatchCodecAdventureTest, MultiplePredicatesAreAllRead)
{
    ByteBuf payload;
    payload.writeVarInt(2);
    _writeBlockPredicate(payload, {"minecraft:stone"}, {});
    _writeBlockPredicate(payload, {"minecraft:dirt"}, {{"snowy", "true"}});

    auto decoded = _decodePatch(payload);
    ASSERT_TRUE(decoded.success()) << decoded.error().message();
    const auto& predicates = _canPlaceOnOf(decoded.value()).getPredicates();
    ASSERT_EQ(predicates.size(), 2U);
    EXPECT_EQ(predicates[0], "minecraft:stone");
    EXPECT_EQ(predicates[1], "minecraft:dirt{snowy=true}");
}

TEST_F(DataComponentPatchCodecAdventureTest, NbtPredicateIsRejectedInsteadOfSilentlyDropped)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    payload.writeBool(true);
    payload.writeVarInt(2); // 1 个方块
    payload.writeVarInt(
        static_cast<i32>(network::backend::java::JavaBlockIdMap::instance().toJavaRegistryId("minecraft:chest")));
    payload.writeBool(false); // 无属性
    payload.writeBool(true);  // 有 NBT 谓词 → 项目无法表达

    auto decoded = _decodePatch(payload);
    EXPECT_FALSE(decoded.success());
}

TEST_F(DataComponentPatchCodecAdventureTest, ComponentMatchersAreRejectedInsteadOfSilentlyDropped)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    payload.writeBool(true);
    payload.writeVarInt(2);
    payload.writeVarInt(
        static_cast<i32>(network::backend::java::JavaBlockIdMap::instance().toJavaRegistryId("minecraft:chest")));
    payload.writeBool(false);
    payload.writeBool(false);
    payload.writeVarInt(1); // 非空 exact 组件匹配 → 项目无法表达

    auto decoded = _decodePatch(payload);
    EXPECT_FALSE(decoded.success());
}

TEST_F(DataComponentPatchCodecAdventureTest, RangedPropertyMatcherIsRejected)
{
    ByteBuf payload;
    payload.writeVarInt(1);
    payload.writeBool(true);
    payload.writeVarInt(2);
    payload.writeVarInt(
        static_cast<i32>(network::backend::java::JavaBlockIdMap::instance().toJavaRegistryId("minecraft:oak_stairs")));
    payload.writeBool(true); // 有属性
    payload.writeVarInt(1);
    payload.writeString("rotation");
    payload.writeU8(1); // 1 = 区间匹配（RangedMatcher）→ 项目无法表达

    auto decoded = _decodePatch(payload);
    EXPECT_FALSE(decoded.success());
}

} // namespace
