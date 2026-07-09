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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// 数据驱动生物生成配置（spawners / creature_spawn_probability）解析对齐测试。
//
// 原版 biome JSON 的顶层字段 creature_spawn_probability 决定动物生成概率
// （MobSpawnSettings.getCreatureProbability()），snowy_plains/badlands/ice_spikes
// 等为 0.07，其余默认 0.1。本项目 BiomeLoader.applySpawners 用一个全新的
// MobSpawnInfo 覆盖 BiomeFactory 设的默认值时，既未从 JSON 解析
// creature_spawn_probability，也把工厂方法设的 0.07 丢回默认 0.1。本测试验证
// 该字段被正确解析并落到 spawnInfo.getCreatureSpawnProbability()。

#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::world::biome;
using namespace mc::world::spawn;

namespace {

class BiomeLoaderSpawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // BiomeRegistry::initialize 通过 BiomeFactory 构造全部默认 Biome，
        // snowy_plains 的 MobSpawnInfo.creatureSpawnProbability 默认被工厂设为 0.07f。
        BiomeRegistry::instance().initialize();
    }
};

// 构造一个最小可用的 biome JSON：仅含 creature_spawn_probability + 一个 spawner 条目。
nlohmann::json makeBiomeJson(f32 creatureProbability, const nlohmann::json& spawners = {})
{
    nlohmann::json j;
    j["creature_spawn_probability"] = creatureProbability;
    if (!spawners.is_null()) {
        j["spawners"] = spawners;
    }
    return j;
}

} // namespace

// ========== creature_spawn_probability 必须从 JSON 解析到 spawnInfo ==========

TEST_F(BiomeLoaderSpawnTest, CreatureSpawnProbabilityParsedFromJson)
{
    // snowy_plains 原版值为 0.07
    nlohmann::json j = makeBiomeJson(0.07f);
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "snowy_plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::SnowyPlains);
    EXPECT_FLOAT_EQ(biome.spawnInfo().getCreatureSpawnProbability(), 0.07f)
        << "creature_spawn_probability 应从 JSON 解析，而非被全新 MobSpawnInfo 重置为默认 0.1";
}

// 即使没有 spawners 字段，仅有 creature_spawn_probability 也应被解析。
// （applySpawners 在无 spawners 字段时直接 return，不应跳过概率字段。）
TEST_F(BiomeLoaderSpawnTest, CreatureSpawnProbabilityParsedWithoutSpawners)
{
    nlohmann::json j;
    j["creature_spawn_probability"] = 0.07f;
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "snowy_plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::SnowyPlains);
    EXPECT_FLOAT_EQ(biome.spawnInfo().getCreatureSpawnProbability(), 0.07f);
}

// 默认（无 creature_spawn_probability 字段）应为 0.1（与原版默认一致）。
TEST_F(BiomeLoaderSpawnTest, DefaultCreatureSpawnProbabilityIsTenth)
{
    nlohmann::json j;
    j["spawners"] = nlohmann::json::object();
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::Plains);
    EXPECT_FLOAT_EQ(biome.spawnInfo().getCreatureSpawnProbability(), 0.1f);
}

// ========== Biome 独立字段与 MobSpawnInfo 字段应统一为单一来源 ==========
// 原版只有 MobSpawnSettings.getCreatureProbability() 一个来源。项目曾同时存在
// Biome::m_creatureSpawnProbability 与 MobSpawnInfo::m_creatureSpawnProbability，
// 不同读取点取不同字段导致不一致。统一后两者应指向同一值。

TEST_F(BiomeLoaderSpawnTest, BiomeProbabilityMatchesSpawnInfoProbability)
{
    nlohmann::json j = makeBiomeJson(0.07f);
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "snowy_plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::SnowyPlains);
    // 统一后两字段必须一致（都来自 JSON 解析结果）
    EXPECT_FLOAT_EQ(biome.creatureSpawnProbability(), biome.spawnInfo().getCreatureSpawnProbability());
}

// ========== 无效 spawn 条目应被跳过而非崩溃 ==========

TEST_F(BiomeLoaderSpawnTest, InvalidSpawnEntrySkipped)
{
    nlohmann::json spawners;
    // 合法条目
    spawners["creature"] = nlohmann::json::array({
        // 有效：type + weight + minCount<=maxCount
        {{"type", "minecraft:pig"}, {"weight", 100}, {"minCount", 1}, {"maxCount", 4}},
        // 无效：minCount > maxCount → 应被跳过
        {{"type", "minecraft:cow"}, {"weight", 100}, {"minCount", 5}, {"maxCount", 2}},
        // 无效：weight <= 0 → 应被跳过
        {{"type", "minecraft:sheep"}, {"weight", 0}, {"minCount", 1}, {"maxCount", 4}},
        // 无效：缺 type → 应被跳过
        {{"weight", 100}, {"minCount", 1}, {"maxCount", 4}},
    });

    nlohmann::json j = makeBiomeJson(0.1f, spawners);
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::Plains);
    const auto& creatureSpawns = biome.spawnInfo().getCreatureSpawns();

    // 只有 pig 这一条合法条目应被保留
    ASSERT_EQ(creatureSpawns.size(), 1u);
    EXPECT_EQ(creatureSpawns[0].entityTypeId, "minecraft:pig");
}

// 合法的多分类 spawn 条目应全部解析。
TEST_F(BiomeLoaderSpawnTest, MultipleCategoriesParsed)
{
    nlohmann::json spawners;
    spawners["monster"] = nlohmann::json::array({
        {{"type", "minecraft:zombie"}, {"weight", 100}, {"minCount", 4}, {"maxCount", 4}},
    });
    spawners["creature"] = nlohmann::json::array({
        {{"type", "minecraft:pig"}, {"weight", 100}, {"minCount", 1}, {"maxCount", 4}},
        {{"type", "minecraft:cow"}, {"weight", 60}, {"minCount", 2}, {"maxCount", 4}},
    });

    nlohmann::json j = makeBiomeJson(0.1f, spawners);
    auto result = BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "plains"));
    ASSERT_TRUE(result.success());

    const auto& biome = BiomeRegistry::instance().getMutable(Biomes::Plains);
    EXPECT_EQ(biome.spawnInfo().getCreatureSpawns().size(), 2u);
    EXPECT_EQ(biome.spawnInfo().getMonsterSpawns().size(), 1u);
}
