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

// UpdateAttributes（S→C，id=129）Java wire codec 往返。
// altIndex 取自 IrPacket.hpp 的 PlayPacket variant 顺序（末尾追加，故为 114）。
// 线格式：VarInt(entityId) + VarInt(count) +
//   count×[ VarInt(attributeRegistryId) + Double(base) + VarInt(modCount) +
//           modCount×[ String(id) + Double(amount) + VarInt(operation) ] ]

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

namespace {

/// 造一条无修饰符的属性快照。
AttributeSnapshot makeSnapshot(i32 attributeRegistryId, f64 base)
{
    AttributeSnapshot snapshot{};
    snapshot.attributeRegistryId = attributeRegistryId;
    snapshot.base = base;
    return snapshot;
}

/// 造一条修饰符。id 用 Identifier 形式（小写 + `.`/`_`），与 wire 约束一致。
AttributeModifierWire makeModifier(const std::string& id, f64 amount, i32 operation)
{
    AttributeModifierWire modifier{};
    modifier.id = id;
    modifier.amount = amount;
    modifier.operation = operation;
    return modifier;
}

} // namespace

TEST_F(NetworkTestBase, PlayUpdateAttributesEmpty)
{
    // 空属性列表是合法输入（实体没有任何可同步属性），需自洽往返。
    UpdateAttributes in{};
    in.entityId = 7;
    in.attributes = {};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 114u);
    EXPECT_EQ(std::get<UpdateAttributes>(out), in);
}

TEST_F(NetworkTestBase, PlayUpdateAttributesBaseOnly)
{
    UpdateAttributes in{};
    in.entityId = 42;
    in.attributes = {makeSnapshot(19, 20.0)}; // max_health 在 vanilla 注册表中的 id
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 114u);
    EXPECT_EQ(std::get<UpdateAttributes>(out), in);
}

TEST_F(NetworkTestBase, PlayUpdateAttributesWithModifiers)
{
    // 覆盖三种 operation（0=ADD_VALUE / 1=ADD_MULTIPLIED_BASE / 2=ADD_MULTIPLIED_TOTAL），
    // 以及带 `.` 与 `_` 的 Identifier 形式修饰符 id。
    UpdateAttributes in{};
    in.entityId = 9;

    auto health = makeSnapshot(19, 20.0);
    health.modifiers = {makeModifier("minecraft:effect.health_boost", 4.0, 0)};

    auto speed = makeSnapshot(22, 0.1);
    speed.modifiers = {
        makeModifier("minecraft:effect.speed", 0.2, 2),
        makeModifier("minecraft:enchantment.soul_speed", 0.0405, 1),
    };

    in.attributes = {health, speed};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 114u);
    EXPECT_EQ(std::get<UpdateAttributes>(out), in);
}

TEST_F(NetworkTestBase, PlayUpdateAttributesNegativeEntityId)
{
    // entityId 是 VarInt 编码的 i32；负数需原样往返（ZigZag 语义由 VarInt 承担）。
    UpdateAttributes in{};
    in.entityId = -1;
    in.attributes = {makeSnapshot(0, 0.0)};
    auto out = roundTripGeneric(*tables()->playCb, PlayPacket{in});
    ASSERT_EQ(out.index(), 114u);
    EXPECT_EQ(std::get<UpdateAttributes>(out), in);
}
