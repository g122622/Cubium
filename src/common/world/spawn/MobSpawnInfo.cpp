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

namespace mc::world::spawn {

// ============================================================================
// 工厂方法实现
// ============================================================================
//
// TODO(spawn-list-alignment): 本文件下各工厂方法的 spawn list 与原版 MC Java 1.16.5
// 仍存在若干偏差，逐项列在对应工厂方法内（搜索关键词 "TODO(spawn-list" 即可定位）。
// 已知共性偏差汇总：
//   1) 多个 1.16.5 实体未注册（parched、camel、bogged、
//      armadillo、zombie_horse 等），无法加入对应 spawn list。
// 收敛上述任一项时，请同步删除对应的 TODO 注释。

MobSpawnInfo MobSpawnInfo::createPlains()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物（怪物在夜间/黑暗环境自然生成）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（区块生成时放置）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:horse", 5, 2, 6));
    info.addCreatureSpawn(SpawnEntry("minecraft:donkey", 1, 1, 3));

    // 环境生物
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createForest()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（MC Java 1.16.5 forest creature spawn list：sheep/pig/cow/chicken/wolf）
    // 依据：原版 BiomeGenerationSettings 默认 farmAnimals + wolf（Forest weight=5/45、pack=4）。
    // 源码此前误注"森林没有狼"导致漏配，且误将 Jungle 的"额外鸡"规则套到 Forest 上，
    // 导致 Forest 出现两条 weight=10 的 chicken 条目。已收敛为原版单条。
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 5, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDesert()
{
    // 对应 MC 1.16.5 BiomeMaker.func_244220_a(0.125F, 0.05F, true, true, true)
    //   → DefaultBiomeFeatures.func_243743_f()（desertSpawns）：
    //     RABBIT (4,2,3) CREATURE
    //     + caveSpawns: BAT (10,8,8) AMBIENT
    //     + monsters(builder, 19, 1, 100):
    //         spider 100,4,4 + zombie 19,4,4 + zombie_villager 1,1,1 +
    //         skeleton 100,4,4 + creeper 100,4,4 + slime 100,4,4 +
    //         enderman 10,1,4 + witch 5,1,1
    //     + HUSK (80,4,4) MONSTER
    //   注：1.21+ 新增的 camel/parched 在 1.16.5 中不存在。
    //   Desert 变体（DesertHills/DesertLakes）spawn list 与 Desert 一致。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：monsters(builder, 19, 1, 100) + husk
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 19, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 1, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:husk", 80, 4, 4)); // 沙漠僵尸

    // 动物（沙漠只有兔子）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 4, 2, 3));

    // 环境生物（caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createOcean()
{
    // 普通海洋：对应 MC 1.16.5 BiomeMaker.func_244234_c(false)（浅水版本）
    //   func_243716_a(builder, 1, 4, 10):
    //     SQUID (1,1,4) WATER_CREATURE
    //     COD (10,3,6) WATER_AMBIENT
    //     + commonSpawns(builder): BAT (10,8,8) AMBIENT + monsters(_, 95, 5, 100): 8 陆地怪物
    //     + DROWNED (5,1,1) MONSTER
    //   额外：DOLPHIN (1,1,2) WATER_CREATURE
    //   注意：原版普通海洋 monster list 包含 8 条标准陆地怪物 + drowned（来自 commonSpawns→monsters + oceanSpawns 加
    //   drowned）。 原版 1.16.5 无 glow_squid（1.21 新增）；nautilus 在 1.21.11 加入海洋生物群系。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(builder, 95, 5, 100, false)）+ drowned
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin + nautilus（原版归 WaterCreature）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 1, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 1, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:nautilus", 5, 1, 1));

    // 水生环境生物：cod（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:cod", 10, 3, 6));

    return info;
}

MobSpawnInfo MobSpawnInfo::createWarmOcean()
{
    // 暖水海洋浅水版本：对应 MC 1.16.5 BiomeMaker.func_244249_o()
    //   PUFFERFISH (15,1,3) WATER_AMBIENT
    //   + warmOceanSpawns(builder, 10, 4):
    //       SQUID (10,4,4) WATER_CREATURE
    //       TROPICAL_FISH (25,8,8) WATER_AMBIENT
    //       DOLPHIN (2,1,2) WATER_CREATURE
    //       + commonSpawns(builder):
    //           BAT (10,8,8) AMBIENT
    //           + monsters(builder, 95, 5, 100): 8 条标准陆地怪物
    //   浅水版本无 drowned、无 cod、无 salmon。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：标准 8 条陆地怪物列表（commonSpawns → func_243735_b(_, 95, 5, 100)）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（原版归 WaterCreature）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 4, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:nautilus", 5, 1, 1));

    // 水生环境生物：pufferfish + tropical_fish（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:pufferfish", 15, 1, 3));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepWarmOcean()
{
    // 深海暖水海洋：对应 MC 1.16.5 BiomeMaker.func_244250_p()
    //   warmOceanSpawns(builder, 5, 1):
    //       SQUID (5,1,4) WATER_CREATURE
    //       TROPICAL_FISH (25,8,8) WATER_AMBIENT
    //       DOLPHIN (2,1,2) WATER_CREATURE
    //       + commonSpawns(builder): BAT + 8 条标准陆地怪物
    //   + DROWNED (5,1,1) MONSTER
    //   深水版本无 pufferfish、无 cod、无 salmon，但有 drowned；squid 权重 5、minCount 1。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：标准 8 条陆地怪物列表（commonSpawns → func_243735_b(_, 95, 5, 100)）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    // 深水版本额外添加 drowned
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（深水 squid 权重更低、minCount=1）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 5, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    // 水生环境生物：仅 tropical_fish（深水版本无 pufferfish）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createLukewarmOcean()
{
    // 温水海洋：对应 MC 1.16.5 BiomeMaker.func_244237_d(false)（浅水版本）
    //   func_243716_a(builder, 10, 2, 15):
    //     SQUID (10,1,2) WATER_CREATURE
    //     COD (15,3,6) WATER_AMBIENT
    //     + commonSpawns: BAT + 8 陆地怪物
    //     + DROWNED (5,1,1) MONSTER
    //   额外：PUFFERFISH (5,1,3) WATER_AMBIENT, TROPICAL_FISH (25,8,8) WATER_AMBIENT,
    //         DOLPHIN (2,1,2) WATER_CREATURE
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物 + drowned
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（原版归 WaterCreature）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:nautilus", 5, 1, 1));

    // 水生环境生物：cod + pufferfish + tropical_fish（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:cod", 15, 3, 6));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:pufferfish", 5, 1, 3));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepLukewarmOcean()
{
    // 深海温水海洋：对应 MC 1.16.5 BiomeMaker.func_244237_d(true)（深水版本）
    //   与浅水版本差异：func_243716_a(builder, 8, 4, 8) → SQUID(8,1,4) WC + COD(8,3,6) WA
    //   （浅水版本为 10, 2, 15 → SQUID(10,1,2) WC + COD(15,3,6) WA）
    //   其余条目（pufferfish/tropical_fish/dolphin/drowned/8 陆地怪物/bat）与浅水版本相同。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物 + drowned
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（深水版本 squid 权重 8、minCount 4）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 8, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    // 水生环境生物：cod + pufferfish + tropical_fish（深水版本 cod 权重 8）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:cod", 8, 3, 6));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:pufferfish", 5, 1, 3));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createColdOcean()
{
    // 冷水海洋：对应 MC 1.16.5 BiomeMaker.func_244230_b(false)（浅水版本）
    //   func_243716_a(builder, 3, 4, 15):
    //     SQUID (3,1,4) WATER_CREATURE
    //     COD (15,3,6) WATER_AMBIENT
    //     + commonSpawns: BAT + 8 陆地怪物
    //     + DROWNED (5,1,1) MONSTER
    //   额外：SALMON (15,1,5) WATER_AMBIENT, DOLPHIN (2,1,2) WATER_CREATURE
    //   注：DEEP_COLD_OCEAN = func_244230_b(true)，spawn list 与浅水版本相同。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物 + drowned
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（原版归 WaterCreature）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 3, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:nautilus", 2, 1, 1));

    // 水生环境生物：cod + salmon（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:cod", 15, 3, 6));
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5));

    return info;
}

MobSpawnInfo MobSpawnInfo::createFrozenOcean()
{
    // 冰冻海洋：对应 MC 1.16.5 BiomeMaker.func_244239_e(false)（浅水版本）
    //   不调用 oceanSpawns，手动构造：
    //     SQUID (1,1,4) WATER_CREATURE
    //     SALMON (15,1,5) WATER_AMBIENT
    //     POLAR_BEAR (1,1,2) CREATURE
    //     + commonSpawns: BAT + 8 陆地怪物
    //     + DROWNED (5,1,1) MONSTER
    //   注意：原版 FrozenOcean 怪物 list 包含 8 条标准陆地怪物 + drowned（不是仅 drowned）。
    //   原版 1.16.5 不在 FrozenOcean 加 stray（stray 由 snowySpawns 添加，FrozenOcean 不调用）。
    //   原版 1.16.5 无 cod（冷水太冷）、无 tropical_fish、无 pufferfish、无 dolphin。
    //   注：DEEP_FROZEN_OCEAN = func_244239_e(true)，spawn list 与浅水版本相同。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物 + drowned（commonSpawns → monsters(_, 95, 5, 100, false) + 显式 drowned）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + nautilus（原版归 WaterCreature，无 dolphin）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 1, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:nautilus", 2, 1, 1));

    // 水生环境生物：salmon（原版归 WaterAmbient，无 cod）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5));

    // 动物（冰面上的北极熊）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:polar_bear", 1, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepOcean()
{
    // 深海：对应 MC 1.16.5 BiomeMaker.func_244234_c(true)（深水版本）
    //   spawn list 与普通 Ocean 完全一致（func_244234_c 的 deep 参数仅影响 generation settings）：
    //     func_243716_a(builder, 1, 4, 10):
    //       SQUID (1,1,4) WATER_CREATURE
    //       COD (10,3,6) WATER_AMBIENT
    //       + commonSpawns: BAT + 8 陆地怪物
    //       + DROWNED (5,1,1) MONSTER
    //     额外：DOLPHIN (1,1,2) WATER_CREATURE
    //   注意：原版 1.16.5 DeepOcean 不含 guardian（守卫者只在 OceanMonument 周围生成，由结构生成器处理，
    //   不在生物群系 spawn list 中）。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物 + drowned
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    // 水生生物：squid + dolphin（原版归 WaterCreature）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 1, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 1, 1, 2));

    // 水生环境生物：cod（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(DEFAULT_MAX_WATER_AMBIENT);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:cod", 10, 3, 6));

    return info;
}

MobSpawnInfo MobSpawnInfo::createTaiga()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(5, TAIGA, BiomeMaker.func_244221_a(0.2F, 0.2F, false, false, true,
    // false))
    //   → func_244221_a 内部：
    //     func_243714_a: farmAnimals（sheep/pig/chicken/cow）
    //     + WOLF (8,4,4) CREATURE + RABBIT (4,2,3) CREATURE + FOX (8,2,4) CREATURE
    //     + func_243737_c: commonSpawns（BAT + 8 陆地怪物）
    //     + setPlayerSpawnFriendly（!p_244221_2_ && !p_244221_3_ = !false && !false = true）
    //   Taiga 变体（SnowyTaiga/GiantTreeTaiga 等）spawn list 与 Taiga 一致，仅气候/地形不同。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(_, 95, 5, 100)）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物：farmAnimals + wolf + rabbit + fox
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 4, 2, 3));
    info.addCreatureSpawn(SpawnEntry("minecraft:fox", 8, 2, 4));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

namespace {
/**
 * @brief 填充 baseJungleSpawns 公共生成列表
 *
 * 对应 MC 1.16.5 BiomeDefaultFeatures.baseJungleSpawns()：
 *   farmAnimals() + 额外 chicken(10,4,4) CREATURE + commonSpawns()
 * 其中 farmAnimals = sheep(12,4,4) + pig(10,4,4) + chicken(10,4,4) + cow(8,4,4)；
 * commonSpawns = caveSpawns(bat AMBIENT 10,8,8) + monsters(spider 100,4,4 +
 * zombie 95,4,4 + zombie_villager 5,1,1 + skeleton 100,4,4 + creeper 100,4,4 +
 * slime 100,4,4 + enderman 10,1,4 + witch 5,1,1)。
 *
 * 注：1.18+ 的 caveSpawns 额外含 glow_squid UNDERGROUND_WATER_CREATURE (10,4,6)，
 * 本项目对齐 1.16.5 故不含 glow_squid。UndergroundWaterCreature 分类与 glow_squid
 * 实体已在 LushCaves 等生物群系中启用。
 *
 * @param info 待填充的 MobSpawnInfo（调用方已设置好概率等基础字段）
 */
void applyBaseJungleSpawns(MobSpawnInfo& info)
{
    // 怪物（commonSpawns -> monsters）
    info.setMaxMonsterInstances(MobSpawnInfo::DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（farmAnimals + 额外 chicken）
    info.setMaxCreatureInstances(MobSpawnInfo::DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    // baseJungleSpawns 显式追加的"额外鸡"（weight=10, pack=4）
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));

    // 环境生物（caveSpawns -> bat）
    info.setMaxAmbientInstances(MobSpawnInfo::DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));
}
} // namespace

MobSpawnInfo MobSpawnInfo::createJungle()
{
    // 对应 MC 1.16.5 OverworldBiomes.jungle()：
    //   baseJungleSpawns + parrot(40,1,2) CREATURE + ocelot(2,1,3) MONSTER
    //   + panda(1,1,2) CREATURE
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseJungleSpawns(info);

    // jungle 变体特定条目
    info.addMonsterSpawn(SpawnEntry("minecraft:ocelot", 2, 1, 3));
    info.addCreatureSpawn(SpawnEntry("minecraft:parrot", 40, 1, 2));
    info.addCreatureSpawn(SpawnEntry("minecraft:panda", 1, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSparseJungle()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(23, JUNGLE_EDGE, BiomeMaker.func_244227_b())
    //   func_244227_b 内部仅调用 func_243747_h（baseJungleSpawns），不额外添加 wolf/parrot/ocelot/panda。
    //   注：1.21.11 sparseJungle() 额外添加了 wolf(8,2,4)，但 1.16.5 中无 wolf。本项目对齐 1.16.5。
    //   ModifiedJungleEdge (func_244231_c) 也仅调用 baseJungleSpawns，spawn list 与 JungleEdge 相同。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseJungleSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createBambooJungle()
{
    // 对应 MC 1.16.5 OverworldBiomes.bambooJungle()：
    //   baseJungleSpawns + parrot(40,1,2) CREATURE + panda(80,1,2) CREATURE
    //   + ocelot(2,1,1) MONSTER
    // 与普通 jungle 的差异：panda weight=80（非 1）、ocelot pack=1（非 3）。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseJungleSpawns(info);

    // bambooJungle 变体特定条目
    info.addMonsterSpawn(SpawnEntry("minecraft:ocelot", 2, 1, 1));
    info.addCreatureSpawn(SpawnEntry("minecraft:parrot", 40, 1, 2));
    info.addCreatureSpawn(SpawnEntry("minecraft:panda", 80, 1, 2));

    return info;
}

namespace {
/// 热带草原系列基础 spawn list（对应 MC 1.16.5 BiomeMaker.func_244258_x()：farmAnimals +
/// horse/donkey + commonSpawns）。Savanna / ShatteredSavanna / ShatteredSavannaPlateau 共享此基础。
/// SavannaPlateau 通过 func_244247_m() 在此基础上额外添加 llama(8,4,4)（无 wolf）。
void applyBaseSavannaSpawns(MobSpawnInfo& info)
{
    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(builder, 95, 5, 100)）
    info.setMaxMonsterInstances(MobSpawnInfo::DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物：farmAnimals + horse + donkey
    info.setMaxCreatureInstances(MobSpawnInfo::DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:horse", 1, 2, 6));
    info.addCreatureSpawn(SpawnEntry("minecraft:donkey", 1, 1, 1));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(MobSpawnInfo::DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));
}
} // namespace

MobSpawnInfo MobSpawnInfo::createSavanna()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(35, SAVANNA, BiomeMaker.func_244211_a(0.125F, 0.05F, 1.2F, false,
    // false))
    //   func_244211_a 内部调用 func_244258_x()：farmAnimals + horse(1,2,6) + donkey(1,1,1) + commonSpawns（8 陆地怪物 +
    //   bat）。 不含 llama/wolf（仅 SavannaPlateau 通过 func_244247_m() 加 llama，无 wolf）。 原版 1.16.5 无
    //   armadillo（1.20.5 新增）。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseSavannaSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSavannaPlateau()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(36, SAVANNA_PLATEAU, BiomeMaker.func_244247_m())
    //   func_244247_m() 在 func_244258_x() 基础上额外添加 llama(8,4,4)（CREATURE 分类）。
    //   注：1.16.5 中仅 SavannaPlateau 加 llama，无 wolf；ShatteredSavannaPlateau 不加任何额外条目。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseSavannaSpawns(info);

    // 高原变体特定条目
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 8, 4, 4));

    return info;
}

MobSpawnInfo MobSpawnInfo::createShatteredSavanna()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(163, SHATTERED_SAVANNA,
    // BiomeMaker.func_244211_a(0.3625F, 1.225F, 1.1F, true, true))
    //   func_244211_a 内部仅调用 func_244258_x()，与普通 Savanna spawn list 相同（无 llama/wolf）。
    //   shattered=true 仅影响地形与地表方块（generation settings），不影响 spawn list。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseSavannaSpawns(info);

    return info;
}

MobSpawnInfo MobSpawnInfo::createSwamp()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(6, SWAMP, BiomeMaker.func_244236_d(-0.2F, 0.1F, false))
    //   → func_244236_d 内部：
    //     func_243714_a: farmAnimals（sheep/pig/chicken/cow）
    //     + func_243737_c: commonSpawns（BAT + 8 陆地怪物）
    //     + SLIME (1,1,1) MONSTER（额外低权重史莱姆，区别于 commonSpawns 中的 slime(100,4,4)）
    //   注：沼泽还有额外的史莱姆生成机制（在沼泽群系的史莱姆区块和 Y=50-70 之间会额外生成史莱姆），
    //   该机制由 SlimeChunkChecker 与 NaturalSpawner 协同处理，不在 spawn list 中。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(_, 95, 5, 100)）+ 额外 slime
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 1, 1, 1)); // 沼泽额外低权重史莱姆

    // 动物：farmAnimals（无 horse/donkey）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createMountains()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(3, MOUNTAINS, BiomeMaker.func_244216_a(1.0F, 0.5F, ..., false))
    //   → func_244216_a 内部：
    //     func_243714_a: farmAnimals（sheep/pig/chicken/cow）
    //     + LLAMA (5,4,6) CREATURE
    //     + func_243737_c: commonSpawns（BAT + 8 陆地怪物）
    //   Mountains 变体（WoodedMountains/GravellyMountains 等）spawn list 与 Mountains 一致。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(_, 95, 5, 100)）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物：farmAnimals + llama（山地有羊驼）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 5, 4, 6));

    // 环境生物（commonSpawns → caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowy()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(12, SNOWY_TUNDRA, BiomeMaker.func_244219_a())
    //   → DefaultBiomeFeatures.func_243741_e()（snowySpawns）：
    //     RABBIT (10,2,3) CREATURE
    //     POLAR_BEAR (1,1,2) CREATURE
    //     + caveSpawns: BAT (10,8,8) AMBIENT
    //     + monsters(builder, 95, 5, 20):
    //         spider 100,4,4 + zombie 95,4,4 + zombie_villager 5,1,1 +
    //         skeleton 20,4,4 + creeper 100,4,4 +
    //         slime 100,4,4 + enderman 10,1,4 + witch 5,1,1
    //     + STRAY (80,4,4) MONSTER
    //   creatureGenerationProbability = 0.07F（雪原更稀疏）
    //   注：原版 1.16.5 snowySpawns 不含 zombie_horse（1.16.5 中无任何生物群系 spawn list 含 zombie_horse）。
    //   变体：IceSpikes spawn list 与 SnowyTundra 完全一致（func_244239_b → func_244219_a），
    //   复用本方法；SnowyBeach 差异较大（无 creature、skeleton 权重 100、无 stray），
    //   使用独立的 createSnowyBeach() 工厂方法。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.07f;

    // 怪物：snowySpawns → monsters(builder, 95, 5, 20) + stray
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 20, 4, 4)); // 雪地骷髅权重较低
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:stray", 80, 4, 4)); // 流浪者

    // 动物（雪地有北极熊和兔子）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 10, 2, 3));
    info.addCreatureSpawn(SpawnEntry("minecraft:polar_bear", 1, 1, 2));

    // 环境生物（caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowyBeach()
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(26, SNOWY_BEACH,
    //   BiomeMaker.func_244208_a(0.0F, 0.025F, 0.05F, 0.3F, true, false, false))
    //   → 不添加 TURTLE（因 p_244208_5_=true 表示雪地沙滩，原版 beach 仅在 p_244208_5_=false 时加 TURTLE）
    //   → DefaultBiomeFeatures.func_243737_c()（commonSpawns）：
    //     caveSpawns: BAT (10,8,8) AMBIENT
    //     monsters(builder, 95, 5, 100): 8 条标准陆地怪物（skeleton 权重 100，无 stray）
    //   与 SnowyTundra(createSnowy) 差异：
    //     - skeleton 权重 100（vs SnowyTundra 20）
    //     - 无 stray（SnowyTundra 有 stray 80）
    //     - 无 creature 分类（SnowyTundra 有 rabbit + polar_bear）
    //     - creatureSpawnProbability 使用默认 0.1F（SnowyTundra 为 0.07F）
    //   1.16.5 与 1.21.11 spawn list 完全相同（1.21.11 仅额外含 glow_squid，
    //   本项目对齐 1.16.5 不含）。
    MobSpawnInfo info;
    // creatureSpawnProbability 使用默认 0.1f（commonSpawns 不修改概率）

    // 怪物：commonSpawns → monsters(builder, 95, 5, 100)
    // 注意 skeleton 权重为 100（与标准陆地一致），无 stray
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 环境生物（caveSpawns → bat）
    info.setMaxAmbientInstances(DEFAULT_MAX_AMBIENT);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createEmpty()
{
    // 空生成信息，用于没有生物生成的生物群系（如虚空）
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;
    return info;
}

// ============================================================================
// 下界生物群系生成信息
// ============================================================================

MobSpawnInfo MobSpawnInfo::createNetherWastes()
{
    // 生物：猪灵、僵尸猪灵、恶魂、岩浆怪、末影人、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f; // 下界没有被动动物生成

    // 怪物
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:ghast", 50, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombified_piglin", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:magma_cube", 2, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:piglin", 15, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 1, 4, 4));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSoulSandValley()
{
    // 生物：骷髅、恶魂、末影人、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // 怪物
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 20, 5, 5));
    info.addMonsterSpawn(SpawnEntry("minecraft:ghast", 50, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 1, 4, 4));

    // 灵魂沙谷所有怪物都有 SpawnCosts
    // 原版 1.16.5：灵魂沙谷 energyBudget=0.15，charge=0.7
    // 此前误用 0.12（与 WarpedForest 混淆），已收敛为原版 0.15
    info.setSpawnCost("minecraft:skeleton", SpawnCosts(0.15, 0.7));
    info.setSpawnCost("minecraft:ghast", SpawnCosts(0.15, 0.7));
    info.setSpawnCost("minecraft:enderman", SpawnCosts(0.15, 0.7));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));
    info.setSpawnCost("minecraft:strider", SpawnCosts(0.15, 0.7));

    return info;
}

MobSpawnInfo MobSpawnInfo::createCrimsonForest()
{
    // 对应 MC 1.16.5 NetherBiomes.crimsonForest()
    //   MONSTER: zombified_piglin (1,2,4), hoglin (9,3,4), piglin (5,3,4)
    //   CREATURE: strider (60,1,2)
    //   注意：原版 1.16.5 中 hoglin 实体类继承 AnimalEntity（Cubium 注册为 Creature），
    //   但 CrimsonForest 的 spawn list 把 hoglin 放入 MONSTER 分类。这是 MC 原版的
    //   设计：下界生物群系中 hoglin 视为敌对生物。Cubium 的实现与原版一致。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // 怪物：zombified_piglin + hoglin + piglin（hoglin 在下界 spawn list 中归 Monster）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombified_piglin", 1, 2, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:hoglin", 9, 3, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:piglin", 5, 3, 4));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createWarpedForest()
{
    // 生物：末影人、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // 怪物（末影人为主）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 1, 4, 4));
    info.setSpawnCost("minecraft:enderman", SpawnCosts(0.12, 1.0));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createBasaltDeltas()
{
    // 生物：岩浆怪、恶魂、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // 怪物（岩浆怪为主）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:ghast", 40, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:magma_cube", 100, 2, 5));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));

    return info;
}

// ============================================================================
// 末地生物群系生成信息
// ============================================================================

MobSpawnInfo MobSpawnInfo::createTheEnd()
{
    // 只有末影人
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // 怪物（只有末影人）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 4, 4));

    return info;
}

// ============================================================================
// 洞穴生物群系生成信息
// ============================================================================

MobSpawnInfo MobSpawnInfo::createLushCaves()
{
    // 对应 MC 1.18+ OverworldBiomes.lushCaves()（1.16.5 不存在此生物群系，按 1.18 配置实现）
    //   AXOLOTLS: axolotl (10,4,6)
    //   WATER_AMBIENT: tropical_fish (25,8,8)
    //   commonSpawns: BAT (10,8,8) AMBIENT + glow_squid (10,4,6) UNDERGROUND_WATER_CREATURE
    //                 + monsters(builder, 95, 5, 100, false): 8 陆地怪物
    //   注意：LushCaves 原版无独立 monster list（继承自宿主生物群系），但 Cubium 工厂方法
    //   必须返回完整 MobSpawnInfo，故硬编码了通用洞穴怪物。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物：8 条标准陆地怪物（commonSpawns → monsters(builder, 95, 5, 100, false)）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 美西螈归独立 Axolotls 分类（maxCount=5），与 WaterCreature 分开计数，
    // 避免与鱿鱼、海豚等竞争 5 个名额。
    info.setMaxAxolotlInstances(DEFAULT_MAX_AXOLOTLS);
    info.addAxolotlSpawn(SpawnEntry("minecraft:axolotl", 10, 4, 6));

    // 地下水生生物：发光鱿鱼（原版 commonSpawns 中的 glow_squid (10,4,6)）
    info.setMaxUndergroundWaterCreatureInstances(DEFAULT_MAX_UNDERGROUND_WATER_CREATURES);
    info.addUndergroundWaterCreatureSpawn(SpawnEntry("minecraft:glow_squid", 10, 4, 6));

    // 水生环境生物：热带鱼（原版归 WaterAmbient）
    info.setMaxWaterAmbientInstances(20);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    // 环境生物：蝙蝠（caveSpawns → bat）
    info.setMaxAmbientInstances(15);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

} // namespace mc::world::spawn
