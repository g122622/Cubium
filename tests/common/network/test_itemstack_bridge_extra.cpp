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

// Phase3 阶段3：ItemStack wire codec + ItemStackBridge 边界单元测试。
// 扩既有 ItemStackComponentRoundTripTest：补错误路径与最小字节布局。
//
// 线格式（readItemStack/writeItemStack）：VarInt(count)——count<=0 即空（仅 1 字节 0x00）；
// 否则 VarInt(itemId) + VarInt(patchLen) + patchLen 字节。patchLen<0 错；patchLen>可用字节
// readBytes 错（截断）。bridge fromItemStackView：itemId==0||count<=0 返空栈（非错）；
// itemId 非零但未注册返 InvalidItem 错。

#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/registry/RegistryAccess.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace mc;
using namespace mc::network;
using namespace mc::network::buffer;
using namespace mc::network::backend::java::codecs;
using namespace mc::network::ir;
using namespace mc::network::ir::play;

namespace {

/// 构造一个绑定默认注册表的 RegistryByteBuf
RegistryByteBuf makeBuf()
{
    RegistryByteBuf buf;
    buf.bindRegistry(RegistryAccess::instance());
    return buf;
}

} // namespace

class ItemStackBridgeExtraTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_sword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
        ASSERT_NE(m_sword, nullptr);
    }

    const Item* m_sword = nullptr;
};

// ============================================================================
// bridge fromItemStackView 边界
// ============================================================================

TEST_F(ItemStackBridgeExtraTest, UnknownItemIdReturnsError)
{
    // 非零但未注册的 itemId → InvalidItem 错误（不返空栈）
    ItemStackView view{};
    view.itemId = 999999u; // 远超 vanilla 注册表
    view.count = 1;
    auto r = fromItemStackView(view);
    ASSERT_FALSE(r.success());
}

TEST_F(ItemStackBridgeExtraTest, NonZeroItemIdButNonPositiveCountReturnsEmpty)
{
    // bridge 防御：itemId!=0 但 count<=0 → 返空栈（非错）。
    // 线上 count<=0 在 wire 层已被编码为空（0x00），不会带 itemId；此处直接测 bridge 守卫。
    ItemStackView view{};
    view.itemId = m_sword->itemId();
    view.count = 0;
    auto r = fromItemStackView(view);
    ASSERT_TRUE(r.success());
    EXPECT_TRUE(r.value().isEmpty());
    EXPECT_EQ(r.value().getItem(), nullptr);

    view.count = -3;
    auto r2 = fromItemStackView(view);
    ASSERT_TRUE(r2.success());
    EXPECT_TRUE(r2.value().isEmpty());
}

TEST_F(ItemStackBridgeExtraTest, EmptyStackRoundTripsToEmpty)
{
    // 空栈 → view（itemId=0,count=0,patch 空）→ wire（0x00 单字节）→ 解码回空 view → 空栈
    ItemStack empty;
    const auto view = toItemStackView(empty);
    EXPECT_EQ(view.itemId, 0u);
    EXPECT_EQ(view.count, 0);
    EXPECT_TRUE(view.componentsPatch.empty());

    auto buf = makeBuf();
    play_detail::writeItemStack(buf, view);
    ASSERT_EQ(buf.size(), 1u); // 仅 VarInt(0)
    EXPECT_EQ(buf.data()[0], 0x00);

    RegistryByteBuf readBuf(buf.data(), buf.size(), RegistryAccess::instance());
    auto dec = play_detail::readItemStack(readBuf);
    ASSERT_TRUE(dec.success());
    EXPECT_EQ(dec.value().itemId, 0u);
    EXPECT_EQ(dec.value().count, 0);
    EXPECT_TRUE(dec.value().componentsPatch.empty());

    auto restored = fromItemStackView(dec.value());
    ASSERT_TRUE(restored.success());
    EXPECT_TRUE(restored.value().isEmpty());
    EXPECT_EQ(restored.value().getItem(), nullptr);
}

// ============================================================================
// wire codec 最小字节布局 + 截断错误
// ============================================================================

TEST_F(ItemStackBridgeExtraTest, EmptyPatchProducesMinimalBytes)
{
    // 合法 itemId + count>0 + patchLen=0：
    // wire = VarInt(count) + VarInt(itemId) + VarInt(0)
    ItemStackView view{};
    view.itemId = m_sword->itemId();
    view.count = 1;
    // componentsPatch 空 → patchLen=0

    auto buf = makeBuf();
    play_detail::writeItemStack(buf, view);
    // count=1(1B) + itemId(VarInt,≥1B) + patchLen=0(1B)；不校验确切长度，校验可往返
    ASSERT_GE(buf.size(), 3u);

    RegistryByteBuf readBuf(buf.data(), buf.size(), RegistryAccess::instance());
    auto dec = play_detail::readItemStack(readBuf);
    ASSERT_TRUE(dec.success());
    EXPECT_EQ(dec.value().itemId, m_sword->itemId());
    EXPECT_EQ(dec.value().count, 1);
    EXPECT_TRUE(dec.value().componentsPatch.empty());

    auto restored = fromItemStackView(dec.value());
    ASSERT_TRUE(restored.success());
    EXPECT_EQ(restored.value().getItem(), m_sword);
    EXPECT_EQ(restored.value().getCount(), 1);
}

TEST_F(ItemStackBridgeExtraTest, TruncatedPatchReturnsError)
{
    // 声明 patchLen=5 但只跟 2 字节 → readBytes 截断错。
    auto buf = makeBuf();
    buf.writeVarInt(1);                                    // count
    buf.writeVarInt(static_cast<i32>(m_sword->itemId()));  // itemId
    buf.writeVarInt(5);                                    // patchLen=5（声明）
    buf.writeBytes(std::vector<u8>{0xAA, 0xBB}.data(), 2); // 实际只 2 字节

    RegistryByteBuf readBuf(buf.data(), buf.size(), RegistryAccess::instance());
    auto dec = play_detail::readItemStack(readBuf);
    ASSERT_FALSE(dec.success());
}

TEST_F(ItemStackBridgeExtraTest, NegativePatchLengthReturnsError)
{
    // patchLen=-1 → 显式 InvalidData 错（非截断）。
    auto buf = makeBuf();
    buf.writeVarInt(1);                                   // count
    buf.writeVarInt(static_cast<i32>(m_sword->itemId())); // itemId
    // VarInt(-1) = 0xFF 0xFF 0xFF 0xFF 0x0F（5 字节）
    buf.writeVarInt(-1);

    RegistryByteBuf readBuf(buf.data(), buf.size(), RegistryAccess::instance());
    auto dec = play_detail::readItemStack(readBuf);
    ASSERT_FALSE(dec.success());
}
