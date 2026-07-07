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
    // 依据：Minecraft Wiki wolf 生成群系表，Forest weight=5/45、pack=4。
    // 源码此前误注"森林没有狼"导致漏配，此处对齐原版补回 wolf。
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:cow", 8, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:wolf", 5, 4, 4));
    // 森林有额外的鸡
    // TODO: 下方重复的 chicken 条目疑似与上方重复（1.16.5 forest chicken 仅一条 weight 10），
    //       待统一核对原版 Biome spawn list 后收敛，暂不动以避免影响本次 wolf 修复范围。
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createDesert()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

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
    // 暖水海洋：热带鱼和河豚为主，鳕鱼、鲑鱼、海豚
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

    // 怪物（标准 + 溺尸）
    info.setMaxMonsterInstances(DEFAULT_MAX_MONSTERS);
    info.addMonsterSpawn(SpawnEntry("minecraft:drowned", 5, 1, 1));

    // 水生生物（暖水海洋配置）
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 15, 3, 6));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 10, 1, 2));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:pufferfish", 5, 1, 3));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:tropical_fish", 25, 8, 8));
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:dolphin", 2, 1, 2));

    return info;
}

MobSpawnInfo MobSpawnInfo::createLukewarmOcean()
{
    // 温水海洋：混合鱼群
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;

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

MobSpawnInfo MobSpawnInfo::createJungle()
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
    info.addMonsterSpawn(SpawnEntry("minecraft:ocelot", 2, 1, 1)); // 丛林豹猫是怪物分类

    // 动物（丛林没有牛！）
    info.setMaxCreatureInstances(DEFAULT_MAX_CREATURES);
    info.addCreatureSpawn(SpawnEntry("minecraft:sheep", 12, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:pig", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:chicken", 10, 4, 4));
    info.addCreatureSpawn(SpawnEntry("minecraft:parrot", 40, 1, 2));
    info.addCreatureSpawn(SpawnEntry("minecraft:panda", 1, 1, 2)); // 竹林变体是80

    // 环境生物
    info.addAmbientSpawn(SpawnEntry("minecraft:bat", 10, 8, 8));

    return info;
}

MobSpawnInfo MobSpawnInfo::createSavanna()
{
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.1f;
    info.m_playerSpawnFriendly = true;

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
    info.setSpawnCost("minecraft:skeleton", SpawnCosts(0.12, 0.7));
    info.setSpawnCost("minecraft:ghast", SpawnCosts(0.12, 0.7));
    info.setSpawnCost("minecraft:enderman", SpawnCosts(0.12, 0.7));

    // 生物（炽足兽）
    info.setMaxCreatureInstances(10);
    info.addCreatureSpawn(SpawnEntry("minecraft:strider", 60, 1, 2));
    info.setSpawnCost("minecraft:strider", SpawnCosts(0.12, 0.7));

    return info;
}

MobSpawnInfo MobSpawnInfo::createCrimsonForest()
{
    // 生物：猪灵、僵尸猪灵、疣猪兽、炽足兽
    MobSpawnInfo info;
    info.m_creatureSpawnProbability = 0.0f;

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
