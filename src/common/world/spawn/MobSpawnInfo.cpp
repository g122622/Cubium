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
//   1) EntityClassification 枚举仅实现 6 类（Monster/Creature/Ambient/
//      WaterCreature/WaterAmbient/Misc），缺失原版的 UndergroundWaterCreature
//      与 Axolotls 两类。导致繁茂洞穴的美西螈、深海守卫者、水下洞穴鱼群等
//      无法归入正确分类，目前临时塞进 WaterCreature。
//   2) 多个 1.16.5 实体未注册（parched、camel、nautilus、glow_squid、bogged、
//      armadillo、zombie_horse 等），无法加入对应 spawn list。
//   3) hoglin 的 EntityClassification 归类与原版不一致（hoglin 原版为 Creature，
//      本项目为 Monster）。
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
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-desert): 与原版 1.16.5 偏差：
    //   1) rabbit 原版 weight=4, pack=2-3，与当前一致，OK。
    //   2) husk 原版 weight=80, pack=4，与当前一致，OK；zombie weight=19、
    //      zombie_villager weight=1，与当前一致，OK。
    //   3) Desert 变体（DesertHills/DesertLakes）原版 spawn list 与 Desert 一致，
    //      但本项目未区分变体，未来若新增变体需复用本工厂。
    //   4) 待实现 bogged（1.21+）后无需加入 Desert，1.16.5 暂无。

    // 怪物（沙漠没有末影人和女巫）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 19, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 1, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:husk", 80, 4, 4)); // 沙漠僵尸

    // 动物（沙漠只有兔子）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 4, 2, 3));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createOcean()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-ocean): 与原版 1.16.5 偏差：
    //   1) 普通海洋 monster list 原版仅 drowned（weight=5, pack=1），不包含
    //      zombie/skeleton/creeper/spider/slime/enderman/witch。这些是陆地怪物，
    //      在海洋水面不应生成。当前误加了陆地怪物列表。
    //   2) WaterCreature 缺少 tropical_fish（原版普通海洋无）、dolphin（原版普通海洋有，
    //      weight=2, pack=1-2）。
    //   3) 缺少 WaterAmbient 分类条目（cod/salmon 原版归 WaterAmbient 而非 WaterCreature，
    //      本项目全部塞进 WaterCreature，分类与原版不一致）。

    // 怪物（海洋有溺尸）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 水生生物
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 15, 3, 6));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 3, 1, 4));

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
    // 温水海洋：混合鱼群
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-lukewarm-ocean): 与原版 1.16.5 偏差：
    //   1) cod/salmon/tropical_fish/pufferfish 原版归 WaterAmbient，当前误归 WaterCreature。
    //   2) squid/dolphin 原版归 WaterCreature，与当前一致，OK。
    //   3) 怪物 list 原版仅 drowned（weight=100, pack=4），与当前一致，OK。

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 100, 4, 4));

    // 水生生物（温水海洋混合配置）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 10, 3, 6));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 5, 1, 5));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:tropical_fish", 10, 8, 8));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:pufferfish", 5, 1, 3));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createColdOcean()
{
    // 冷水海洋：更多鲑鱼
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-cold-ocean): 与原版 1.16.5 偏差：
    //   1) cod/salmon 原版归 WaterAmbient，当前误归 WaterCreature。
    //   2) squid/dolphin 原版归 WaterCreature，与当前一致，OK。
    //   3) 怪物 list 原版仅 drowned（weight=100, pack=4），与当前一致，OK。

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 100, 4, 4));

    // 水生生物（冷水海洋配置）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 10, 3, 6));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5)); // 更高权重
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 1, 4));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createFrozenOcean()
{
    // 冰冻海洋：鲑鱼为主，北极熊，没有鳕鱼！
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-frozen-ocean): 与原版 1.16.5 偏差：
    //   1) salmon 原版归 WaterAmbient，当前误归 WaterCreature。
    //   2) squid 原版归 WaterCreature，与当前一致，OK。
    //   3) 怪物 list 原版仅 drowned（weight=5, pack=1），当前误加陆地怪物
    //      （zombie/skeleton/creeper/spider/slime/enderman/witch）。
    //   4) polar_bear 原版归 Creature，与当前一致，OK。
    //   5) 原版 FrozenOcean 在冰面生成 stray，当前仅在注释中提及未实现。
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
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));
    // 注意：流浪者(Stray)只在冰面上生成，不在水中

    // 水生生物（冰冻海洋没有鳕鱼！）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 1, 1, 4));

    // 动物（冰面上的北极熊）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:polar_bear", 1, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDeepOcean()
{
    // 深海：更多鱿鱼
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-deep-ocean): 与原版 1.16.5 偏差：
    //   1) cod/salmon 原版归 WaterAmbient，当前误归 WaterCreature。
    //   2) squid/dolphin 原版归 WaterCreature，与当前一致，OK。
    //   3) 怪物 list 原版仅 drowned（weight=100, pack=4），与当前一致，OK。
    //   4) 深海原版有 guardian（水下守卫者）生成，归 WaterCreature（待确认），
    //      当前缺失，待 guardian 实体实现后补回。
    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 100, 4, 4));

    // 水生生物（深海更多鱿鱼）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 10, 3, 6));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 5, 1, 5));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 1, 8)); // 深海鱿鱼更多
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createTaiga()
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

    // 动物（针叶林有狐狸和狼）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:fox", 8, 2, 4));

    // 环境生物
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
 * commonSpawns = caveSpawns(bat AMBIENT 10,8,8 + glow_squid UNDERGROUND_WATER_CREATURE
 * 10,4,6) + monsters(spider 100,4,4 + zombie 95,4,4 + zombie_villager 5,1,1 +
 * skeleton 100,4,4 + creeper 100,4,4 + slime 100,4,4 + enderman 10,1,4 + witch 5,1,1)。
 *
 * 注：glow_squid 归 UNDERGROUND_WATER_CREATURE 分类，本项目暂未实现该分类，
 * glow_squid 实体亦未注册，故此处暂不添加（见文件顶 TODO(spawn-list-alignment) #1/#2）。
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
    // 对应 MC 1.16.5 OverworldBiomes.sparseJungle()（旧名 JungleEdge）：
    //   baseJungleSpawns + wolf(8,2,4) CREATURE
    // 稀疏丛林没有 ocelot/parrot/panda，取而代之的是狼。
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    applyBaseJungleSpawns(info);

    // sparseJungle 变体特定条目
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 8, 2, 4));

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

MobSpawnInfo MobSpawnInfo::createSavanna()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

    // TODO(spawn-list-savanna): 与原版 1.16.5 偏差：
    //   1) llama 原版仅在 SavannaPlateau/ShatteredSavanna 变体生成（weight=8, pack=4），
    //      普通 Savanna 不应有 llama，当前误加。
    //   2) horse 原版 weight=1, pack=2-6，donkey 原版 weight=1, pack=1，与当前一致，OK。
    //   3) 原版 Savanna 无 zombie_villager，当前 monster list 也未加，OK。
    //   4) 待实现 armadillo（1.20.5+）后需加入 Savanna spawn list，1.16.5 暂无。

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（热带草原有马、驴、羊驼）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:horse", 1, 2, 6));
    info.addCreatureSpawn(SpawnEntry("minecraft:donkey", 1, 1, 1));
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 8, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSwamp()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物（沼泽有更多女巫）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));
    // 注意：沼泽还有额外的史莱姆生成，通过特定条件检测实现
    // 在沼泽群系的史莱姆区块和 Y=50-70 之间会额外生成史莱姆

    // 动物
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createMountains()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（山地有羊驼）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:llama", 5, 4, 6));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSnowy()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-snowy): 与原版 1.16.5 偏差：
    //   1) snowy 生物群系原版 creatureSpawnProbability 应为 0.07（雪原更稀疏），
    //      当前误用 0.1。SnowyTundra/SnowyMountains/SnowyBeaches 等变体均应核对。
    //   2) 雪地 animal list 原版只有 rabbit（weight=10, pack=2-3）和 polar_bear
    //      （weight=1, pack=1-2），与当前一致，OK。但 SnowyBeaches 有 polar_bear
    //      无 rabbit，变体未区分。
    //   3) stray 原版 weight=80, pack=4，与当前一致，OK；skeleton 原版 weight=20，
    //      与当前一致，OK。
    // 怪物（雪地有流浪者）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie_villager", 5, 1, 1));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 20, 4, 4)); // 雪地骷髅权重较低
    info.addMonsterSpawn(SpawnEntry("minecraft:stray", 80, 4, 4));    // 流浪者
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 动物（雪地有北极熊和兔子）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:rabbit", 10, 2, 3));
    info.addCreatureSpawn(SpawnEntry("minecraft:polar_bear", 1, 1, 2));

    // 环境生物
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
    // 生物：猪灵、僵尸猪灵、疣猪兽、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

    // TODO(spawn-list-crimson-forest): 与原版 1.16.5 偏差：
    //   1) hoglin 原版归 Creature（weight=9, pack=3-4），本项目误归 Monster。
    //      原版 CrimsonForest creature list 为 hoglin，monster list 为
    //      zombified_piglin/piglin。待 hoglin EntityClassification 修正后迁移。
    //   2) zombified_piglin 原版 weight=1, pack=2-4，与当前一致，OK。
    //   3) piglin 原版 weight=5, pack=3-4，与当前一致，OK。
    //   4) strider 原版归 Creature（weight=60, pack=1-2），与当前一致，OK。

    // 怪物（注意：疣猪兽是 MONSTER 分类）
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
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // TODO(spawn-list-lush-caves): 与原版 1.16.5 偏差：
    //   1) axolotl 原版归 Axolotls 分类（独立分类，maxCount=5），本项目缺失该分类，
    //      当前临时塞进 WaterCreature。待 EntityClassification 扩展 Axolotls 后迁移。
    //   2) tropical_fish 原版归 WaterAmbient，与当前一致，OK。
    //   3) 繁茂洞穴原版无独立 monster list（继承自宿主生物群系），当前硬编码了
    //      通用洞穴怪物，可能与宿主群系不一致。
    //   4) glow_squid 原版在 LushCaves 应生成（weight=10, pack=2-4），当前缺失，
    //      待 glow_squid 实体实现后补回。
    // 怪物（普通洞穴怪物）
    info.setMaxMonsterInstances(70);
    info.addMonsterSpawn(SpawnEntry("minecraft:creeper", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:skeleton", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:slime", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:spider", 100, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:zombie", 95, 4, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:enderman", 10, 1, 4));
    info.addMonsterSpawn(SpawnEntry("minecraft:witch", 5, 1, 1));

    // 水生生物：美西螈
    info.setMaxWaterCreatureInstances(5);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:axolotl", 10, 4, 6));

    // 水生环境生物：热带鱼
    info.setMaxWaterAmbientInstances(20);
    info.addWaterAmbientSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));

    // 环境生物：蝙蝠
    info.setMaxAmbientInstances(15);
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

} // namespace mc::world::spawn
