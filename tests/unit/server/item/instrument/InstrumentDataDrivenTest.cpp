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

// 乐器（instrument）数据驱动体系测试：
//   - 内置八种山羊角乐器的注册顺序、音效映射、时长与距离换算；
//   - 数据包 instrument/ 定义与 tags/instrument 标签的加载；
//   - 山羊角物品按 instrument 组件解析乐器、吹奏播放与冷却；
//   - 战利品表 set_instrument 从标签随机抽取乐器。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/item/instrument/InstrumentTags.hpp"
#include "common/item/items/special/GoatHornItem.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/SetInstrumentFunction.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/item/instrument/InstrumentLoader.hpp"
#include "server/item/instrument/InstrumentTagLoader.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace mc;

using mc::item::instrument::InstrumentDefinition;
using mc::item::instrument::InstrumentRegistry;
using mc::item::instrument::InstrumentTags;

namespace {

constexpr const char* PONDER_GOAT_HORN = "minecraft:ponder_goat_horn";

/// 记录音效播放请求的测试世界
class RecordingSoundWorld final : public mc::test::BaseTestWorld {
public:
    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSound = soundEventId;
        m_lastCategory = category;
        m_lastVolume = volume;
        m_lastPitch = pitch;
        m_playedSounds.push_back(soundEventId);
    }

    [[nodiscard]] const ResourceLocation& lastSound() const
    {
        return m_lastSound;
    }

    [[nodiscard]] sound::SoundCategory lastCategory() const
    {
        return m_lastCategory;
    }

    [[nodiscard]] f32 lastVolume() const
    {
        return m_lastVolume;
    }

    [[nodiscard]] f32 lastPitch() const
    {
        return m_lastPitch;
    }

    [[nodiscard]] const std::vector<ResourceLocation>& playedSounds() const
    {
        return m_playedSounds;
    }

    void reset()
    {
        m_playedSounds.clear();
    }

private:
    ResourceLocation m_lastSound;
    sound::SoundCategory m_lastCategory = sound::SoundCategory::Master;
    f32 m_lastVolume = 0.0f;
    f32 m_lastPitch = 0.0f;
    std::vector<ResourceLocation> m_playedSounds;
};

} // namespace

class InstrumentDataDrivenTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::ensureInitialized();

        // 与生产启动顺序一致：清空 → 内置定义 → 内置标签占位（数据包在此之上覆盖/追加）
        InstrumentRegistry::instance().clear();
        InstrumentRegistry::instance().initialize();
        item::instrument::initializeVanillaInstrumentTags();
    }

    void TearDown() override
    {
        InstrumentRegistry::instance().clear();
        InstrumentRegistry::instance().initialize();
        item::instrument::initializeVanillaInstrumentTags();
    }
};

// ============================================================================
// 内置乐器
// ============================================================================

TEST_F(InstrumentDataDrivenTest, BuiltinGoatHornsRegisteredInVanillaOrder)
{
    const auto all = InstrumentRegistry::instance().getAll();
    ASSERT_EQ(all.size(), 8u);

    const std::vector<std::string> expectedIds = {"minecraft:admire_goat_horn",
        "minecraft:call_goat_horn",
        "minecraft:dream_goat_horn",
        "minecraft:feel_goat_horn",
        "minecraft:ponder_goat_horn",
        "minecraft:seek_goat_horn",
        "minecraft:sing_goat_horn",
        "minecraft:yearn_goat_horn"};
    for (std::size_t i = 0; i < expectedIds.size(); ++i) {
        EXPECT_EQ(all[i].first, ResourceLocation(expectedIds[i])) << "wrong instrument at index " << i;
    }
}

TEST_F(InstrumentDataDrivenTest, BuiltinGoatHornParametersMatchVanilla)
{
    const InstrumentDefinition* ponder = InstrumentRegistry::instance().get(ResourceLocation(PONDER_GOAT_HORN));
    ASSERT_NE(ponder, nullptr);
    EXPECT_EQ(ponder->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_0);
    EXPECT_FLOAT_EQ(ponder->useDuration(), 7.0f);
    EXPECT_EQ(ponder->useDurationTicks(), 140);
    EXPECT_FLOAT_EQ(ponder->range(), 256.0f);
    EXPECT_FLOAT_EQ(ponder->volume(), 16.0f);

    // 音色序号映射：admire=4、call=5、dream=7、feel=3、ponder=0、seek=2、sing=1、yearn=6
    const InstrumentDefinition* dream =
        InstrumentRegistry::instance().get(ResourceLocation("minecraft:dream_goat_horn"));
    ASSERT_NE(dream, nullptr);
    EXPECT_EQ(dream->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_7);
    const InstrumentDefinition* feel = InstrumentRegistry::instance().get(ResourceLocation("minecraft:feel_goat_horn"));
    ASSERT_NE(feel, nullptr);
    EXPECT_EQ(feel->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_3);
}

TEST_F(InstrumentDataDrivenTest, InitializeIsIdempotentAndKeepsRegisteredEntries)
{
    InstrumentRegistry::instance().initialize();
    EXPECT_EQ(InstrumentRegistry::instance().size(), 8u);

    // 数据包已登记的同名条目不得被内置定义覆盖
    InstrumentRegistry::instance().registerInstrument(ResourceLocation(PONDER_GOAT_HORN),
        std::make_unique<InstrumentDefinition>(SoundEvents::ITEM_GOAT_HORN_SOUND_7, 3.0f, 64.0f, nullptr));
    InstrumentRegistry::instance().initialize();

    const InstrumentDefinition* ponder = InstrumentRegistry::instance().get(ResourceLocation(PONDER_GOAT_HORN));
    ASSERT_NE(ponder, nullptr);
    EXPECT_EQ(ponder->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_7);
    EXPECT_EQ(ponder->useDurationTicks(), 60);
    EXPECT_FLOAT_EQ(ponder->volume(), 4.0f);
}

// ============================================================================
// 山羊角物品
// ============================================================================

TEST_F(InstrumentDataDrivenTest, GoatHornWithoutInstrumentComponentCannotBeUsed)
{
    ItemStack stack(*Items::GOAT_HORN, 1);
    EXPECT_EQ(item::items::GoatHornItem::getInstrument(stack), nullptr);
    EXPECT_EQ(Items::GOAT_HORN->getUseDuration(stack), 0);

    RecordingSoundWorld world;
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world);
    player.getHeldItem(Hand::MainHand) = stack;

    const auto result = Items::GOAT_HORN->onItemRightClick(world, player, Hand::MainHand);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(world.playedSounds().empty());
}

TEST_F(InstrumentDataDrivenTest, GoatHornPlaysInstrumentSoundAndAppliesCooldown)
{
    const InstrumentDefinition* ponder = InstrumentRegistry::instance().get(ResourceLocation(PONDER_GOAT_HORN));
    ASSERT_NE(ponder, nullptr);

    ItemStack stack = item::items::GoatHornItem::createStack(*ponder);
    EXPECT_EQ(stack.getInstrument(), std::string(PONDER_GOAT_HORN));
    EXPECT_EQ(Items::GOAT_HORN->getUseDuration(stack), 140);

    RecordingSoundWorld world;
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world);
    player.setPosition(10.0f, 64.0f, 20.0f);
    player.getHeldItem(Hand::MainHand) = stack;

    const auto result = Items::GOAT_HORN->onItemRightClick(world, player, Hand::MainHand);
    EXPECT_TRUE(result.isSuccess());

    ASSERT_EQ(world.playedSounds().size(), 1u);
    EXPECT_EQ(world.lastSound(), SoundEvents::ITEM_GOAT_HORN_SOUND_0);
    EXPECT_EQ(world.lastCategory(), sound::SoundCategory::Records);
    EXPECT_FLOAT_EQ(world.lastVolume(), 16.0f);
    EXPECT_FLOAT_EQ(world.lastPitch(), 1.0f);

    // 冷却期间再次吹奏失败且不重复播放
    const auto second = Items::GOAT_HORN->onItemRightClick(world, player, Hand::MainHand);
    EXPECT_FALSE(second.isSuccess());
    EXPECT_EQ(world.playedSounds().size(), 1u);
}

// ============================================================================
// 数据包加载
// ============================================================================

TEST_F(InstrumentDataDrivenTest, LoadsInstrumentDefinitionsAndTagsFromDataPack)
{
    const auto dataPackDir = GameDirectory::defaultDirectory().dataPacksDir();
    if (!std::filesystem::exists(dataPackDir)) {
        GTEST_SKIP() << "vanilla datapack not found: " << dataPackDir.string();
    }
    resource::DataPackRepository repo;
    const auto scanResult = repo.scanDirectory(dataPackDir);
    ASSERT_TRUE(scanResult.success()) << scanResult.error().message();

    const auto instrumentResult = server::item::InstrumentLoader::loadFromDataPackRepository(repo);
    ASSERT_TRUE(instrumentResult.success()) << instrumentResult.error().message();
    EXPECT_EQ(instrumentResult.value(), 8u);

    // 数据包定义覆盖内置条目：ponder 仍映射 sound.0，yearn 映射 sound.6
    const InstrumentDefinition* ponder = InstrumentRegistry::instance().get(ResourceLocation(PONDER_GOAT_HORN));
    ASSERT_NE(ponder, nullptr);
    EXPECT_EQ(ponder->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_0);
    const InstrumentDefinition* yearn =
        InstrumentRegistry::instance().get(ResourceLocation("minecraft:yearn_goat_horn"));
    ASSERT_NE(yearn, nullptr);
    EXPECT_EQ(yearn->soundEvent(), SoundEvents::ITEM_GOAT_HORN_SOUND_6);

    const auto tagResult = server::item::InstrumentTagLoader::loadFromDataPackRepository(repo);
    ASSERT_TRUE(tagResult.success()) << tagResult.error().message();

    auto& tags = InstrumentTags::instance();
    ASSERT_NE(tags.getTag(ResourceLocation("minecraft:regular_goat_horns")), nullptr);
    ASSERT_NE(tags.getTag(ResourceLocation("minecraft:screaming_goat_horns")), nullptr);
    const auto& allGoatHorns = tags.registerTag(ResourceLocation("minecraft:goat_horns"));
    EXPECT_EQ(allGoatHorns.size(), 8u) << "goat_horns should reference all eight instruments";

    const auto& regular = tags.registerTag(ResourceLocation("minecraft:regular_goat_horns"));
    EXPECT_TRUE(regular.contains(ResourceLocation(PONDER_GOAT_HORN)));
    EXPECT_TRUE(regular.contains(ResourceLocation("minecraft:feel_goat_horn")));
    EXPECT_FALSE(regular.contains(ResourceLocation("minecraft:admire_goat_horn")));

    const auto& screaming = tags.registerTag(ResourceLocation("minecraft:screaming_goat_horns"));
    EXPECT_TRUE(screaming.contains(ResourceLocation("minecraft:admire_goat_horn")));
    EXPECT_TRUE(screaming.contains(ResourceLocation("minecraft:dream_goat_horn")));
    EXPECT_FALSE(screaming.contains(ResourceLocation(PONDER_GOAT_HORN)));
}

// ============================================================================
// 战利品表 set_instrument
// ============================================================================

TEST_F(InstrumentDataDrivenTest, SetInstrumentFunctionDrawsFromInstrumentTag)
{
    // 内置标签占位为空，此处按 vanilla regular_goat_horns 内容填充后验证抽取范围
    auto& regular = InstrumentTags::instance().registerTag(ResourceLocation("minecraft:regular_goat_horns"));
    regular.clear();
    regular.add(ResourceLocation(PONDER_GOAT_HORN));
    regular.add(ResourceLocation("minecraft:sing_goat_horn"));

    RecordingSoundWorld world;
    loot::SetInstrumentFunction function(ResourceLocation("minecraft:regular_goat_horns"));
    math::Random random(1234);
    auto context = loot::LootContextBuilder(world).withRandom(random).build();

    ItemStack stack(*Items::GOAT_HORN, 1);
    const ItemStack result = function.apply(stack, *context);
    const std::string instrumentId = result.getInstrument();
    EXPECT_TRUE(instrumentId == PONDER_GOAT_HORN || instrumentId == "minecraft:sing_goat_horn");
}

TEST_F(InstrumentDataDrivenTest, SetInstrumentFunctionWithUnknownTagLeavesStackUntouched)
{
    RecordingSoundWorld world;
    loot::SetInstrumentFunction function(ResourceLocation("minecraft:unknown_goat_horns"));
    math::Random random(42);
    auto context = loot::LootContextBuilder(world).withRandom(random).build();

    ItemStack stack(*Items::GOAT_HORN, 1);
    const ItemStack result = function.apply(stack, *context);
    EXPECT_TRUE(result.getInstrument().empty());
}
