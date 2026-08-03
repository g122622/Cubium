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
#include "common/resource/ResourceLocation.hpp"
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
const BlockState* getBlockState(Block* block)
{
    return block ? &block->defaultState() : nullptr;
}
} // namespace

// ============================================================================
// 下界生物群系工厂函数
// ============================================================================

Biome createNetherWastes()
{
    // 下界荒地：下界岩为主，猪灵、恶魂、岩浆怪
    Biome biome(Biomes::NetherWastes, "nether_wastes");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 3344392 (暗红色)
    biome.setEffects(BiomeEffects::Builder().fogColor(3344392).waterColor(4159204).waterFogColor(329011).build());

    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createNetherWastes());

    // 下界荒地环境音效
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.nether_wastes.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.nether_wastes.mood"),
        6000, // tick_delay
        8,    // block_search_extent
        2.0   // offset
        ));
    sounds.setAdditionsSound(SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.nether_wastes.additions"),
        0.0111 // tick_chance
        ));
    // 下界荒地专属音乐
    sounds.setMusic(BiomeMusic(ResourceLocation("minecraft:music.nether.nether_wastes"),
        12000, // min_delay_ticks
        24000, // max_delay_ticks
        false  // replace_current
        ));
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createSoulSandValley()
{
    // 灵魂沙谷：灵魂沙和灵魂土，骷髅和恶魂
    Biome biome(Biomes::SoulSandValley, "soul_sand_valley");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SOUL_SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SOUL_SOIL));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 1787717 (蓝灰色)
    biome.setEffects(BiomeEffects::Builder().fogColor(1787717).waterColor(4159204).waterFogColor(329011).build());

    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSoulSandValley());

    // 灵魂沙谷环境音效
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.soul_sand_valley.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.soul_sand_valley.mood"),
        6000, // tick_delay
        8,    // block_search_extent
        2.0   // offset
        ));
    sounds.setAdditionsSound(SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.soul_sand_valley.additions"),
        0.0111 // tick_chance
        ));
    // 灵魂沙谷专属音乐
    sounds.setMusic(BiomeMusic(ResourceLocation("minecraft:music.nether.soul_sand_valley"),
        12000, // min_delay_ticks
        24000, // max_delay_ticks
        false  // replace_current
        ));
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createCrimsonForest()
{
    // 绯红森林：绯红菌和疣猪兽，红色主题
    Biome biome(Biomes::CrimsonForest, "crimson_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::CRIMSON_NYLIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 3343107 (暗红色)
    biome.setEffects(BiomeEffects::Builder().fogColor(3343107).waterColor(4159204).waterFogColor(329011).build());

    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createCrimsonForest());

    // 绯红森林环境音效
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.crimson_forest.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.crimson_forest.mood"),
        6000, // tick_delay
        8,    // block_search_extent
        2.0   // offset
        ));
    sounds.setAdditionsSound(SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.crimson_forest.additions"),
        0.0111 // tick_chance
        ));
    // 绯红森林专属音乐
    sounds.setMusic(BiomeMusic(ResourceLocation("minecraft:music.nether.crimson_forest"),
        12000, // min_delay_ticks
        24000, // max_delay_ticks
        false  // replace_current
        ));
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createWarpedForest()
{
    // 诡异森林：诡异菌和末影人，青色主题
    Biome biome(Biomes::WarpedForest, "warped_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::WARPED_NYLIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 1705242 (青色)
    biome.setEffects(BiomeEffects::Builder().fogColor(1705242).waterColor(4159204).waterFogColor(329011).build());

    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createWarpedForest());

    // 诡异森林环境音效
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.warped_forest.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.warped_forest.mood"),
        6000, // tick_delay
        8,    // block_search_extent
        2.0   // offset
        ));
    sounds.setAdditionsSound(SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.warped_forest.additions"),
        0.0111 // tick_chance
        ));
    // 诡异森林专属音乐
    // 注意: sounds.json 中 music.nether.warped_forest 定义为空数组，实际上不会播放音乐
    // 但游戏代码中仍然注册了这个音乐选择器
    sounds.setMusic(BiomeMusic(ResourceLocation("minecraft:music.nether.warped_forest"),
        12000, // min_delay_ticks
        24000, // max_delay_ticks
        false  // replace_current
        ));
    biome.setAmbientSounds(sounds);

    return biome;
}

Biome createBasaltDeltas()
{
    // 玄武岩三角洲：玄武岩和岩浆块，黑色颗粒效果
    Biome biome(Biomes::BasaltDeltas, "basalt_deltas");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::BASALT));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::BASALT));
    biome.setClimate(BiomeClimate(false, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f, 0.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 6840176 (深灰蓝)
    biome.setEffects(BiomeEffects::Builder().fogColor(6840176).waterColor(4341314).waterFogColor(4341314).build());

    biome.setGenerationSettings(BiomeGenerationSettings{});
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createBasaltDeltas());

    // 玄武岩三角洲环境音效
    BiomeAmbientSounds sounds;
    sounds.setLoopSound(ResourceLocation("minecraft:ambient.basalt_deltas.loop"));
    sounds.setMoodSound(MoodSoundAmbience(ResourceLocation("minecraft:ambient.basalt_deltas.mood"),
        6000, // tick_delay
        8,    // block_search_extent
        2.0   // offset
        ));
    sounds.setAdditionsSound(SoundAdditionsAmbience(ResourceLocation("minecraft:ambient.basalt_deltas.additions"),
        0.0111 // tick_chance
        ));
    // 玄武岩三角洲专属音乐
    sounds.setMusic(BiomeMusic(ResourceLocation("minecraft:music.nether.basalt_deltas"),
        12000, // min_delay_ticks
        24000, // max_delay_ticks
        false  // replace_current
        ));
    biome.setAmbientSounds(sounds);

    return biome;
}
} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc
