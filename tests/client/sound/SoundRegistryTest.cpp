#include <gtest/gtest.h>

#include "client/sound/resource/SoundRegistry.hpp"

using namespace mc;
using namespace mc::client::sound;

namespace {

SoundEventDefinition makeEvent(const std::string& id, const std::string& soundId, bool replace = false)
{
    SoundEventDefinition definition{ResourceLocation(id)};
    definition.replace = replace;
    definition.sounds.emplace_back(ResourceLocation(soundId));
    return definition;
}

} // namespace

TEST(SoundRegistryTest, MergeSelfIsNoopAndDoesNotThrow)
{
    SoundRegistry registry;
    registry.registerSoundEvent(makeEvent("minecraft:block.stone.break", "minecraft:dig/stone1"));

    EXPECT_NO_THROW(registry.merge(registry));
    EXPECT_EQ(registry.getSoundEventCount(), 1u);

    const auto* eventDef = registry.getSoundEvent(ResourceLocation("minecraft:block.stone.break"));
    ASSERT_NE(eventDef, nullptr);
    ASSERT_EQ(eventDef->sounds.size(), 1u);
    EXPECT_EQ(eventDef->sounds[0].location, ResourceLocation("minecraft:dig/stone1"));
}

TEST(SoundRegistryTest, MergeDifferentRegistriesAvoidsDeadlockAndMergesData)
{
    SoundRegistry base;
    SoundRegistry other;

    base.registerSoundEvent(makeEvent("minecraft:block.stone.break", "minecraft:dig/stone1"));
    other.registerSoundEvent(makeEvent("minecraft:block.stone.break", "minecraft:dig/stone2"));
    other.registerSoundEvent(makeEvent("minecraft:block.grass.break", "minecraft:dig/grass1"));

    EXPECT_NO_THROW(base.merge(other));

    const auto* stoneDef = base.getSoundEvent(ResourceLocation("minecraft:block.stone.break"));
    ASSERT_NE(stoneDef, nullptr);
    EXPECT_EQ(stoneDef->sounds.size(), 2u);

    const auto* grassDef = base.getSoundEvent(ResourceLocation("minecraft:block.grass.break"));
    ASSERT_NE(grassDef, nullptr);
    EXPECT_EQ(grassDef->sounds.size(), 1u);
}
