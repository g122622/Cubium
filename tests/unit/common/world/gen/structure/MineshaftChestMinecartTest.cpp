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

/**
 * @file MineshaftChestMinecartTest.cpp
 * @brief 废弃矿井走廊的宝箱矿车测试
 *
 * 走廊的 1% 宝箱分支在原版里会放置一辆带战利品表的宝箱矿车。世界生成阶段没有实体容器，
 * 项目把实体记进 ChunkPrimer 的待生成实体表，区块发布后再由 ServerWorld 创建，
 * 因此这里断言「待生成实体表里出现了宝箱矿车，并带着正确的战利品表与种子」。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/MineshaftStructure.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::block_registry;
using namespace mc::world::gen::structure;

namespace {

/**
 * @brief 可编排返回值的随机源
 *
 * 走廊的宝箱分支由若干 `nextInt` 控制，用固定脚本把「命中 1%」这件事变成确定行为，
 * 不必靠种子搜索。
 */
class ScriptedRandom final : public math::IRandom {
public:
    void setSeed(u64 seed) override
    {
        m_seed = seed;
    }

    [[nodiscard]] u64 nextU64() override
    {
        return static_cast<u64>(nextInt());
    }

    [[nodiscard]] math::PositionalRandomFactory forkPositional() override
    {
        return math::JavaLegacyRandom(static_cast<i64>(m_seed)).forkPositional();
    }

    [[nodiscard]] i32 nextInt() override
    {
        return 0;
    }

    /// 默认返回 0：1% 判定（`nextInt(100) == 0`）因此必然命中。
    [[nodiscard]] i32 nextInt(i32 /*bound*/) override
    {
        return 0;
    }

    [[nodiscard]] bool nextBoolean() override
    {
        return false;
    }

    [[nodiscard]] f32 nextFloat() override
    {
        return 0.0f;
    }

    [[nodiscard]] f64 nextDouble() override
    {
        return 0.0;
    }

    [[nodiscard]] i64 nextLong() override
    {
        return m_nextLong;
    }

    /// 后续 `nextLong()` 固定返回该值，便于断言战利品表种子。
    void setNextLong(i64 value)
    {
        m_nextLong = value;
    }

private:
    u64 m_seed = 0;
    i64 m_nextLong = 0;
};

} // namespace

class MineshaftChestMinecartTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                // 普通地形：y<60 实心石头、y>=60 空气。走廊会把 y=60..62 掏成洞穴空气，
                // 于是宝箱位置的「上方是空气、下方非空气」判定成立。
                for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                        for (i32 y = 40; y < 60; ++y) {
                            chunk->setBlockState(x, y, z, &VanillaBlocks::STONE->defaultState());
                        }
                        for (i32 y = 60; y <= 64; ++y) {
                            chunk->setBlockState(x, y, z, &VanillaBlocks::AIR->defaultState());
                        }
                    }
                }
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }
        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, m_chunks);
    }

    [[nodiscard]] ChunkPrimer& centerPrimer()
    {
        return *m_ownedChunks[(1 + 1) * 3 + (1 + 1)];
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(MineshaftChestMinecartTest, CorridorSpawnsChestMinecartWithMineshaftLootTable)
{
    // 方向 0 = North/South：走廊沿 Z 轴展开，段数取 Z 跨度；边长 20 给出 4 段，宝箱分支必然执行。
    ScriptedRandom rng;
    rng.setNextLong(0x5EED1234LL);

    MineshaftCorridor corridor(0, rng, 0, 60, 0, 2, 62, 20, 0, MineshaftType::Normal);
    ChunkPrimer& primer = centerPrimer();
    const StructureBoundingBox chunkBounds(0, 50, 0, 15, 70, 15);

    corridor.generate(*m_region, rng, 0, 0, chunkBounds, &primer, nullptr);


    const auto& spawned = primer.spawnedEntities();
    ASSERT_FALSE(spawned.empty()) << "the 1% chest branch must record a chest minecart";

    const auto minecart = std::find_if(spawned.begin(), spawned.end(), [](const SpawnedEntityData& data) {
        return data.entityTypeId == "minecraft:chest_minecart";
    });
    ASSERT_NE(minecart, spawned.end());

    // 战利品表必须是废弃矿井的宝箱表，且种子来自 nextLong()。
    ASSERT_TRUE(minecart->nbt.has_value());
    const auto& nbt = minecart->nbt.value();
    ASSERT_EQ(nbt.value.count(entity::serialization::nbt_keys::LOOT_TABLE), 1u);
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(
                  *nbt.value.at(entity::serialization::nbt_keys::LOOT_TABLE))
                  .value,
        "minecraft:chests/abandoned_mineshaft");
    ASSERT_EQ(nbt.value.count(entity::serialization::nbt_keys::LOOT_TABLE_SEED), 1u);
    EXPECT_EQ(dynamic_cast<const nbt::tags::long_tag&>(
                  *nbt.value.at(entity::serialization::nbt_keys::LOOT_TABLE_SEED))
                  .value,
        0x5EED1234LL);

    // 实体位置是方块中心（原版 setInitialPos(x+0.5, y+0.5, z+0.5)）。
    EXPECT_NEAR(minecart->x - std::floor(minecart->x), 0.5f, 1e-4f);
    EXPECT_NEAR(minecart->y - std::floor(minecart->y), 0.5f, 1e-4f);
    EXPECT_NEAR(minecart->z - std::floor(minecart->z), 0.5f, 1e-4f);
}

TEST_F(MineshaftChestMinecartTest, CorridorWithoutPrimerOnlyConsumesRandomStream)
{
    // 不带 ChunkPrimer 的调用方（结构探针）拿不到待生成实体表：随机流照常消费，但不产生实体。
    ScriptedRandom rng;
    MineshaftCorridor corridor(0, rng, 0, 60, 0, 2, 62, 20, 0, MineshaftType::Normal);
    const StructureBoundingBox chunkBounds(0, 50, 0, 15, 70, 15);

    corridor.generate(*m_region, rng, 0, 0, chunkBounds, nullptr, nullptr);

    for (const auto& chunk : m_ownedChunks) {
        EXPECT_TRUE(chunk->spawnedEntities().empty());
    }
}
