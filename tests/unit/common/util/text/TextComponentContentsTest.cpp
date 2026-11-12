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
 * @file TextComponentContentsTest.cpp
 * @brief 文本组件 7 种 contents 的 JSON 解析与 Component NBT 往返测试
 *
 * 覆盖原版 1.21.11 的全部 contents：text / translate / score / selector / keybind /
 * nbt / object。重点是：
 * - JSON → 具体组件类型（含字段值）；
 * - 组件 → Component NBT → 组件 的往返保持一致；
 * - 不可信输入（缺字段/类型错误/未知类型）安全降级，不崩溃。
 */

#include "common/util/json/JsonRead.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/util/text/contents/KeybindTextComponent.hpp"
#include "common/util/text/contents/NbtTextComponent.hpp"
#include "common/util/text/contents/ObjectTextComponent.hpp"
#include "common/util/text/contents/ScoreTextComponent.hpp"
#include "common/util/text/contents/SelectorTextComponent.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::text;

namespace {

std::unique_ptr<ITextComponent> parseComponent(const std::string& jsonText)
{
    auto parsed = util::json::parse(jsonText);
    if (parsed.failed()) {
        return nullptr;
    }
    return ITextComponent::fromJson(parsed.value());
}

} // namespace

// ============================================================================
// JSON 解析：7 种 contents
// ============================================================================

TEST(TextComponentContentsTest, FromJsonText)
{
    auto component = parseComponent(R"({"text":"Hello","color":"red"})");
    ASSERT_NE(component, nullptr);
    EXPECT_NE(dynamic_cast<StringTextComponent*>(component.get()), nullptr);
    EXPECT_EQ(component->getUnformattedText(), "Hello");
}

TEST(TextComponentContentsTest, FromJsonTranslateWithParams)
{
    auto component = parseComponent(R"({"translate":"chat.type.text","with":[{"text":"Steve"},{"text":"Hi"}]})");
    ASSERT_NE(component, nullptr);
    auto* translation = dynamic_cast<TranslationTextComponent*>(component.get());
    ASSERT_NE(translation, nullptr);
    EXPECT_EQ(translation->getKey(), "chat.type.text");
    ASSERT_EQ(translation->getParams().size(), 2u);
}

TEST(TextComponentContentsTest, FromJsonScore)
{
    auto component = parseComponent(R"({"score":{"name":"Steve","objective":"obj"}})");
    ASSERT_NE(component, nullptr);
    auto* score = dynamic_cast<ScoreTextComponent*>(component.get());
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(score->name(), "Steve");
    EXPECT_EQ(score->objective(), "obj");
}

TEST(TextComponentContentsTest, FromJsonSelectorWithSeparator)
{
    auto component = parseComponent(R"({"selector":"@a","separator":{"text":", "}})");
    ASSERT_NE(component, nullptr);
    auto* selector = dynamic_cast<SelectorTextComponent*>(component.get());
    ASSERT_NE(selector, nullptr);
    EXPECT_EQ(selector->selector(), "@a");
    ASSERT_NE(selector->separator(), nullptr);
    EXPECT_EQ(selector->separator()->getUnformattedText(), ", ");
}

TEST(TextComponentContentsTest, FromJsonKeybind)
{
    auto component = parseComponent(R"({"keybind":"key.jump"})");
    ASSERT_NE(component, nullptr);
    auto* keybind = dynamic_cast<KeybindTextComponent*>(component.get());
    ASSERT_NE(keybind, nullptr);
    EXPECT_EQ(keybind->key(), "key.jump");
}

TEST(TextComponentContentsTest, FromJsonNbtEntitySource)
{
    auto component = parseComponent(R"({"nbt":"Health","interpret":true,"entity":"@p"})");
    ASSERT_NE(component, nullptr);
    auto* nbtComponent = dynamic_cast<NbtTextComponent*>(component.get());
    ASSERT_NE(nbtComponent, nullptr);
    EXPECT_EQ(nbtComponent->path(), "Health");
    EXPECT_TRUE(nbtComponent->isInterpreting());
    EXPECT_EQ(nbtComponent->dataSourceKind(), NbtDataSourceKind::Entity);
    EXPECT_EQ(nbtComponent->dataSourceValue(), "@p");
}

TEST(TextComponentContentsTest, FromJsonObject)
{
    auto component = parseComponent(R"({"object":{"atlas":"minecraft:blocks","sprite":"stone"}})");
    ASSERT_NE(component, nullptr);
    auto* object = dynamic_cast<ObjectTextComponent*>(component.get());
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->object()["sprite"], "stone");
}

TEST(TextComponentContentsTest, FromJsonArray)
{
    auto component = parseComponent(R"([{"text":"a"},{"text":"b"}])");
    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "ab");
}

// ============================================================================
// 不可信输入：缺字段/类型错误时安全降级
// ============================================================================

TEST(TextComponentContentsTest, InvalidScoreFallsBackToEmptyFields)
{
    auto component = parseComponent(R"({"score":{"name":123}})");
    ASSERT_NE(component, nullptr);
    auto* score = dynamic_cast<ScoreTextComponent*>(component.get());
    ASSERT_NE(score, nullptr);
    EXPECT_TRUE(score->name().empty());
    EXPECT_TRUE(score->objective().empty());
}

TEST(TextComponentContentsTest, InvalidNbtWithoutDataSource)
{
    // 缺数据源：仍构造组件，数据源值留空（不崩溃）
    auto component = parseComponent(R"({"nbt":"Health"})");
    ASSERT_NE(component, nullptr);
    auto* nbtComponent = dynamic_cast<NbtTextComponent*>(component.get());
    ASSERT_NE(nbtComponent, nullptr);
    EXPECT_EQ(nbtComponent->path(), "Health");
    EXPECT_TRUE(nbtComponent->dataSourceValue().empty());
}

TEST(TextComponentContentsTest, UnknownContentsFallsBackToText)
{
    auto component = parseComponent(R"({"unknown":"value","text":"fallback"})");
    ASSERT_NE(component, nullptr);
    EXPECT_NE(dynamic_cast<StringTextComponent*>(component.get()), nullptr);
    EXPECT_EQ(component->getUnformattedText(), "fallback");
}

// ============================================================================
// Component NBT 往返
// ============================================================================

TEST(TextComponentContentsTest, NbtRoundTripScore)
{
    ScoreTextComponent original("Steve", "obj");
    const auto bytes = componentToNbtBytes(&original);

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* score = dynamic_cast<ScoreTextComponent*>(restoredComponent.get());
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(score->name(), "Steve");
    EXPECT_EQ(score->objective(), "obj");
}

TEST(TextComponentContentsTest, NbtRoundTripSelector)
{
    SelectorTextComponent original("@a[team=red]");
    original.setSeparator(literal(", "));
    const auto bytes = componentToNbtBytes(&original);

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* selector = dynamic_cast<SelectorTextComponent*>(restoredComponent.get());
    ASSERT_NE(selector, nullptr);
    EXPECT_EQ(selector->selector(), "@a[team=red]");
    ASSERT_NE(selector->separator(), nullptr);
    EXPECT_EQ(selector->separator()->getUnformattedText(), ", ");
}

TEST(TextComponentContentsTest, NbtRoundTripKeybind)
{
    KeybindTextComponent original("key.jump");
    const auto bytes = componentToNbtBytes(&original);

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* keybind = dynamic_cast<KeybindTextComponent*>(restoredComponent.get());
    ASSERT_NE(keybind, nullptr);
    EXPECT_EQ(keybind->key(), "key.jump");
}

TEST(TextComponentContentsTest, NbtRoundTripNbt)
{
    NbtTextComponent original("Inventory[0].id", true, NbtDataSourceKind::Storage, "minecraft:test");
    original.setSeparator(literal(" | "));
    const auto bytes = componentToNbtBytes(&original);

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* nbtComponent = dynamic_cast<NbtTextComponent*>(restoredComponent.get());
    ASSERT_NE(nbtComponent, nullptr);
    EXPECT_EQ(nbtComponent->path(), "Inventory[0].id");
    EXPECT_TRUE(nbtComponent->isInterpreting());
    EXPECT_EQ(nbtComponent->dataSourceKind(), NbtDataSourceKind::Storage);
    EXPECT_EQ(nbtComponent->dataSourceValue(), "minecraft:test");
    ASSERT_NE(nbtComponent->separator(), nullptr);
}

TEST(TextComponentContentsTest, NbtRoundTripObject)
{
    auto object = util::json::parse(R"({"player":{"name":"Steve"},"hat":false})");
    ASSERT_TRUE(object.success());
    ObjectTextComponent original(object.value());
    const auto bytes = componentToNbtBytes(&original);

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* objectComponent = dynamic_cast<ObjectTextComponent*>(restoredComponent.get());
    ASSERT_NE(objectComponent, nullptr);
    // NBT 的布尔是 ByteTag，回读后 JSON 表示为数字 0/1；这里按语义断言而非 JSON 类型。
    EXPECT_EQ(objectComponent->object()["hat"].get<i32>(), 0);
    EXPECT_EQ(objectComponent->object()["player"]["name"], "Steve");
}

TEST(TextComponentContentsTest, NbtSerializesTranslateWithParams)
{
    auto component = translatable("chat.type.text", {"Steve", "Hi"});
    const auto bytes = componentToNbtBytes(component.get());

    auto restored = componentFromNbtBytes(bytes);
    ASSERT_TRUE(restored.success());
    auto restoredComponent = restored.value();
    auto* translation = dynamic_cast<TranslationTextComponent*>(restoredComponent.get());
    ASSERT_NE(translation, nullptr);
    EXPECT_EQ(translation->getKey(), "chat.type.text");
    ASSERT_EQ(translation->getParams().size(), 2u);
    EXPECT_EQ(translation->getParams()[0]->getUnformattedText(), "Steve");
}

// ============================================================================
// 组件工厂
// ============================================================================

TEST(TextComponentContentsTest, FactoryTranslatableEscape)
{
    auto component = translatableEscape("commands.scoreboard.players.get.success", "100%");
    ASSERT_NE(component, nullptr);
    auto* translation = dynamic_cast<TranslationTextComponent*>(component.get());
    ASSERT_NE(translation, nullptr);
    ASSERT_EQ(translation->getParams().size(), 1u);
    EXPECT_EQ(translation->getParams()[0]->getUnformattedText(), "100%%");
}

TEST(TextComponentContentsTest, FactoryTranslatableWithInt)
{
    auto component = translatable("commands.give.success.single", static_cast<i32>(64));
    ASSERT_NE(component, nullptr);
    auto* translation = dynamic_cast<TranslationTextComponent*>(component.get());
    ASSERT_NE(translation, nullptr);
    ASSERT_EQ(translation->getParams().size(), 1u);
    EXPECT_EQ(translation->getParams()[0]->getUnformattedText(), "64");
}
