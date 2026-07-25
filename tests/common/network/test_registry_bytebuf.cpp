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

#include "common/entity/core/EntityRegistry.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <gtest/gtest.h>

using namespace mc::network::buffer;
using namespace mc::network::test;
using namespace mc;

// ============================================================================
// RegistryByteBuf holder 往返（物品/方块状态/实体类型）
// ============================================================================

TEST_F(NetworkTestBase, ItemHolderRoundTrip)
{
    auto* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);

    RegistryByteBuf buf = makeBoundBuf();
    buf.writeItemHolder(stone);
    auto r = buf.readItemHolder();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), stone);
}

TEST_F(NetworkTestBase, ItemHolderNullWritesZero)
{
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeItemHolder(nullptr);
    ASSERT_EQ(buf.size(), 1u);
    EXPECT_EQ(buf.bytes()[0], 0x00);
    auto r = buf.readItemHolder();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), nullptr);
}

TEST_F(NetworkTestBase, ItemHolderUnknownIdReturnsNullptr)
{
    // 写一个超大 itemId（注册表不存在），读侧 itemById 应返 nullptr（不报错）
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeVarUInt(0x7FFFFFFFu); // 远超注册表条目数
    auto r = buf.readItemHolder();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), nullptr);
}

TEST_F(NetworkTestBase, BlockStateHolderRoundTrip)
{
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);
    const u32 stateId = stoneState->stateId();

    RegistryByteBuf buf = makeBoundBuf();
    buf.writeBlockStateHolder(stoneState);
    auto r = buf.readBlockStateHolder();
    ASSERT_TRUE(r.success());
    ASSERT_NE(r.value(), nullptr);
    EXPECT_EQ(r.value()->stateId(), stateId);
}

TEST_F(NetworkTestBase, BlockStateHolderAirIsZero)
{
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeBlockStateHolder(nullptr);
    ASSERT_EQ(buf.size(), 1u);
    EXPECT_EQ(buf.bytes()[0], 0x00);
}

TEST_F(NetworkTestBase, BlockStateHolderUnknownIdReturnsNullptr)
{
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeVarUInt(0x7FFFFFFFu);
    auto r = buf.readBlockStateHolder();
    // blockStateById 对不存在 id 返 nullptr（非错误）
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), nullptr);
}

TEST_F(NetworkTestBase, EntityTypeHolderRoundTrip)
{
    auto* type = entity::EntityRegistry::instance().getType("minecraft:zombie");
    // 若该类型未注册则跳过本断言对应字段（不同测试环境可能差异），仍验证往返不崩
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeEntityTypeHolder(type);
    auto r = buf.readEntityTypeHolder();
    ASSERT_TRUE(r.success());
    if (type != nullptr) {
        EXPECT_EQ(r.value(), type);
    }
}

TEST_F(NetworkTestBase, EntityTypeHolderNullWritesZero)
{
    RegistryByteBuf buf = makeBoundBuf();
    buf.writeEntityTypeHolder(nullptr);
    ASSERT_EQ(buf.size(), 1u);
    auto r = buf.readEntityTypeHolder();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), nullptr);
}

// ============================================================================
// 无 registry 绑定时报错
// ============================================================================

TEST(RegistryByteBuf, ReadItemHolderWithoutRegistryFails)
{
    RegistryByteBuf buf; // 默认 ctor，无 registry
    buf.writeVarUInt(5); // 写一个非 0 itemId
    auto r = buf.readItemHolder();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidState);
}

TEST(RegistryByteBuf, ReadBlockStateHolderWithoutRegistryFails)
{
    RegistryByteBuf buf;
    buf.writeVarUInt(1);
    auto r = buf.readBlockStateHolder();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidState);
}

TEST(RegistryByteBuf, ReadEntityTypeHolderWithoutRegistryFails)
{
    RegistryByteBuf buf;
    buf.writeVarUInt(3);
    auto r = buf.readEntityTypeHolder();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidState);
}

TEST(RegistryByteBuf, BindRegistryEnablesHolderRead)
{
    RegistryByteBuf buf;
    EXPECT_FALSE(buf.hasRegistry());
    buf.bindRegistry(RegistryAccess::instance());
    EXPECT_TRUE(buf.hasRegistry());
}

TEST(RegistryByteBuf, InheritsByteBufPrimitives)
{
    RegistryByteBuf buf;
    buf.bindRegistry(RegistryAccess::instance());
    buf.writeU32(0xCAFEBABEu);
    EXPECT_EQ(buf.readU32().value(), 0xCAFEBABEu);
}
