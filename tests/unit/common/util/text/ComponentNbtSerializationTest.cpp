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
 * @file ComponentNbtSerializationTest.cpp
 * @brief ITextComponent → 1.21.11 Component NBT wire 字节序列化测试
 *
 * 覆盖 componentToNbtBytes 的两条 contents 分支：
 * - 纯文本（StringTextComponent）可折叠 → StringTag(0x08 + U16 大端长度 + UTF8)
 * - 翻译（TranslationTextComponent）不可折叠 → CompoundTag(0x0A) 含 translate/with/style
 * 以及 style（红字）写入与 extra（siblings）递归。
 *
 * 解析回读用 NbtIo::readRootCompound（对齐 Java FriendlyByteBuf.readNbt = readAnyTag）。
 */

#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/TranslationTextComponent.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::text;

namespace {

/// 把 wire 字节按根 NBT 解析为 compound_tag（StringTag 输入不适用）。
std::unique_ptr<nbt::tags::compound_tag> parseCompound(const std::vector<u8>& bytes)
{
    network::buffer::ByteBuf buf(bytes.data(), bytes.size());
    auto result = network::buffer::nbt_io::readRootCompound(buf);
    if (result.failed()) {
        return nullptr;
    }
    return std::move(result).value();
}

/// 从 compound_tag 取字符串键（不存在或类型不符返回空串）。
std::string getString(const nbt::tags::compound_tag& comp, const std::string& key)
{
    auto it = comp.value.find(key);
    if (it == comp.value.end()) {
        return {};
    }
    const auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    return str != nullptr ? str->value : std::string{};
}

} // namespace

// ============================================================================
// 纯文本：可折叠为 StringTag
// ============================================================================

TEST(ComponentNbtSerializationTest, PlainTextCollapsesToStringTag)
{
    StringTextComponent text("Hello");
    const auto bytes = componentToNbtBytes(&text);

    ASSERT_EQ(bytes.size(), 3u + 5u);
    EXPECT_EQ(bytes[0], 0x08); // StringTag
    EXPECT_EQ(bytes[1], 0x00); // 长度高字节
    EXPECT_EQ(bytes[2], 0x05); // 长度低字节
    EXPECT_EQ(std::string(bytes.begin() + 3, bytes.end()), "Hello");
}

TEST(ComponentNbtSerializationTest, NullComponentYieldsEmptyStringTag)
{
    const auto bytes = componentToNbtBytes(nullptr);
    ASSERT_EQ(bytes.size(), 3u);
    EXPECT_EQ(bytes[0], 0x08);
    EXPECT_EQ(bytes[1], 0x00);
    EXPECT_EQ(bytes[2], 0x00);
}

TEST(ComponentNbtSerializationTest, StyledPlainTextIsNotCollapsed)
{
    // 带 style 的纯文本不可折叠（对齐 tryCollapseToString），走 CompoundTag。
    StringTextComponent text("Red");
    Style style;
    style.setColor(TextFormatting::Red);
    text.setStyle(style);

    const auto bytes = componentToNbtBytes(&text);
    ASSERT_FALSE(bytes.empty());
    EXPECT_EQ(bytes[0], 0x0A); // CompoundTag

    auto comp = parseCompound(bytes);
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(getString(*comp, "text"), "Red");
    EXPECT_EQ(getString(*comp, "color"), "red");
}

// ============================================================================
// 翻译组件：translate + with + style
// ============================================================================

TEST(ComponentNbtSerializationTest, TranslatableWithArgsAndStyle)
{
    // 复刻 build.tooHigh：Component.translatable("build.tooHigh", 319).withStyle(RED)
    TranslationTextComponent msg("build.tooHigh");
    msg.addParam(std::make_unique<StringTextComponent>("319"));
    Style style;
    style.setColor(TextFormatting::Red);
    msg.setStyle(style);

    const auto bytes = componentToNbtBytes(&msg);
    ASSERT_FALSE(bytes.empty());
    EXPECT_EQ(bytes[0], 0x0A); // CompoundTag（翻译组件不可折叠）

    auto comp = parseCompound(bytes);
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(getString(*comp, "translate"), "build.tooHigh");
    EXPECT_EQ(getString(*comp, "color"), "red");

    // with 是 ListTag(CompoundTag)，首元素 {text:"319"}。
    // 注意：解析回读得到的是 compound_list_tag（list_tag 的派生），非 tag_list_tag——
    // 两者都是 list_tag 的兄弟派生类，故统一按基类 list_tag 取元素。
    auto withIt = comp->value.find("with");
    ASSERT_NE(withIt, comp->value.end());
    const auto* withList = dynamic_cast<const nbt::tags::list_tag*>(withIt->second.get());
    ASSERT_NE(withList, nullptr);
    ASSERT_EQ(withList->size(), 1u);
    auto arg = (*withList)[0];
    const auto* argComp = dynamic_cast<const nbt::tags::compound_tag*>(arg.get());
    ASSERT_NE(argComp, nullptr);
    EXPECT_EQ(getString(*argComp, "text"), "319");
}

TEST(ComponentNbtSerializationTest, TranslatableWithoutArgsOmitsWith)
{
    TranslationTextComponent msg("entity.minecraft.ender_dragon");
    const auto bytes = componentToNbtBytes(&msg);

    auto comp = parseCompound(bytes);
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(getString(*comp, "translate"), "entity.minecraft.ender_dragon");
    // 无参数时不写 "with" 键（对齐 vanilla optionalFieldOf("with")）
    EXPECT_EQ(comp->value.find("with"), comp->value.end());
}

TEST(ComponentNbtSerializationTest, TranslatableWithMultipleArgs)
{
    TranslationTextComponent msg("commands.give.success.single");
    msg.addParam(std::make_unique<StringTextComponent>("64"));
    msg.addParam(std::make_unique<StringTextComponent>("Stone"));
    msg.addParam(std::make_unique<StringTextComponent>("Steve"));

    auto comp = parseCompound(componentToNbtBytes(&msg));
    ASSERT_NE(comp, nullptr);
    auto withIt = comp->value.find("with");
    ASSERT_NE(withIt, comp->value.end());
    const auto* withList = dynamic_cast<const nbt::tags::list_tag*>(withIt->second.get());
    ASSERT_NE(withList, nullptr);
    ASSERT_EQ(withList->size(), 3u);
}

// ============================================================================
// extra（siblings）递归
// ============================================================================

TEST(ComponentNbtSerializationTest, SiblingsSerializedAsExtra)
{
    TranslationTextComponent msg("key");
    Style style;
    style.setColor(TextFormatting::Red);
    msg.setStyle(style);
    msg.append(std::make_unique<StringTextComponent>("tail"));

    auto comp = parseCompound(componentToNbtBytes(&msg));
    ASSERT_NE(comp, nullptr);
    auto extraIt = comp->value.find("extra");
    ASSERT_NE(extraIt, comp->value.end());
    const auto* extraList = dynamic_cast<const nbt::tags::list_tag*>(extraIt->second.get());
    ASSERT_NE(extraList, nullptr);
    ASSERT_EQ(extraList->size(), 1u);
    auto sibling = (*extraList)[0];
    const auto* siblingComp = dynamic_cast<const nbt::tags::compound_tag*>(sibling.get());
    ASSERT_NE(siblingComp, nullptr);
    EXPECT_EQ(getString(*siblingComp, "text"), "tail");
}
