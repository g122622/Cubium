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
