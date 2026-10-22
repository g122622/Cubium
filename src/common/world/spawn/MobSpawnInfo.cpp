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

#include "MobSpawnInfo.hpp"

#include "common/entity/core/EntityRegistry.hpp"

namespace mc {
namespace world {
namespace spawn {

// ============================================================================
// 工厂方法实现
// ============================================================================
//
// 全部条目逐项取自 MC Java 1.21.11 的生物群系刷怪数据
// （net/minecraft/data/worldgen/biome/OverworldBiomes.java、NetherBiomes.java 与
// net/minecraft/data/worldgen/BiomeDefaultFeatures.java 的 addSpawn 调用），
// 权重、最小/最大群组数量、分类与生物群系属性（creatureGenerationProbability、spawn cost）一一对应。
//
// 与原版的已知偏差（对应实体尚未实现，无法进入 spawn list）：
//   1) camel（沙漠 CREATURE 1,1,1）——CamelEntity 未实现；
//   3) parched（沙漠 MONSTER 50,4,4）——ParchedEntity 未实现；
//   上述条目若写入刷怪表，会占用权重并在抽取后被 NaturalSpawner 跳过，使实际生成分布偏离原版，
//   因此暂不加入（内置刷怪表与数据包口径保持一致）。
// TODO(spawn-list-entities): 上述实体实现并注册后，在此补齐对应 spawn list，并删除本条注释中的相应条目。

namespace {

using entity::EntityTypeKeys::ARMADILLO;
using entity::EntityTypeKeys::GOAT;
using entity::EntityTypeKeys::AXOLOTL;
using entity::EntityTypeKeys::BAT;
using entity::EntityTypeKeys::BOGGED;
using entity::EntityTypeKeys::CHICKEN;
using entity::EntityTypeKeys::COD;
using entity::EntityTypeKeys::COW;
using entity::EntityTypeKeys::CREEPER;
using entity::EntityTypeKeys::DOLPHIN;
using entity::EntityTypeKeys::DONKEY;
using entity::EntityTypeKeys::DROWNED;
using entity::EntityTypeKeys::ENDERMAN;
using entity::EntityTypeKeys::FOX;
using entity::EntityTypeKeys::FROG;
using entity::EntityTypeKeys::GHAST;
using entity::EntityTypeKeys::GLOW_SQUID;
using entity::EntityTypeKeys::HOGLIN;
using entity::EntityTypeKeys::HORSE;
using entity::EntityTypeKeys::HUSK;
using entity::EntityTypeKeys::LLAMA;
using entity::EntityTypeKeys::MAGMA_CUBE;
using entity::EntityTypeKeys::MOOSHROOM;
using entity::EntityTypeKeys::NAUTILUS;
using entity::EntityTypeKeys::OCELOT;
using entity::EntityTypeKeys::PANDA;
using entity::EntityTypeKeys::PARROT;
using entity::EntityTypeKeys::PIG;
using entity::EntityTypeKeys::PIGLIN;
using entity::EntityTypeKeys::POLAR_BEAR;
using entity::EntityTypeKeys::PUFFERFISH;
using entity::EntityTypeKeys::RABBIT;
using entity::EntityTypeKeys::SALMON;
using entity::EntityTypeKeys::SHEEP;
using entity::EntityTypeKeys::SKELETON;
using entity::EntityTypeKeys::SLIME;
using entity::EntityTypeKeys::SPIDER;
using entity::EntityTypeKeys::SQUID;
using entity::EntityTypeKeys::STRAY;
using entity::EntityTypeKeys::STRIDER;
using entity::EntityTypeKeys::TROPICAL_FISH;
using entity::EntityTypeKeys::TURTLE;
using entity::EntityTypeKeys::WITCH;
using entity::EntityTypeKeys::WOLF;
using entity::EntityTypeKeys::ZOMBIE;
using entity::EntityTypeKeys::ZOMBIE_HORSE;
using entity::EntityTypeKeys::ZOMBIE_VILLAGER;
using entity::EntityTypeKeys::ZOMBIFIED_PIGLIN;

/// BiomeDefaultFeatures.caveSpawns(builder)：蝙蝠（AMBIENT 10,8,8）+ 发光鱿鱼（UNDERGROUND_WATER_CREATURE 10,4,6）。
void _addCaveSpawns(MobSpawnInfo& info)
{
    info.addAmbientSpawn(SpawnEntry(BAT, 10, 8, 8));
    info.addUndergroundWaterCreatureSpawn(SpawnEntry(GLOW_SQUID, 10, 4, 6));
}

/// BiomeDefaultFeatures.monsters(builder, zombieWeight, zombieVillagerWeight, zombieHorseWeight, skeletonWeight)，
/// drownedInsteadOfZombie 恒为 false。僵尸马仅在 zombieHorseWeight > 0 时加入（1.21.11 中仅平原系使用）。
void _addMonsters(
    MobSpawnInfo& info, i32 zombieWeight, i32 zombieVillagerWeight, i32 zombieHorseWeight, i32 skeletonWeight)
{
    info.addMonsterSpawn(SpawnEntry(SPIDER, 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ZOMBIE, zombieWeight, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ZOMBIE_VILLAGER, zombieVillagerWeight, 1, 1));
    if (zombieHorseWeight > 0) {
        info.addMonsterSpawn(SpawnEntry(ZOMBIE_HORSE, zombieHorseWeight, 1, 1));
    }
    info.addMonsterSpawn(SpawnEntry(SKELETON, skeletonWeight, 4, 4));
    info.addMonsterSpawn(SpawnEntry(CREEPER, 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry(SLIME, 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ENDERMAN, 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry(WITCH, 5, 1, 1));
}

/// BiomeDefaultFeatures.commonSpawns(builder[, skeletonWeight=100])：caveSpawns + monsters(95, 5, 0, skeletonWeight)。
void _addCommonSpawns(MobSpawnInfo& info, i32 skeletonWeight = 100)
{
    _addCaveSpawns(info);
    _addMonsters(info, 95, 5, 0, skeletonWeight);
}

/// BiomeDefaultFeatures.commonSpawnWithZombieHorse(builder)：caveSpawns + monsters(90, 5, 5, 100)。
void _addCommonSpawnWithZombieHorse(MobSpawnInfo& info)
{
    _addCaveSpawns(info);
    _addMonsters(info, 90, 5, 5, 100);
}

/// BiomeDefaultFeatures.farmAnimals(builder)：羊、猪、鸡、牛。
void _addFarmAnimals(MobSpawnInfo& info)
{
    info.addCreatureSpawn(SpawnEntry(SHEEP, 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry(PIG, 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry(CHICKEN, 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry(COW, 8, 4, 4));
}

/// BiomeDefaultFeatures.baseJungleSpawns(builder)：farmAnimals + 额外鸡 + commonSpawns。
void _addBaseJungleSpawns(MobSpawnInfo& info)
{
    _addFarmAnimals(info);
    info.addCreatureSpawn(SpawnEntry(CHICKEN, 10, 4, 4));
    _addCommonSpawns(info);
}

/// BiomeDefaultFeatures.swampSpawns(builder, skeletonWeight)：commonSpawns(skeletonWeight) + 沼泽特有条目。
void _addSwampSpawns(MobSpawnInfo& info, i32 skeletonWeight)
{
    _addCommonSpawns(info, skeletonWeight);
    info.addMonsterSpawn(SpawnEntry(SLIME, 1, 1, 1));
    info.addMonsterSpawn(SpawnEntry(BOGGED, 30, 4, 4));
    info.addCreatureSpawn(SpawnEntry(FROG, 10, 2, 5));
}

/// BiomeDefaultFeatures.oceanSpawns(builder, squidWeight, squidPack, codWeight)：水生生物 + commonSpawns + 溺尸。
void _addOceanSpawns(MobSpawnInfo& info, i32 squidWeight, i32 squidPack, i32 codWeight)
{
    info.addWaterCreatureSpawn(SpawnEntry(SQUID, squidWeight, 1, squidPack));
    info.addWaterAmbientSpawn(SpawnEntry(COD, codWeight, 3, 6));
    _addCommonSpawns(info);
    info.addMonsterSpawn(SpawnEntry(DROWNED, 5, 1, 1));
}

/// BiomeDefaultFeatures.warmOceanSpawns(builder, squidWeight, squidPack)：暖水海洋的水生条目 + commonSpawns。
void _addWarmOceanSpawns(MobSpawnInfo& info, i32 squidWeight, i32 squidPack)
{
    info.addWaterCreatureSpawn(SpawnEntry(SQUID, squidWeight, squidPack, 4));
    info.addWaterAmbientSpawn(SpawnEntry(TROPICAL_FISH, 25, 8, 8));
    info.addWaterCreatureSpawn(SpawnEntry(DOLPHIN, 2, 1, 2));
    info.addMonsterSpawn(SpawnEntry(DROWNED, 5, 1, 1));
    _addCommonSpawns(info);
}

/// BiomeDefaultFeatures.snowySpawns(builder, snowy)：依赖 isSnowy 的两个分支。
void _addSnowySpawns(MobSpawnInfo& info, bool snowy)
{
    info.addCreatureSpawn(SpawnEntry(RABBIT, 10, 2, 3));
    info.addCreatureSpawn(SpawnEntry(POLAR_BEAR, 1, 1, 2));
    _addCaveSpawns(info);
    _addMonsters(info, snowy ? 90 : 95, 5, snowy ? 5 : 0, 20);
    info.addMonsterSpawn(SpawnEntry(STRAY, 80, 4, 4));
}

/// OverworldBiomes.meadowOrCherryGrove(cherryGrove)：onlyPig 决定首个条目是猪还是驴。
void _addMeadowOrCherryGroveSpawns(MobSpawnInfo& info, bool cherryGrove)
{
    info.addCreatureSpawn(SpawnEntry(cherryGrove ? PIG : DONKEY, 1, 1, 2));
    info.addCreatureSpawn(SpawnEntry(RABBIT, 2, 2, 6));
    info.addCreatureSpawn(SpawnEntry(SHEEP, 2, 2, 4));
    _addCommonSpawns(info);
}

/// 生物群系通用的上限设置：怪物 70、动物 10、环境生物 15、水下生物 5 等（与原版 MobCategory 默认一致）。
void _setDefaultMaxInstances(MobSpawnInfo& info)
{
    info.setMaxMonsterInstances(MobSpawnInfo::DEFAULT_MAX_MONSTERS);
    info.setMaxCreatureInstances(MobSpawnInfo::DEFAULT_MAX_CREATURES);
    info.setMaxAmbientInstances(MobSpawnInfo::DEFAULT_MAX_AMBIENT);
    info.setMaxWaterCreatureInstances(MobSpawnInfo::DEFAULT_MAX_WATER_CREATURES);
    info.setMaxWaterAmbientInstances(MobSpawnInfo::DEFAULT_MAX_WATER_AMBIENT);
    info.setMaxAxolotlInstances(MobSpawnInfo::DEFAULT_MAX_AXOLOTLS);
    info.setMaxUndergroundWaterCreatureInstances(MobSpawnInfo::DEFAULT_MAX_UNDERGROUND_WATER_CREATURES);
}

} // namespace

MobSpawnInfo MobSpawnInfo::createPlains()
{
    // OverworldBiomes.plains(sunflower=false, snowy=false, spike=false) → BiomeDefaultFeatures.plainsSpawns：
    //   farmAnimals + horse(5,2,6) + donkey(1,1,3) + commonSpawnWithZombieHorse。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    info.addCreatureSpawn(SpawnEntry(HORSE, 5, 2, 6));
    info.addCreatureSpawn(SpawnEntry(DONKEY, 1, 1, 3));
    _addCommonSpawnWithZombieHorse(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createFlowerForest()
{
    // OverworldBiomes.forest(birch=false, tallBirch=false, flower=true)：farmAnimals + commonSpawns
    //   + rabbit(4,2,3)（花森林专属，替代普通森林的狼）。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    _addCommonSpawns(info);
    info.addCreatureSpawn(SpawnEntry(RABBIT, 4, 2, 3));

    return info;
}

MobSpawnInfo MobSpawnInfo::createBirchForest()
{
    // OverworldBiomes.forest(birch=true, ..., flower=false)：farmAnimals + commonSpawns，
    //   桦木森林无狼、无兔（仅普通森林追加 wolf(5,4,4)）。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createForest()
{
    // OverworldBiomes.forest(birch=false, ..., flower=false)：farmAnimals + commonSpawns + wolf(5,4,4)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    _addCommonSpawns(info);
    info.addCreatureSpawn(SpawnEntry(WOLF, 5, 4, 4));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDarkForest()
{
    // OverworldBiomes.darkForest：farmAnimals + commonSpawns（无狼、无兔）。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addFarmAnimals(info);
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createPaleGarden()
{
    // OverworldBiomes.darkForest(paleGarden=true)：不调用 farmAnimals，仅 commonSpawns。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createDesert()
{
    // OverworldBiomes.desert → BiomeDefaultFeatures.desertSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(RABBIT, 12, 2, 3));
    // TODO(spawn-list-entities): camel（CREATURE 1,1,1）——CamelEntity 未实现。

    _addCaveSpawns(info);
    // desertSpawns 不走 commonSpawns，而是直接 monsters(19, 1, 0, 50)。
    _addMonsters(info, 19, 1, 0, 50);
    info.addMonsterSpawn(SpawnEntry(HUSK, 80, 4, 4));
    // TODO(spawn-list-entities): parched（MONSTER 50,4,4）——ParchedEntity 未实现。

    return info;
}

MobSpawnInfo MobSpawnInfo::createBadlands()
{
    // OverworldBiomes.badlands(wooded=false)：farmAnimals + commonSpawns + armadillo(6,1,2)；
    //   creatureGenerationProbability 0.03。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.03f;

    _addFarmAnimals(info);
    _addCommonSpawns(info);
    info.addCreatureSpawn(SpawnEntry(ARMADILLO, 6, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createWoodedBadlands()
{
    // OverworldBiomes.badlands(wooded=true)：在普通恶地基础上加 wolf(2,4,8)，概率提升到 0.04。
    MobSpawnInfo info = createBadlands();
    info.m_creatureSpawnProbability = 0.04f;
    info.addCreatureSpawn(SpawnEntry(WOLF, 2, 4, 8));
    return info;
}

MobSpawnInfo MobSpawnInfo::createOcean()
{
    // OverworldBiomes.ocean：oceanSpawns(1,4,10) + dolphin(1,1,2) + nautilus(5,1,1)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addOceanSpawns(info, 1, 4, 10);
    info.addWaterCreatureSpawn(SpawnEntry(DOLPHIN, 1, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 5, 1, 1));

    return info;
}

MobSpawnInfo MobSpawnInfo::createWarmOcean()
{
    // OverworldBiomes.warmOcean：pufferfish(15,1,3) + nautilus(5,1,1) + warmOceanSpawns(10,4)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addWaterAmbientSpawn(SpawnEntry(PUFFERFISH, 15, 1, 3));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 5, 1, 1));
    _addWarmOceanSpawns(info, 10, 4);

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepWarmOcean()
{
    // 深海暖水海洋：与暖水海洋的刷怪表一致（deep 参数只影响地形生成）。
    return createWarmOcean();
}

MobSpawnInfo MobSpawnInfo::createLukewarmOcean()
{
    // OverworldBiomes.lukeWarmOcean：pufferfish(5,1,3) + tropical_fish(25,8,8) + dolphin(2,1,2)
    //   + nautilus(5,1,1) + oceanSpawns(10,2,15)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addWaterAmbientSpawn(SpawnEntry(PUFFERFISH, 5, 1, 3));
    info.addWaterAmbientSpawn(SpawnEntry(TROPICAL_FISH, 25, 8, 8));
    info.addWaterCreatureSpawn(SpawnEntry(DOLPHIN, 2, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 5, 1, 1));
    _addOceanSpawns(info, 10, 2, 15);

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepLukewarmOcean()
{
    // OverworldBiomes.lukeWarmOcean(deep=true)：其余条目与浅水一致，oceanSpawns 的 squid/cod 参数为 (8,4,8)。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addWaterAmbientSpawn(SpawnEntry(PUFFERFISH, 5, 1, 3));
    info.addWaterAmbientSpawn(SpawnEntry(TROPICAL_FISH, 25, 8, 8));
    info.addWaterCreatureSpawn(SpawnEntry(DOLPHIN, 2, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 5, 1, 1));
    _addOceanSpawns(info, 8, 4, 8);

    return info;
}

MobSpawnInfo MobSpawnInfo::createColdOcean()
{
    // OverworldBiomes.coldOcean：oceanSpawns(3,4,15) + salmon(15,1,5) + nautilus(2,1,1)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addOceanSpawns(info, 3, 4, 15);
    info.addWaterAmbientSpawn(SpawnEntry(SALMON, 15, 1, 5));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 2, 1, 1));

    return info;
}

MobSpawnInfo MobSpawnInfo::createFrozenOcean()
{
    // OverworldBiomes.frozenOcean：squid(1,1,4) + salmon(15,1,5) + polar_bear(1,1,2) + nautilus(2,1,1)
    //   + commonSpawns + drowned(5,1,1)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addWaterCreatureSpawn(SpawnEntry(SQUID, 1, 1, 4));
    info.addWaterAmbientSpawn(SpawnEntry(SALMON, 15, 1, 5));
    info.addCreatureSpawn(SpawnEntry(POLAR_BEAR, 1, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry(NAUTILUS, 2, 1, 1));
    info.addMonsterSpawn(SpawnEntry(DROWNED, 5, 1, 1));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepOcean()
{
    // 深海：与普通海洋的刷怪表一致（deep 参数只影响地形生成）。
    return createOcean();
}

MobSpawnInfo MobSpawnInfo::createTaiga()
{
    // OverworldBiomes.taiga(oldGrowth=false, snowy=false)：farmAnimals + wolf(8,4,4) + rabbit(4,2,3)
    //   + fox(8,2,4) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    info.addCreatureSpawn(SpawnEntry(WOLF, 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry(RABBIT, 4, 2, 3));
    info.addCreatureSpawn(SpawnEntry(FOX, 8, 2, 4));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createJungle()
{
    // OverworldBiomes.jungle：baseJungleSpawns + parrot(40,1,2) + ocelot(MONSTER 2,1,3) + panda(1,1,2)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addBaseJungleSpawns(info);
    info.addCreatureSpawn(SpawnEntry(PARROT, 40, 1, 2));
    info.addMonsterSpawn(SpawnEntry(OCELOT, 2, 1, 3));
    info.addCreatureSpawn(SpawnEntry(PANDA, 1, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSparseJungle()
{
    // OverworldBiomes.sparseJungle：baseJungleSpawns + wolf(8,2,4)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addBaseJungleSpawns(info);
    info.addCreatureSpawn(SpawnEntry(WOLF, 8, 2, 4));

    return info;
}

MobSpawnInfo MobSpawnInfo::createBambooJungle()
{
    // OverworldBiomes.bambooJungle：baseJungleSpawns + parrot(40,1,2) + panda(80,1,2) + ocelot(MONSTER 2,1,1)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addBaseJungleSpawns(info);
    info.addCreatureSpawn(SpawnEntry(PARROT, 40, 1, 2));
    info.addCreatureSpawn(SpawnEntry(PANDA, 80, 1, 2));
    info.addMonsterSpawn(SpawnEntry(OCELOT, 2, 1, 1));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSavanna(bool llamaAndWolves)
{
    // OverworldBiomes.savanna(llamaAndWolves)：horse(1,2,6) + donkey(1,1,1) + armadillo(10,2,3)
    //   + commonSpawnWithZombieHorse；llamaAndWolves 时另加 llama(8,4,4) + wolf(4,8)。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    info.addCreatureSpawn(SpawnEntry(HORSE, 1, 2, 6));
    info.addCreatureSpawn(SpawnEntry(DONKEY, 1, 1, 1));
    info.addCreatureSpawn(SpawnEntry(ARMADILLO, 10, 2, 3));
    if (llamaAndWolves) {
        info.addCreatureSpawn(SpawnEntry(LLAMA, 8, 4, 4));
        info.addCreatureSpawn(SpawnEntry(WOLF, 8, 4, 8));
    }
    _addCommonSpawnWithZombieHorse(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSavannaPlateau()
{
    // OverworldBiomes.savanna(llamaAndWolves=true)
    return createSavanna(true);
}

MobSpawnInfo MobSpawnInfo::createShatteredSavanna()
{
    // 破碎热带草原：刷怪表与普通热带草原一致（shattered 参数只影响地形与地表方块）。
    return createSavanna(false);
}

MobSpawnInfo MobSpawnInfo::createSwamp()
{
    // OverworldBiomes.swamp：farmAnimals + swampSpawns(70)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addFarmAnimals(info);
    _addSwampSpawns(info, 70);

    return info;
}

MobSpawnInfo MobSpawnInfo::createMangroveSwamp()
{
    // OverworldBiomes.mangroveSwamp：swampSpawns(70)（不含 farmAnimals）+ tropical_fish(25,8,8)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addSwampSpawns(info, 70);
    info.addWaterAmbientSpawn(SpawnEntry(TROPICAL_FISH, 25, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createMountains()
{
    // OverworldBiomes.windsweptHills：farmAnimals + llama(5,4,6) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    _addFarmAnimals(info);
    info.addCreatureSpawn(SpawnEntry(LLAMA, 5, 4, 6));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createWindsweptForest()
{
    // OverworldBiomes.windsweptHills(withTrees=true)：刷怪表与疾风丘陵一致，仅树木特征不同。
    return createMountains();
}

MobSpawnInfo MobSpawnInfo::createGoatPeaks()
{
    // OverworldBiomes.basePeaks（尖峭峰 / 冰封峰 / 裸岩峰）：goat(5,1,3) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(GOAT, 5, 1, 3));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowySlopes()
{
    // OverworldBiomes.snowySlopes：rabbit(4,2,3) + goat(5,1,3) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(RABBIT, 4, 2, 3));
    info.addCreatureSpawn(SpawnEntry(GOAT, 5, 1, 3));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createGrove()
{
    // OverworldBiomes.grove：wolf(1,1,1) + rabbit(8,2,3) + fox(4,2,4) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(WOLF, 1, 1, 1));
    info.addCreatureSpawn(SpawnEntry(RABBIT, 8, 2, 3));
    info.addCreatureSpawn(SpawnEntry(FOX, 4, 2, 4));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowy()
{
    // OverworldBiomes.plains(snowy=true) → snowySpawns(true)：rabbit(10,2,3) + polar_bear(1,1,2)
    //   + caveSpawns + monsters(90,5,5,20) + stray(80,4,4)；creatureGenerationProbability 0.07。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.07f;

    _addSnowySpawns(info, true);

    return info;
}

MobSpawnInfo MobSpawnInfo::createMushroomFields()
{
    // OverworldBiomes.mushroomFields → BiomeDefaultFeatures.mooshroomSpawns：
    //   mooshroom(CREATURE 8,4,8) + caveSpawns。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(MOOSHROOM, 8, 4, 8));
    _addCaveSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowyBeach()
{
    // OverworldBiomes.beach(snowy=true)：无海龟（仅普通沙滩），其余为 commonSpawns。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createBeach()
{
    // OverworldBiomes.beach(snowy=false, stony=false)：turtle(5,2,5) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addCreatureSpawn(SpawnEntry(TURTLE, 5, 2, 5));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createStoneShore()
{
    // OverworldBiomes.beach(snowy=false, stony=true)：无海龟、无生物生成，仅有 commonSpawns。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createRiver()
{
    // OverworldBiomes.river(frozen=false)：squid(2,1,4) + salmon(5,1,5) + commonSpawns + drowned(100,1,1)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addWaterCreatureSpawn(SpawnEntry(SQUID, 2, 1, 4));
    info.addWaterAmbientSpawn(SpawnEntry(SALMON, 5, 1, 5));
    _addCommonSpawns(info);
    info.addMonsterSpawn(SpawnEntry(DROWNED, 100, 1, 1));

    return info;
}

MobSpawnInfo MobSpawnInfo::createFrozenRiver()
{
    // OverworldBiomes.river(frozen=true)：溺尸权重降为 1，其余条目与普通河流一致。
    MobSpawnInfo info = createRiver();
    for (auto& entry : info.m_monsters.entries) {
        if (entry.entityTypeId == entity::EntityTypeKeys::DROWNED) {
            entry.weight = 1;
        }
    }
    return info;
}

MobSpawnInfo MobSpawnInfo::createEmpty()
{
    // 空生成信息：没有生物自然生成的生物群系（虚空、深暗之域）。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.0f;
    return info;
}

MobSpawnInfo MobSpawnInfo::createNetherWastes()
{
    // NetherBiomes.netherWastes
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(GHAST, 50, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ZOMBIFIED_PIGLIN, 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry(MAGMA_CUBE, 2, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ENDERMAN, 1, 4, 4));
    info.addMonsterSpawn(SpawnEntry(PIGLIN, 15, 4, 4));
    info.addCreatureSpawn(SpawnEntry(STRIDER, 60, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSoulSandValley()
{
    // NetherBiomes.soulSandValley：全部条目附带 mobCharge（energyBudget 0.7 / charge 0.15）
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(SKELETON, 20, 5, 5));
    info.addMonsterSpawn(SpawnEntry(GHAST, 50, 4, 4));
    info.addMonsterSpawn(SpawnEntry(ENDERMAN, 1, 4, 4));
    info.addCreatureSpawn(SpawnEntry(STRIDER, 60, 1, 2));
    info.setSpawnCost(SKELETON, SpawnCosts(0.7, 0.15));
    info.setSpawnCost(GHAST, SpawnCosts(0.7, 0.15));
    info.setSpawnCost(ENDERMAN, SpawnCosts(0.7, 0.15));
    info.setSpawnCost(STRIDER, SpawnCosts(0.7, 0.15));

    return info;
}

MobSpawnInfo MobSpawnInfo::createCrimsonForest()
{
    // NetherBiomes.crimsonForest
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(ZOMBIFIED_PIGLIN, 1, 2, 4));
    info.addMonsterSpawn(SpawnEntry(HOGLIN, 9, 3, 4));
    info.addMonsterSpawn(SpawnEntry(PIGLIN, 5, 3, 4));
    info.addCreatureSpawn(SpawnEntry(STRIDER, 60, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createWarpedForest()
{
    // NetherBiomes.warpedForest：末影人附带 mobCharge（energyBudget 1.0 / charge 0.12）
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(ENDERMAN, 1, 4, 4));
    info.addCreatureSpawn(SpawnEntry(STRIDER, 60, 1, 2));
    info.setSpawnCost(ENDERMAN, SpawnCosts(1.0, 0.12));

    return info;
}

MobSpawnInfo MobSpawnInfo::createBasaltDeltas()
{
    // NetherBiomes.basaltDeltas
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(GHAST, 40, 1, 1));
    info.addMonsterSpawn(SpawnEntry(MAGMA_CUBE, 100, 2, 5));
    info.addCreatureSpawn(SpawnEntry(STRIDER, 60, 1, 2));

    return info;
}

// ============================================================================
// 末地生物群系生成信息
// ============================================================================

MobSpawnInfo MobSpawnInfo::createTheEnd()
{
    // BiomeDefaultFeatures.endSpawns（末地全部群系共用）：仅末影人（4,4），权重 10；
    //   末地群系不设置 creatureGenerationProbability，保持 Builder 默认值 0.1。
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addMonsterSpawn(SpawnEntry(ENDERMAN, 10, 4, 4));

    return info;
}

MobSpawnInfo MobSpawnInfo::createLushCaves()
{
    // OverworldBiomes.lushCaves：axolotl(AXOLOTLS 10,4,6) + tropical_fish(WATER_AMBIENT 25,8,8) + commonSpawns
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    info.addAxolotlSpawn(SpawnEntry(AXOLOTL, 10, 4, 6));
    info.addWaterAmbientSpawn(SpawnEntry(TROPICAL_FISH, 25, 8, 8));
    _addCommonSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createDripstoneCaves()
{
    // OverworldBiomes.dripstoneCaves → BiomeDefaultFeatures.dripstoneCavesSpawns：
    //   caveSpawns + monsters(95,5,0,100) + drowned(95,4,4)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;

    _addCaveSpawns(info);
    _addMonsters(info, 95, 5, 0, 100);
    info.addMonsterSpawn(SpawnEntry(DROWNED, 95, 4, 4));

    return info;
}

MobSpawnInfo MobSpawnInfo::createMeadow()
{
    // OverworldBiomes.meadowOrCherryGrove(cherryGrove=false)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    _addMeadowOrCherryGroveSpawns(info, false);
    return info;
}

MobSpawnInfo MobSpawnInfo::createCherryGrove()
{
    // OverworldBiomes.meadowOrCherryGrove(cherryGrove=true)
    MobSpawnInfo info;
    _setDefaultMaxInstances(info);
    info.m_creatureSpawnProbability = 0.1f;
    _addMeadowOrCherryGroveSpawns(info, true);
    return info;
}

} // namespace spawn
} // namespace world
} // namespace mc