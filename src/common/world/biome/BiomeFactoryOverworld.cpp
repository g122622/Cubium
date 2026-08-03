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
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

namespace mc {
namespace world {
namespace biome {

namespace BiomeFactory {

namespace {
// 辅助函数：获取方块状态
const BlockState* getBlockState(Block* block)
{
    return block ? &block->defaultState() : nullptr;
}
} // namespace

Biome createPlains()
{
    Biome biome(Biomes::Plains, "plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createDesert()
{
    Biome biome(Biomes::Desert, "desert");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createMountains()
{
    Biome biome(Biomes::Mountains, "mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createForest()
{
    Biome biome(Biomes::Forest, "forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createOcean()
{
    Biome biome(Biomes::Ocean, "ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 海洋生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    return biome;
}

Biome createDeepOcean()
{
    Biome biome(Biomes::DeepOcean, "deep_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDeepOcean());
    return biome;
}

Biome createTaiga()
{
    Biome biome(Biomes::Taiga, "taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createSnowyTaiga()
{
    // 雪地针叶林表面应为雪
    Biome biome(Biomes::SnowyTaiga, "snowy_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    // 雪地针叶林水体颜色
    biome.setEffects(BiomeEffects::Builder().waterColor(0x3D57E6).build());
    return biome;
}

Biome createJungle()
{
    Biome biome(Biomes::Jungle, "jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createSavanna()
{
    Biome biome(Biomes::Savanna, "savanna");
    biome.setDepth(0.3625f);
    biome.setScale(0.05f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavanna());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createShatteredSavanna()
{
    Biome biome(Biomes::ShatteredSavanna, "shattered_savanna");
    biome.setDepth(0.3625f);
    biome.setScale(1.225f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createShatteredSavanna());
    return biome;
}

Biome createSavannaPlateau()
{
    Biome biome(Biomes::SavannaPlateau, "savanna_plateau");
    biome.setDepth(1.05f);
    biome.setScale(0.0125f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavannaPlateau());
    return biome;
}

Biome createBadlands()
{
    // 恶地有特殊的草和树叶颜色
    Biome biome(Biomes::Badlands, "badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));  // RED_SAND substitute
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE)); // Terracotta substitute
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 恶地特殊的黄褐色草和树叶
    biome.setEffects(BiomeEffects::Builder()
            .grassColor(BiomeEffects::BADLANDS_GRASS_COLOR)
            .foliageColor(BiomeEffects::BADLANDS_FOLIAGE_COLOR)
            .grassColorModifier(GrassColorModifier::Badlands)
            .build());
    return biome;
}

Biome createErodedBadlands()
{
    // 风蚀恶地使用与恶地相同的颜色
    Biome biome(Biomes::ErodedBadlands, "eroded_badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 与恶地相同的颜色
    biome.setEffects(BiomeEffects::Builder()
            .grassColor(BiomeEffects::BADLANDS_GRASS_COLOR)
            .foliageColor(BiomeEffects::BADLANDS_FOLIAGE_COLOR)
            .grassColorModifier(GrassColorModifier::Badlands)
            .build());
    return biome;
}

Biome createBadlandsPlateau()
{
    // 恶地高原使用与恶地相同的颜色
    Biome biome(Biomes::BadlandsPlateau, "badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 与恶地相同的颜色
    biome.setEffects(BiomeEffects::Builder()
            .grassColor(BiomeEffects::BADLANDS_GRASS_COLOR)
            .foliageColor(BiomeEffects::BADLANDS_FOLIAGE_COLOR)
            .grassColorModifier(GrassColorModifier::Badlands)
            .build());
    return biome;
}

Biome createWoodedBadlandsPlateau()
{
    // 繁茂恶地高原有草和树叶
    Biome biome(Biomes::WoodedBadlandsPlateau, "wooded_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 与恶地相同的颜色
    biome.setEffects(BiomeEffects::Builder()
            .grassColor(BiomeEffects::BADLANDS_GRASS_COLOR)
            .foliageColor(BiomeEffects::BADLANDS_FOLIAGE_COLOR)
            .grassColorModifier(GrassColorModifier::Badlands)
            .build());
    return biome;
}

Biome createBeach()
{
    Biome biome(Biomes::Beach, "beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    return biome;
}

Biome createStoneShore()
{
    Biome biome(Biomes::StoneShore, "stone_shore");
    biome.setDepth(0.1f);
    biome.setScale(0.8f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    return biome;
}

Biome createSnowyBeach()
{
    Biome biome(Biomes::SnowyBeach, "snowy_beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.05f);
    biome.setHumidity(0.3f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    return biome;
}

Biome createSwamp()
{
    // 沼泽有特殊的水体颜色和草颜色修改器
    Biome biome(Biomes::Swamp, "swamp");
    biome.setDepth(-0.2f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSwamp());
    // 沼泽特殊颜色：水体为灰绿色，使用双色草/树叶
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::SWAMP_WATER_COLOR)
            .waterFogColor(BiomeEffects::SWAMP_WATER_FOG_COLOR)
            .fogColor(BiomeEffects::SWAMP_FOG_COLOR)
            .grassColorModifier(GrassColorModifier::Swamp)
            .build());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createRiver()
{
    Biome biome(Biomes::River, "river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createWoodedHills()
{
    Biome biome(Biomes::WoodedHills, "wooded_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createBirchForest()
{
    Biome biome(Biomes::BirchForest, "birch_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createDarkForest()
{
    // 黑森林有特殊的深绿色草
    Biome biome(Biomes::DarkForest, "dark_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    // 黑森林草颜色变暗
    biome.setEffects(BiomeEffects::Builder()
            .grassColor(BiomeEffects::DARK_FOREST_GRASS_COLOR)
            .grassColorModifier(GrassColorModifier::DarkForest)
            .dryFoliageColor(0x7B6B4D)
            .build());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createSnowyPlains()
{
    // 雪地平原表面应为雪
    Biome biome(Biomes::SnowyPlains, "snowy_plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.5f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createGiantTreeTaiga()
{
    Biome biome(Biomes::GiantTreeTaiga, "giant_tree_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());

    // 主世界默认洞穴心境音效
    BiomeAmbientSounds sounds;
    sounds.setMoodSound(MoodSoundAmbience::defaultCaveMood());
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createWoodedMountains()
{
    Biome biome(Biomes::WoodedMountains, "wooded_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createMountainEdge()
{
    Biome biome(Biomes::MountainEdge, "mountain_edge");
    biome.setDepth(0.8f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createFrozenOcean()
{
    // 冻洋有特殊的深紫色水体
    Biome biome(Biomes::FrozenOcean, "frozen_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(true, 0.0f, BiomeClimate::TemperatureModifier::Frozen, 0.5f, 0.5f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createFrozenOcean());
    // 冻洋水体颜色为深紫色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::FROZEN_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::DEFAULT_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createFrozenRiver()
{
    Biome biome(Biomes::FrozenRiver, "frozen_river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(true, 0.0f, BiomeClimate::TemperatureModifier::Frozen, 0.5f, 0.5f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    // 冻河使用冻洋的水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::FROZEN_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::DEFAULT_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createSnowyMountains()
{
    Biome biome(Biomes::SnowyMountains, "snowy_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createIceSpikes()
{
    Biome biome(Biomes::IceSpikes, "ice_spikes");
    biome.setDepth(0.4375f);
    biome.setScale(0.05f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createDeepFrozenOcean()
{
    // 深海冻洋使用与冻洋相同的水体颜色
    Biome biome(Biomes::DeepFrozenOcean, "deep_frozen_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(true, 0.0f, BiomeClimate::TemperatureModifier::Frozen, 0.5f, 0.5f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createFrozenOcean());
    // 与冻洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::FROZEN_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::DEFAULT_WATER_FOG_COLOR)
            .build());
    return biome;
}

// ============================================================================
// 高优先级生物群系（阶段1）
// ============================================================================

Biome createWarmOcean()
{
    // 温暖海洋，温度高，沙子底部，水体为青绿色
    Biome biome(Biomes::WarmOcean, "warm_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createWarmOcean());
    // 暖水海洋水体颜色为青绿色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::WARM_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createLukewarmOcean()
{
    // 温水海洋，水体为浅蓝色
    Biome biome(Biomes::LukewarmOcean, "lukewarm_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createLukewarmOcean());
    // 温水海洋水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createColdOcean()
{
    // 冷水海洋，水体为深蓝色
    Biome biome(Biomes::ColdOcean, "cold_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createColdOcean());
    // 冷水海洋水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::COLD_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createDeepWarmOcean()
{
    // 深海暖水海洋，使用与暖水海洋相同的水体颜色
    Biome biome(Biomes::DeepWarmOcean, "deep_warm_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDeepWarmOcean());
    // 与暖水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::WARM_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createDeepLukewarmOcean()
{
    // 深海温水海洋
    Biome biome(Biomes::DeepLukewarmOcean, "deep_lukewarm_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 对应 MC 1.16.5 func_244237_d(true)：深水版本 squid/cod 权重与浅水版本不同
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDeepLukewarmOcean());
    // 与温水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createDeepColdOcean()
{
    // 深海冷水海洋
    Biome biome(Biomes::DeepColdOcean, "deep_cold_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createColdOcean());
    // 与冷水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::COLD_OCEAN_WATER_COLOR)
            .waterFogColor(BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR)
            .build());
    return biome;
}

Biome createJungleHills()
{
    Biome biome(Biomes::JungleHills, "jungle_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createJungleEdge()
{
    // 旧名 JungleEdge，对应 MC 1.16.5 sparseJungle()（baseJungleSpawns + wolf）
    Biome biome(Biomes::JungleEdge, "jungle_edge");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSparseJungle());
    return biome;
}

Biome createBambooJungle()
{
    Biome biome(Biomes::BambooJungle, "bamboo_jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createBambooJungle());
    return biome;
}

Biome createBambooJungleHills()
{
    Biome biome(Biomes::BambooJungleHills, "bamboo_jungle_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createBambooJungle());
    return biome;
}

Biome createBirchForestHills()
{
    Biome biome(Biomes::BirchForestHills, "birch_forest_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createFlowerForest()
{
    Biome biome(Biomes::FlowerForest, "flower_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createTallBirchForest()
{
    // 高桦木
    Biome biome(Biomes::TallBirchForest, "tall_birch_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createTallBirchHills()
{
    Biome biome(Biomes::TallBirchHills, "tall_birch_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createDarkForestHills()
{
    Biome biome(Biomes::DarkForestHills, "dark_forest_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createMushroomFields()
{
    // 蘑菇岛
    Biome biome(Biomes::MushroomFields, "mushroom_fields");
    biome.setDepth(0.2f);
    biome.setScale(0.3f);
    biome.setTemperature(0.9f);
    biome.setHumidity(1.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::MYCELIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(true, 0.9f, BiomeClimate::TemperatureModifier::None, 1.0f, 1.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createEmpty()); // 蘑菇岛没有普通生物
    return biome;
}

Biome createMushroomFieldShore()
{
    // 蘑菇岛海岸
    Biome biome(Biomes::MushroomFieldShore, "mushroom_field_shore");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.9f);
    biome.setHumidity(1.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::MYCELIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(true, 0.9f, BiomeClimate::TemperatureModifier::None, 1.0f, 1.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createEmpty());
    return biome;
}

Biome createDesertHills()
{
    Biome biome(Biomes::DesertHills, "desert_hills");
    biome.setDepth(0.225f);
    biome.setScale(0.25f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createTaigaHills()
{
    Biome biome(Biomes::TaigaHills, "taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createGiantSpruceTaiga()
{
    Biome biome(Biomes::GiantSpruceTaiga, "giant_spruce_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createGiantSpruceTaigaHills()
{
    Biome biome(Biomes::GiantSpruceTaigaHills, "giant_spruce_taiga_hills");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

// ============================================================================
// 中优先级生物群系（阶段2）
// ============================================================================

Biome createSunflowerPlains()
{
    Biome biome(Biomes::SunflowerPlains, "sunflower_plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createDesertLakes()
{
    Biome biome(Biomes::DesertLakes, "desert_lakes");
    biome.setDepth(0.225f);
    biome.setScale(0.25f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createGravellyMountains()
{
    Biome biome(Biomes::GravellyMountains, "gravelly_mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createTaigaMountains()
{
    Biome biome(Biomes::TaigaMountains, "taiga_mountains");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createSwampHills()
{
    Biome biome(Biomes::SwampHills, "swamp_hills");
    biome.setDepth(-0.1f);
    biome.setScale(0.3f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSwamp());
    // 沼泽山丘使用与沼泽相同的颜色
    biome.setEffects(BiomeEffects::Builder()
            .waterColor(BiomeEffects::SWAMP_WATER_COLOR)
            .waterFogColor(BiomeEffects::SWAMP_WATER_FOG_COLOR)
            .fogColor(BiomeEffects::SWAMP_FOG_COLOR)
            .grassColorModifier(GrassColorModifier::Swamp)
            .build());
    return biome;
}

Biome createModifiedJungle()
{
    Biome biome(Biomes::ModifiedJungle, "modified_jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createModifiedJungleEdge()
{
    Biome biome(Biomes::ModifiedJungleEdge, "modified_jungle_edge");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createSnowyTaigaMountains()
{
    Biome biome(Biomes::SnowyTaigaMountains, "snowy_taiga_mountains");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    // 雪地针叶林山地水体颜色
    biome.setEffects(BiomeEffects::Builder().waterColor(0x3D57E6).build());
    return biome;
}

Biome createModifiedGravellyMountains()
{
    Biome biome(Biomes::ModifiedGravellyMountains, "modified_gravelly_mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createShatteredSavannaPlateau()
{
    Biome biome(Biomes::ShatteredSavannaPlateau, "shattered_savanna_plateau");
    biome.setDepth(1.05f);
    biome.setScale(0.0125f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    // 对应 MC 1.16.5 func_244211_a(true, true)：spawn list 与 ShatteredSavanna 相同（无 llama/wolf）
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createShatteredSavanna());
    return biome;
}

Biome createModifiedWoodedBadlandsPlateau()
{
    Biome biome(Biomes::ModifiedWoodedBadlandsPlateau, "modified_wooded_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createModifiedBadlandsPlateau()
{
    Biome biome(Biomes::ModifiedBadlandsPlateau, "modified_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createGiantTreeTaigaHillsBiome()
{
    // 注意：这是 GiantTreeTaiga 的丘陵变体，与已有的 GiantTreeTaigaHills 类似
    Biome biome(Biomes::GiantTreeTaigaHills, "giant_tree_taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createSnowyTaigaHills()
{
    Biome biome(Biomes::SnowyTaigaHills, "snowy_taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setHasPrecipitation(true);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    // 雪地针叶林丘陵水体颜色
    biome.setEffects(BiomeEffects::Builder().waterColor(0x3D57E6).build());
    return biome;
}
} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc
