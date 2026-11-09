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

#include <gtest/gtest.h>

#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentPayloadCodec.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <memory>
#include <string>

using namespace mc;
using nbt::TagId;
using nbt::tags::compound_tag;
using nbt::tags::string_tag;

namespace {

/// @brief 构造一份内联乐器定义（不依赖 InstrumentRegistry 是否已加载）
item::instrument::InstrumentDefinition _makeInlineDefinition()
{
    auto description = text::ITextComponent::fromJson(nlohmann::json{{"text", "Custom Horn"}});
    return item::instrument::InstrumentDefinition(
        ResourceLocation("minecraft:item.goat_horn.sound.0"), 7.0f, 256.0f, std::move(description));
}

/// @brief 在组件补丁里找到 instrument 条目；没有则返回 nullptr
const item::component::DataComponentPayload* _findInstrumentPayload(
    const item::component::DataComponentPatch& patch)
{
    const i32 wantedId = item::component::componentTypeId(item::component::DataComponentType::Instrument);
    for (const auto& entry : patch.added()) {
        if (entry.typeId == wantedId) {
            return &entry.value;
        }
    }
    return nullptr;
}

} // namespace

// ============================================================================
// instrument 组件的两种 Holder 形态
// ============================================================================

// 内联定义必须序列化成 compound（含 sound_event/sound_id 等字段），而不是裸字符串。
TEST(InstrumentPayloadInlineTest, InlineDefinitionSerializesAsCompound)
{
    item::component::InstrumentPayload payload{};
    payload.inlineDefinition = _makeInlineDefinition();

    const auto tag = item::component::detail::payloadToNbt(item::component::DataComponentType::Instrument, payload);
    ASSERT_NE(tag, nullptr);
    ASSERT_EQ(tag->id(), TagId::Compound) << "inline definition must not degrade to a bare string";

    const auto& compound = dynamic_cast<const compound_tag&>(*tag);

    // sound_event 是 {sound_id: "..."} 复合对象，不是裸字符串。
    const auto soundIt = compound.value.find("sound_event");
    ASSERT_NE(soundIt, compound.value.end());
    ASSERT_EQ(soundIt->second->id(), TagId::Compound);
    const auto& soundCompound = dynamic_cast<const compound_tag&>(*soundIt->second);
    const auto idIt = soundCompound.value.find("sound_id");
    ASSERT_NE(idIt, soundCompound.value.end());
    EXPECT_EQ(dynamic_cast<const string_tag&>(*idIt->second).value, "minecraft:item.goat_horn.sound.0");

    // use_duration / range 是裸 float。
    EXPECT_NE(compound.value.find("use_duration"), compound.value.end());
    EXPECT_NE(compound.value.find("range"), compound.value.end());
}

// 内联定义经 NBT 往返后四个字段都要保留。
TEST(InstrumentPayloadInlineTest, InlineDefinitionSurvivesNbtRoundTrip)
{
    item::component::DataComponentPatch patch;
    item::component::InstrumentPayload payload{};
    payload.inlineDefinition = _makeInlineDefinition();
    patch.add(item::component::DataComponentType::Instrument,
        item::component::DataComponentPayload{
            std::in_place_index<item::component::ComponentPayloadIndex::Instrument>, std::move(payload)});

    compound_tag out;
    item::component::writePatchToNbt(out, patch);
    const item::component::DataComponentPatch restored = item::component::readPatchFromNbt(out);

    const auto* found = _findInstrumentPayload(restored);
    ASSERT_NE(found, nullptr) << "Instrument component lost in NBT round trip";

    const auto* restoredPayload = std::get_if<item::component::InstrumentPayload>(found);
    ASSERT_NE(restoredPayload, nullptr);
    ASSERT_TRUE(restoredPayload->inlineDefinition.has_value())
        << "inline definition must survive the round trip instead of degrading to an empty reference";

    const auto& definition = restoredPayload->inlineDefinition.value();
    EXPECT_EQ(definition.soundEvent().toString(), "minecraft:item.goat_horn.sound.0");
    EXPECT_FLOAT_EQ(definition.useDuration(), 7.0f);
    EXPECT_FLOAT_EQ(definition.range(), 256.0f);
    ASSERT_NE(definition.description(), nullptr) << "description must not be dropped";
}

// 只给注册名的字符串形态仍要能读回，并保持「无内联定义」。
TEST(InstrumentPayloadInlineTest, PlainRegistryIdStringStaysReferenceOnly)
{
    compound_tag out;
    out.put("minecraft:instrument", std::string("minecraft:ponder_goat_horn"));

    const item::component::DataComponentPatch restored = item::component::readPatchFromNbt(out);
    const auto* found = _findInstrumentPayload(restored);
    ASSERT_NE(found, nullptr);

    const auto* restoredPayload = std::get_if<item::component::InstrumentPayload>(found);
    ASSERT_NE(restoredPayload, nullptr);
    EXPECT_EQ(restoredPayload->instrumentId, "minecraft:ponder_goat_horn");
    EXPECT_FALSE(restoredPayload->inlineDefinition.has_value())
        << "a bare id must not fabricate an inline definition";
}