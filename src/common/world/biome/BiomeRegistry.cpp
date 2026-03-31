#include "BiomeRegistry.hpp"
#include "BiomeEffects.hpp"
#include "../block/VanillaBlocks.hpp"
#include <algorithm>

namespace mc {

// 使用 biome 命名空间中的类型
using world::biome::BiomeEffects;
using world::biome::GrassColorModifier;

// ============================================================================
// BiomeRegistry 实现
// ============================================================================

BiomeRegistry& BiomeRegistry::instance()
{
    static BiomeRegistry instance;
    return instance;
}

BiomeRegistry::BiomeRegistry()
    : m_defaultBiome(Biomes::Plains, "plains")
{
}

void BiomeRegistry::initialize()
{
    if (!m_biomes.empty()) {
        return;  // 已经初始化
    }
    registerDefaultBiomes();
}

void BiomeRegistry::registerBiome(const Biome& biome)
{
    const BiomeId id = biome.id();
    if (id >= m_biomes.size()) {
        m_biomes.resize(id + 1);
    }
    m_biomes[id] = biome;
}

const Biome& BiomeRegistry::get(BiomeId id) const
{
    if (id < m_biomes.size()) {
        return m_biomes[id];
    }
    return m_defaultBiome;
}

bool BiomeRegistry::hasBiome(BiomeId id) const
{
    return id < m_biomes.size();
}

void BiomeRegistry::registerDefaultBiomes()
{
    // 注册所有默认生物群系

    // === 基础生物群系 (0-13) ===
    registerBiome(BiomeFactory::createOcean());
    registerBiome(BiomeFactory::createPlains());
    registerBiome(BiomeFactory::createDesert());
    registerBiome(BiomeFactory::createMountains());
    registerBiome(BiomeFactory::createForest());
    registerBiome(BiomeFactory::createTaiga());
    registerBiome(BiomeFactory::createSwamp());
    registerBiome(BiomeFactory::createRiver());
    registerBiome(BiomeFactory::createFrozenOcean());
    registerBiome(BiomeFactory::createFrozenRiver());
    registerBiome(BiomeFactory::createSnowyPlains());
    registerBiome(BiomeFactory::createSnowyMountains());

    // === 蘑菇岛 (14-15) ===
    registerBiome(BiomeFactory::createMushroomFields());
    registerBiome(BiomeFactory::createMushroomFieldShore());

    // === 海滩 (16) ===
    registerBiome(BiomeFactory::createBeach());

    // === 山地变体和丘陵 (17-20) ===
    registerBiome(BiomeFactory::createDesertHills());
    registerBiome(BiomeFactory::createWoodedHills());
    registerBiome(BiomeFactory::createTaigaHills());
    registerBiome(BiomeFactory::createMountainEdge());

    // === 丛林 (21-23) ===
    registerBiome(BiomeFactory::createJungle());
    registerBiome(BiomeFactory::createJungleHills());
    registerBiome(BiomeFactory::createJungleEdge());

    // === 深海和石岸 (24-25) ===
    registerBiome(BiomeFactory::createDeepOcean());
    registerBiome(BiomeFactory::createStoneShore());

    // === 雪地海滩 (26) ===
    registerBiome(BiomeFactory::createSnowyBeach());

    // === 桦木森林 (27-28) ===
    registerBiome(BiomeFactory::createBirchForest());
    registerBiome(BiomeFactory::createBirchForestHills());

    // === 黑森林 (29) ===
    registerBiome(BiomeFactory::createDarkForest());

    // === 雪地针叶林 (30-31) ===
    registerBiome(BiomeFactory::createSnowyTaiga());
    registerBiome(BiomeFactory::createSnowyTaigaHills());

    // === 大型针叶林 (32-33) ===
    registerBiome(BiomeFactory::createGiantTreeTaiga());
    registerBiome(BiomeFactory::createGiantTreeTaigaHillsBiome());

    // === 热带草原 (34-36) ===
    registerBiome(BiomeFactory::createWoodedMountains());
    registerBiome(BiomeFactory::createSavanna());
    registerBiome(BiomeFactory::createSavannaPlateau());

    // === 恶地 (37-39) ===
    registerBiome(BiomeFactory::createBadlands());
    registerBiome(BiomeFactory::createWoodedBadlandsPlateau());
    registerBiome(BiomeFactory::createBadlandsPlateau());

    // === 海洋温度变体 (44-50) ===
    registerBiome(BiomeFactory::createWarmOcean());
    registerBiome(BiomeFactory::createLukewarmOcean());
    registerBiome(BiomeFactory::createColdOcean());
    registerBiome(BiomeFactory::createDeepWarmOcean());
    registerBiome(BiomeFactory::createDeepLukewarmOcean());
    registerBiome(BiomeFactory::createDeepColdOcean());
    registerBiome(BiomeFactory::createDeepFrozenOcean());

    // === 特殊地形变体 ===
    registerBiome(BiomeFactory::createIceSpikes());

    // === 丛林变体 (168-169) ===
    registerBiome(BiomeFactory::createBambooJungle());
    registerBiome(BiomeFactory::createBambooJungleHills());

    // === 森林变体 ===
    registerBiome(BiomeFactory::createFlowerForest());
    registerBiome(BiomeFactory::createTallBirchForest());
    registerBiome(BiomeFactory::createTallBirchHills());
    registerBiome(BiomeFactory::createDarkForestHills());

    // === 巨型针叶林变体 (160-161) ===
    registerBiome(BiomeFactory::createGiantSpruceTaiga());
    registerBiome(BiomeFactory::createGiantSpruceTaigaHills());

    // === 稀有变体生物群系 (129-167) ===
    registerBiome(BiomeFactory::createSunflowerPlains());
    registerBiome(BiomeFactory::createDesertLakes());
    registerBiome(BiomeFactory::createGravellyMountains());
    registerBiome(BiomeFactory::createTaigaMountains());
    registerBiome(BiomeFactory::createSwampHills());
    registerBiome(BiomeFactory::createModifiedJungle());
    registerBiome(BiomeFactory::createModifiedJungleEdge());
    registerBiome(BiomeFactory::createSnowyTaigaMountains());
    registerBiome(BiomeFactory::createModifiedGravellyMountains());
    registerBiome(BiomeFactory::createShatteredSavanna());
    registerBiome(BiomeFactory::createShatteredSavannaPlateau());
    registerBiome(BiomeFactory::createErodedBadlands());
    registerBiome(BiomeFactory::createModifiedWoodedBadlandsPlateau());
    registerBiome(BiomeFactory::createModifiedBadlandsPlateau());

    // === 下界生物群系 (8, 170-173) ===
    registerBiome(BiomeFactory::createNetherWastes());        // ID: 8
    registerBiome(BiomeFactory::createSoulSandValley());      // ID: 170
    registerBiome(BiomeFactory::createCrimsonForest());       // ID: 171
    registerBiome(BiomeFactory::createWarpedForest());        // ID: 172
    registerBiome(BiomeFactory::createBasaltDeltas());        // ID: 173

    // === 末地生物群系 (9, 40-43) ===
    registerBiome(BiomeFactory::createTheEnd());              // ID: 9
    registerBiome(BiomeFactory::createSmallEndIslands());     // ID: 40
    registerBiome(BiomeFactory::createEndMidlands());         // ID: 41
    registerBiome(BiomeFactory::createEndHighlands());        // ID: 42
    registerBiome(BiomeFactory::createEndBarrens());          // ID: 43
}

// ============================================================================
// BiomeFactory 实现（参考 MC 1.16.5 BiomeMaker）
// ============================================================================

namespace BiomeFactory {

namespace {
    // 辅助函数：获取方块状态
    const BlockState* getBlockState(Block* block) {
        return block ? &block->defaultState() : nullptr;
    }
}

Biome createPlains()
{
    // MC: depth=0.125F, scale=0.05F
    Biome biome(Biomes::Plains, "plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createPlains());
    // 设置生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createDesert()
{
    // MC: depth=0.125F, scale=0.05F
    Biome biome(Biomes::Desert, "desert");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createDesert());
    // 沙漠生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createMountains()
{
    // MC: depth=1.0F, scale=0.5F (Extreme Hills)
    Biome biome(Biomes::Mountains, "mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::Forest, "forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    // 森林生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    Biome biome(Biomes::Ocean, "ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    // 海洋生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    return biome;
}

Biome createDeepOcean()
{
    // MC: depth=-1.8F, scale=0.1F (Deep Ocean)
    Biome biome(Biomes::DeepOcean, "deep_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    return biome;
}

Biome createTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::Taiga, "taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createSnowyTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    // 雪地针叶林表面应为雪
    Biome biome(Biomes::SnowyTaiga, "snowy_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createJungle()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::Jungle, "jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createSavanna()
{
    // MC: depth=0.3625F, scale=0.05F
    Biome biome(Biomes::Savanna, "savanna");
    biome.setDepth(0.3625f);
    biome.setScale(0.05f);
    biome.setTemperature(1.2f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavanna());
    return biome;
}

Biome createShatteredSavanna()
{
    // MC: depth=0.3625F, scale=1.225F (Shattered Savanna)
    Biome biome(Biomes::ShatteredSavanna, "shattered_savanna");
    biome.setDepth(0.3625f);
    biome.setScale(1.225f);
    biome.setTemperature(1.1f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavanna());
    return biome;
}

Biome createSavannaPlateau()
{
    // MC: depth=1.05F, scale=0.0125F
    Biome biome(Biomes::SavannaPlateau, "savanna_plateau");
    biome.setDepth(1.05f);
    biome.setScale(0.0125f);
    biome.setTemperature(1.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavanna());
    return biome;
}

Biome createBadlands()
{
    // MC: depth=0.1F, scale=0.2F
    // 恶地有特殊的草和树叶颜色
    Biome biome(Biomes::Badlands, "badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE)); // RED_SAND substitute
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE)); // Terracotta substitute
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createBadlands());
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
    // MC: depth=0.1F, scale=0.2F
    // 风蚀恶地使用与恶地相同的颜色
    Biome biome(Biomes::ErodedBadlands, "eroded_badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
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
    // MC: depth=1.5F, scale=0.025F
    // 恶地高原使用与恶地相同的颜色
    Biome biome(Biomes::BadlandsPlateau, "badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
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
    // MC: depth=1.5F, scale=0.025F
    // 繁茂恶地高原有草和树叶
    Biome biome(Biomes::WoodedBadlandsPlateau, "wooded_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
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
    // MC: depth=0.0F, scale=0.025F
    Biome biome(Biomes::Beach, "beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createStoneShore()
{
    // MC: depth=0.1F, scale=0.8F (Stone Shore)
    Biome biome(Biomes::StoneShore, "stone_shore");
    biome.setDepth(0.1f);
    biome.setScale(0.8f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createSnowyBeach()
{
    // MC: depth=0.0F, scale=0.025F
    Biome biome(Biomes::SnowyBeach, "snowy_beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.05f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createSwamp()
{
    // MC: depth=-0.2F, scale=0.1F
    // 沼泽有特殊的水体颜色和草颜色修改器
    Biome biome(Biomes::Swamp, "swamp");
    biome.setDepth(-0.2f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createSwamp());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSwamp());
    // 沼泽特殊颜色：水体为灰绿色，使用双色草/树叶
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::SWAMP_WATER_COLOR)
        .waterFogColor(BiomeEffects::SWAMP_WATER_FOG_COLOR)
        .fogColor(BiomeEffects::SWAMP_FOG_COLOR)
        .grassColorModifier(GrassColorModifier::Swamp)
        .build());
    return biome;
}

Biome createRiver()
{
    // MC: depth=-0.5F, scale=0.0F
    Biome biome(Biomes::River, "river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createWoodedHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::WoodedHills, "wooded_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createBirchForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::BirchForest, "birch_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createDarkForest()
{
    // MC: depth=0.1F, scale=0.2F
    // 黑森林有特殊的深绿色草
    Biome biome(Biomes::DarkForest, "dark_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    // 黑森林草颜色变暗
    biome.setEffects(BiomeEffects::Builder()
        .grassColor(BiomeEffects::DARK_FOREST_GRASS_COLOR)
        .grassColorModifier(GrassColorModifier::DarkForest)
        .build());
    return biome;
}

Biome createSnowyPlains()
{
    // MC: depth=0.125F, scale=0.05F (Snowy Tundra)
    // 雪地平原表面应为雪
    Biome biome(Biomes::SnowyPlains, "snowy_plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createGiantTreeTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::GiantTreeTaiga, "giant_tree_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createWoodedMountains()
{
    // MC: depth=0.45F, scale=0.3F (Gravelly Mountains)
    Biome biome(Biomes::WoodedMountains, "wooded_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createMountainEdge()
{
    // MC: depth=0.8F, scale=0.3F (Mountain Edge)
    Biome biome(Biomes::MountainEdge, "mountain_edge");
    biome.setDepth(0.8f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createFrozenOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    // 冻洋有特殊的深紫色水体
    Biome biome(Biomes::FrozenOcean, "frozen_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 冻洋水体颜色为深紫色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::FROZEN_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::DEFAULT_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createFrozenRiver()
{
    // MC: depth=-0.5F, scale=0.0F
    Biome biome(Biomes::FrozenRiver, "frozen_river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
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
    // MC: depth=0.45F, scale=0.3F (Snowy Mountains)
    Biome biome(Biomes::SnowyMountains, "snowy_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createIceSpikes()
{
    // MC: depth=0.4375F, scale=0.05F
    Biome biome(Biomes::IceSpikes, "ice_spikes");
    biome.setDepth(0.4375f);
    biome.setScale(0.05f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createIceSpikes());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createDeepFrozenOcean()
{
    // MC: depth=-1.8F, scale=0.1F
    // 深海冻洋使用与冻洋相同的水体颜色
    Biome biome(Biomes::DeepFrozenOcean, "deep_frozen_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::ICE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
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
    // MC: depth=-1.0F, scale=0.1F
    // 温暖海洋，温度高，沙子底部，水体为青绿色
    Biome biome(Biomes::WarmOcean, "warm_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 暖水海洋水体颜色为青绿色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::WARM_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createLukewarmOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    // 温水海洋，水体为浅蓝色
    Biome biome(Biomes::LukewarmOcean, "lukewarm_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 温水海洋水体颜色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createColdOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    // 冷水海洋，水体为深蓝色
    Biome biome(Biomes::ColdOcean, "cold_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 冷水海洋水体颜色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::COLD_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createDeepWarmOcean()
{
    // MC: depth=-1.8F, scale=0.1F
    // 深海暖水海洋，使用与暖水海洋相同的水体颜色
    Biome biome(Biomes::DeepWarmOcean, "deep_warm_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 与暖水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::WARM_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createDeepLukewarmOcean()
{
    // MC: depth=-1.8F, scale=0.1F
    // 深海温水海洋
    Biome biome(Biomes::DeepLukewarmOcean, "deep_lukewarm_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 与温水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createDeepColdOcean()
{
    // MC: depth=-1.8F, scale=0.1F
    // 深海冷水海洋
    Biome biome(Biomes::DeepColdOcean, "deep_cold_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    // 与冷水海洋相同的水体颜色
    biome.setEffects(BiomeEffects::Builder()
        .waterColor(BiomeEffects::COLD_OCEAN_WATER_COLOR)
        .waterFogColor(BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR)
        .build());
    return biome;
}

Biome createJungleHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::JungleHills, "jungle_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createJungleEdge()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::JungleEdge, "jungle_edge");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createBambooJungle()
{
    // MC: depth=0.1F, scale=0.2F
    // 暂不生成竹子，使用丛林生成设置
    Biome biome(Biomes::BambooJungle, "bamboo_jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createBambooJungleHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::BambooJungleHills, "bamboo_jungle_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createBirchForestHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::BirchForestHills, "birch_forest_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createFlowerForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::FlowerForest, "flower_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createFlowerForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createTallBirchForest()
{
    // MC: depth=0.1F, scale=0.2F
    // 高桦木
    Biome biome(Biomes::TallBirchForest, "tall_birch_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createTallBirchHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::TallBirchHills, "tall_birch_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createDarkForestHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::DarkForestHills, "dark_forest_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createMushroomFields()
{
    // MC: depth=0.2F, scale=0.3F
    // 蘑菇岛
    Biome biome(Biomes::MushroomFields, "mushroom_fields");
    biome.setDepth(0.2f);
    biome.setScale(0.3f);
    biome.setTemperature(0.9f);
    biome.setHumidity(1.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::MYCELIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createEmpty()); // 蘑菇岛没有普通生物
    return biome;
}

Biome createMushroomFieldShore()
{
    // MC: depth=0.0F, scale=0.025F
    // 蘑菇岛海岸
    Biome biome(Biomes::MushroomFieldShore, "mushroom_field_shore");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.9f);
    biome.setHumidity(1.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::MYCELIUM));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createEmpty());
    return biome;
}

Biome createDesertHills()
{
    // MC: depth=0.225F, scale=0.25F
    Biome biome(Biomes::DesertHills, "desert_hills");
    biome.setDepth(0.225f);
    biome.setScale(0.25f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createDesert());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createTaigaHills()
{
    // MC: depth=0.3F, scale=0.25F
    Biome biome(Biomes::TaigaHills, "taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createGiantSpruceTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::GiantSpruceTaiga, "giant_spruce_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createGiantSpruceTaigaHills()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::GiantSpruceTaigaHills, "giant_spruce_taiga_hills");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.25f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

// ============================================================================
// 中优先级生物群系（阶段2）
// ============================================================================

Biome createSunflowerPlains()
{
    // MC: depth=0.125F, scale=0.05F
    Biome biome(Biomes::SunflowerPlains, "sunflower_plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createPlains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createDesertLakes()
{
    // MC: depth=0.225F, scale=0.25F
    Biome biome(Biomes::DesertLakes, "desert_lakes");
    biome.setDepth(0.225f);
    biome.setScale(0.25f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SAND));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::SAND));
    biome.setGenerationSettings(BiomeGenerationSettings::createDesert());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createGravellyMountains()
{
    // MC: depth=1.0F, scale=0.5F
    Biome biome(Biomes::GravellyMountains, "gravelly_mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createTaigaMountains()
{
    // MC: depth=0.3F, scale=0.25F
    Biome biome(Biomes::TaigaMountains, "taiga_mountains");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createSwampHills()
{
    // MC: depth=-0.1F, scale=0.3F
    Biome biome(Biomes::SwampHills, "swamp_hills");
    biome.setDepth(-0.1f);
    biome.setScale(0.3f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
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
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::ModifiedJungle, "modified_jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createModifiedJungleEdge()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::ModifiedJungleEdge, "modified_jungle_edge");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createJungle());
    return biome;
}

Biome createSnowyTaigaMountains()
{
    // MC: depth=0.3F, scale=0.25F
    Biome biome(Biomes::SnowyTaigaMountains, "snowy_taiga_mountains");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

Biome createModifiedGravellyMountains()
{
    // MC: depth=1.0F, scale=0.5F
    Biome biome(Biomes::ModifiedGravellyMountains, "modified_gravelly_mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::STONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::STONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createMountains());
    return biome;
}

Biome createShatteredSavannaPlateau()
{
    // MC: depth=1.05F, scale=0.0125F
    Biome biome(Biomes::ShatteredSavannaPlateau, "shattered_savanna_plateau");
    biome.setDepth(1.05f);
    biome.setScale(0.0125f);
    biome.setTemperature(1.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSavanna());
    return biome;
}

Biome createModifiedWoodedBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    Biome biome(Biomes::ModifiedWoodedBadlandsPlateau, "modified_wooded_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createModifiedBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    Biome biome(Biomes::ModifiedBadlandsPlateau, "modified_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::COBBLESTONE));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::RED_SANDSTONE));
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createGiantTreeTaigaHillsBiome()
{
    // MC: depth=0.3F, scale=0.25F
    // 注意：这是 GiantTreeTaiga 的丘陵变体，与已有的 GiantTreeTaigaHills 类似
    Biome biome(Biomes::GiantTreeTaigaHills, "giant_tree_taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::GRASS_BLOCK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTaiga());
    return biome;
}

Biome createSnowyTaigaHills()
{
    // MC: depth=0.3F, scale=0.25F
    Biome biome(Biomes::SnowyTaigaHills, "snowy_taiga_hills");
    biome.setDepth(0.3f);
    biome.setScale(0.25f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SNOW));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::DIRT));
    biome.setUnderWaterBlock(getBlockState(VanillaBlocks::GRAVEL));
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSnowy());
    return biome;
}

// ============================================================================
// 下界生物群系工厂函数（参考 MC 1.16.5 BiomeMaker）
// ============================================================================

Biome createNetherWastes()
{
    // MC: 下界荒地 - 下界岩为主，猪灵、恶魂、岩浆怪
    // depth=0.1F, scale=0.2F, temperature=2.0F
    Biome biome(Biomes::NetherWastes, "nether_wastes");
    biome.setCategory(Biome::Category::Nether);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 3344392 (暗红色)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(3344392)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createNether());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createNetherWastes());
    return biome;
}

Biome createSoulSandValley()
{
    // MC: 灵魂沙谷 - 灵魂沙和灵魂土，骷髅和恶魂
    // depth=0.1F, scale=0.2F, temperature=2.0F
    Biome biome(Biomes::SoulSandValley, "soul_sand_valley");
    biome.setCategory(Biome::Category::Nether);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::SOUL_SAND));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::SOUL_SOIL));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 1787717 (蓝灰色)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(1787717)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createSoulSandValley());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createSoulSandValley());
    return biome;
}

Biome createCrimsonForest()
{
    // MC: 绯红森林 - 绯红菌和疣猪兽，红色主题
    // depth=0.1F, scale=0.2F, temperature=2.0F
    Biome biome(Biomes::CrimsonForest, "crimson_forest");
    biome.setCategory(Biome::Category::Nether);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    // TODO: 添加绯红菌岩方块
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 3343107 (暗红色)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(3343107)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createCrimsonForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createCrimsonForest());
    return biome;
}

Biome createWarpedForest()
{
    // MC: 诡异森林 - 诡异菌和末影人，青色主题
    // depth=0.1F, scale=0.2F, temperature=2.0F
    Biome biome(Biomes::WarpedForest, "warped_forest");
    biome.setCategory(Biome::Category::Nether);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    // TODO: 添加诡异菌岩方块
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::NETHERRACK));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 1705242 (青色)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(1705242)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createWarpedForest());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createWarpedForest());
    return biome;
}

Biome createBasaltDeltas()
{
    // MC: 玄武岩三角洲 - 玄武岩和岩浆块，黑色颗粒效果
    // depth=0.1F, scale=0.2F, temperature=2.0F
    Biome biome(Biomes::BasaltDeltas, "basalt_deltas");
    biome.setCategory(Biome::Category::Nether);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::BASALT));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::BASALT));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f));

    // 视觉效果：雾颜色 6840176 (深灰蓝)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(6840176)
        .waterColor(4341314)
        .waterFogColor(4341314)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createBasaltDeltas());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createBasaltDeltas());
    return biome;
}

// ============================================================================
// 末地生物群系工厂函数（参考 MC 1.16.5 BiomeMaker）
// ============================================================================

Biome createTheEnd()
{
    // MC: 末地主岛 - 末影龙战斗区域
    // depth=0.1F, scale=0.2F, temperature=0.5F
    Biome biome(Biomes::TheEnd, "the_end");
    biome.setCategory(Biome::Category::TheEnd);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, 0.0f, 0.5f));

    // 视觉效果：雾颜色 10518688 (暗紫色)
    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(10518688)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createTheEnd());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createSmallEndIslands()
{
    // MC: 小型末地岛屿 - 外岛的小型岛屿群
    // depth=0.1F, scale=0.2F, temperature=0.5F
    Biome biome(Biomes::SmallEndIslands, "small_end_islands");
    biome.setCategory(Biome::Category::TheEnd);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, 0.0f, 0.5f));

    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(10518688)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createSmallEndIslands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndMidlands()
{
    // MC: 末地中部 - 外岛过渡区域
    // depth=0.1F, scale=0.2F, temperature=0.5F
    Biome biome(Biomes::EndMidlands, "end_midlands");
    biome.setCategory(Biome::Category::TheEnd);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, 0.0f, 0.5f));

    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(10518688)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createEndMidlands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndHighlands()
{
    // MC: 末地高地 - 末地城和紫颂树生成区域
    // depth=0.1F, scale=0.2F, temperature=0.5F
    Biome biome(Biomes::EndHighlands, "end_highlands");
    biome.setCategory(Biome::Category::TheEnd);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, 0.0f, 0.5f));

    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(10518688)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createEndHighlands());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

Biome createEndBarrens()
{
    // MC: 末地荒地 - 空旷区域，无特征
    // depth=0.1F, scale=0.2F, temperature=0.5F
    Biome biome(Biomes::EndBarrens, "end_barrens");
    biome.setCategory(Biome::Category::TheEnd);
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setSubSurfaceBlock(getBlockState(VanillaBlocks::END_STONE));
    biome.setClimate(BiomeClimate(BiomeClimate::Precipitation::None, 0.5f, 0.0f, 0.5f));

    biome.setEffects(world::biome::BiomeEffects::Builder()
        .fogColor(10518688)
        .waterColor(4159204)
        .waterFogColor(329011)
        .build());

    biome.setGenerationSettings(BiomeGenerationSettings::createEndBarrens());
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createTheEnd());
    return biome;
}

} // namespace BiomeFactory

} // namespace mc
