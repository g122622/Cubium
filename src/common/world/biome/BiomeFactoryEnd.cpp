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
#include "BiomeAmbientSounds.hpp"
#include "BiomeEffects.hpp"
#include "BiomeFactory.hpp"
#include "BiomeGenerationSettings.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

namespace mc {
namespace world {
namespace biome {

namespace BiomeFactory {

namespace {
const BlockState* getBlockState(Block* block)
{
    return block ? &block->defaultState() : nullptr;
}
} // namespace

// ============================================================================
// 末地生物群系工厂函数
// ============================================================================

Biome createTheEnd()
{
    // 末地主岛：末影龙战斗区域
    Biome biome(Biomes::TheEnd, "the_end");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));

    // 视觉效果：雾颜色 10518688 (暗紫色)
    biome.setEffects(BiomeEffects::Builder().fogColor(10518688).waterColor(4159204).waterFogColor(329011).build());

    // 环境音效：默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    biome.setGenerationSettings(BiomeGenerationSettings::createTheEnd());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createSmallEndIslands()
{
    // 小型末地岛屿：外岛的小型岛屿群
    Biome biome(Biomes::SmallEndIslands, "small_end_islands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));

    biome.setEffects(BiomeEffects::Builder().fogColor(10518688).waterColor(4159204).waterFogColor(329011).build());

    // 环境音效：默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    biome.setGenerationSettings(BiomeGenerationSettings::createSmallEndIslands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndMidlands()
{
    // 末地中部：外岛过渡区域
    Biome biome(Biomes::EndMidlands, "end_midlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));

    biome.setEffects(BiomeEffects::Builder().fogColor(10518688).waterColor(4159204).waterFogColor(329011).build());

    // 环境音效：默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    biome.setGenerationSettings(BiomeGenerationSettings::createEndMidlands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndHighlands()
{
    // 末地高地：末地城和紫颂树生成区域
    Biome biome(Biomes::EndHighlands, "end_highlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));

    biome.setEffects(BiomeEffects::Builder().fogColor(10518688).waterColor(4159204).waterFogColor(329011).build());

    // 环境音效：默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    biome.setGenerationSettings(BiomeGenerationSettings::createEndHighlands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndBarrens()
{
    // 末地荒地：空旷区域，无特征
    Biome biome(Biomes::EndBarrens, "end_barrens");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));

    biome.setEffects(BiomeEffects::Builder().fogColor(10518688).waterColor(4159204).waterFogColor(329011).build());

    // 环境音效：默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    biome.setGenerationSettings(BiomeGenerationSettings::createEndBarrens());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

// ============================================================================
// 新生物群系
// ============================================================================

Biome createMeadow()
{
    Biome biome(Biomes::Meadow, "meadow");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.5f, BiomeClimate::TemperatureModifier::None, 0.8f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .grassColor(0x63A948)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createGrove()
{
    Biome biome(Biomes::Grove, "grove");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.2f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Snow, -0.2f, BiomeClimate::TemperatureModifier::None, 0.8f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createSnowySlopes()
{
    Biome biome(Biomes::SnowySlopes, "snowy_slopes");
    biome.setDepth(0.3f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.3f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Snow, -0.3f, BiomeClimate::TemperatureModifier::None, 0.9f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createJaggedPeaks()
{
    Biome biome(Biomes::JaggedPeaks, "jagged_peaks");
    biome.setDepth(1.0f);
    biome.setScale(1.0f);
    biome.setTemperature(-0.7f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Snow, -0.7f, BiomeClimate::TemperatureModifier::None, 0.9f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createFrozenPeaks()
{
    Biome biome(Biomes::FrozenPeaks, "frozen_peaks");
    biome.setDepth(1.0f);
    biome.setScale(1.0f);
    biome.setTemperature(-0.7f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Snow, -0.7f, BiomeClimate::TemperatureModifier::None, 0.9f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createStonyPeaks()
{
    Biome biome(Biomes::StonyPeaks, "stony_peaks");
    biome.setDepth(1.0f);
    biome.setScale(1.0f);
    biome.setTemperature(1.0f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 1.0f, BiomeClimate::TemperatureModifier::None, 0.3f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createDripstoneCaves()
{
    Biome biome(Biomes::DripstoneCaves, "dripstone_caves");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.8f, BiomeClimate::TemperatureModifier::None, 0.4f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createLushCaves()
{
    Biome biome(Biomes::LushCaves, "lush_caves");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.5f, BiomeClimate::TemperatureModifier::None, 0.5f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createLushCaves());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createLushCaves());

    // 繁茂洞穴环境音效：循环音、心境音、随机附加音
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.lush_caves.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.lush_caves.mood"), 6000, 8, 2.0));
    sounds.setAdditionsSound(
        SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.lush_caves.additions"), 0.0111));
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createDeepDark()
{
    Biome biome(Biomes::DeepDark, "deep_dark");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::None, 0.0f, BiomeClimate::TemperatureModifier::None, 0.5f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createMangroveSwamp()
{
    Biome biome(Biomes::MangroveSwamp, "mangrove_swamp");
    biome.setDepth(-0.2f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.8f, BiomeClimate::TemperatureModifier::None, 0.9f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0x78A7FF)
            .waterColor(0x3A7F3E)
            .waterFogColor(0x0D2E1F)
            .skyColor(0x78A7FF)
            .grassColor(0x6A7039)
            .grassColorModifier(GrassColorModifier::Swamp)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createSwamp());
    return biome;
}

Biome createCherryGrove()
{
    Biome biome(Biomes::CherryGrove, "cherry_grove");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.5f, BiomeClimate::TemperatureModifier::None, 0.8f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x5D93DF)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .grassColor(0xB7A6E1)
            .foliageColor(0xB7A6E1)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createCherryGrove());
    return biome;
}

Biome createPaleGarden()
{
    Biome biome(Biomes::PaleGarden, "pale_garden");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setClimate(
        BiomeClimate(BiomeClimate::Precipitation::Rain, 0.7f, BiomeClimate::TemperatureModifier::None, 0.8f));
    biome.setEffects(BiomeEffects::Builder()
            .fogColor(0xC0D8FF)
            .waterColor(0x3F76E4)
            .waterFogColor(0x050533)
            .skyColor(0x78A7FF)
            .dryFoliageColor(0xAEB05E)
            .build());
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}
} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc
