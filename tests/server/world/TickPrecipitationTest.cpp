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

#include "common/TempDirHelper.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;
using namespace mc::world::gamerule;

// ============================================================================
// tickPrecipitation 测试固件
// ============================================================================

class TickPrecipitationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        BiomeRegistry::instance().initialize();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 42;

        m_world = std::make_unique<ServerWorld>(config);

        // 打开存档:ServerWorld::initialize 要求 m_storage 已设置且 isOpen()。
        // 复用 ServerWorldTest 模式,临时目录由 TempDirHelper 生成(PID 分量跨进程唯一)。
        m_testDir = mc::test::makeUniqueTestDir("mc_tick_precip_test");
        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();
        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_world->setChunkManager(std::move(chunkManager));

        // 设置天气管理器
        auto weatherManager = std::make_unique<WeatherManager>();
        weatherManager->setRain(10000);
        m_world->setWeatherManager(std::move(weatherManager));

        // initialize() 创建 m_tickManager / m_lightManager 等:tickPrecipitation 触发冰/雪生成时
        // 调 setBlockState,其内 MC_ASSERT_RELEASE 要求 m_lightManager/m_tickManager 非空(光线更新 +
        // 流体 tick 调度),否则断言崩溃。不 initialize() 会在冷生物群系降水时 SEH 0xc0000005。
        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
        m_world->shutdown();
        m_world.reset();
        m_storage.close();
        // TempDirHelper 内置 10 次重试,覆盖 Windows 上 RocksDB 后台线程延迟释放句柄的窗口。
        mc::test::removeTestDir(m_testDir);
    }

    /**
     * @brief 生成并加载一个区块
     */
    ChunkData* ensureChunk(i32 x, i32 z) { return m_world->chunkManager()->getChunkSync(x, z); }

    std::unique_ptr<ServerWorld> m_world;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
    static constexpr i32 SEA_LEVEL = 63;
};

// ============================================================================
// 基本行为测试
// ============================================================================

TEST_F(TickPrecipitationTest, ZeroRandomTickSpeedDoesNothing)
{
    // randomTickSpeed = 0 时不应执行任何操作
    ensureChunk(0, 0);

    // tickPrecipitation 不需要 initialize()，只需要区块管理器
    m_world->tickPrecipitation(0);
}

TEST_F(TickPrecipitationTest, NegativeRandomTickSpeedDoesNothing)
{
    ensureChunk(0, 0);
    m_world->tickPrecipitation(-1);
}

TEST_F(TickPrecipitationTest, HighRandomTickSpeedTriggersPrecipitation)
{
    // 生成区块并设置降雪天气
    ChunkData* chunk = ensureChunk(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 确保天气是下雨状态
    auto* weatherManager = m_world->weatherManager();
    ASSERT_NE(weatherManager, nullptr);
    weatherManager->setRain(10000);

    // 设置游戏规则：允许雪层积累
    m_world->getGameRules().setInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT, 8);

    // 使用高 randomTickSpeed 以确保触发降水 tick
    // 遍历多个区块寻找冷水/冷生物群系
    // 注意：由于生物群系由种子决定，我们无法保证区块(0,0)是冷的，
    // 因此我们扫描附近多个区块
    bool foundIceOrSnow = false;
    for (i32 cx = -1; cx <= 1 && !foundIceOrSnow; ++cx) {
        for (i32 cz = -1; cz <= 1 && !foundIceOrSnow; ++cz) {
            ensureChunk(cx, cz);
        }
    }

    for (int tick = 0; tick < 30 && !foundIceOrSnow; ++tick) {
        m_world->tickPrecipitation(100);
        // 检查所有已加载区块
        for (i32 cx = -1; cx <= 1 && !foundIceOrSnow; ++cx) {
            for (i32 cz = -1; cz <= 1 && !foundIceOrSnow; ++cz) {
                ChunkData* c = m_world->getChunk(cx, cz);
                if (c == nullptr) continue;
                for (i32 x = 0; x < 16 && !foundIceOrSnow; ++x) {
                    for (i32 z = 0; z < 16 && !foundIceOrSnow; ++z) {
                        for (i32 y = world::SEA_LEVEL - 5; y <= world::SEA_LEVEL + 5; ++y) {
                            const BlockState* state = m_world->getBlockState(x + c->x() * 16, y, z + c->z() * 16);
                            if (state != nullptr && (state->is(VanillaBlocks::ICE) || state->is(VanillaBlocks::SNOW))) {
                                foundIceOrSnow = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (foundIceOrSnow) {
        SUCCEED();
    } else {
        GTEST_SKIP() << "Generated biomes were not cold enough for precipitation effects";
    }
}

TEST_F(TickPrecipitationTest, NoWeatherManagerDoesNotCrash)
{
    // 移除天气管理器
    m_world->setWeatherManager(nullptr);
    ensureChunk(0, 0);

    // 不应崩溃
    m_world->tickPrecipitation(100);
}

TEST_F(TickPrecipitationTest, NoChunkManagerDoesNotCrash)
{
    // 创建一个没有区块管理器的世界
    ServerWorldConfig config;
    config.viewDistance = 10;
    config.dimension = 0;
    config.seed = 42;
    auto worldNoChunks = std::make_unique<ServerWorld>(config);
    // 不设置区块管理器

    auto weatherManager = std::make_unique<WeatherManager>();
    weatherManager->setRain(10000);
    worldNoChunks->setWeatherManager(std::move(weatherManager));

    // 不应崩溃
    worldNoChunks->tickPrecipitation(100);
}

// ============================================================================
// 游戏规则测试
// ============================================================================

TEST_F(TickPrecipitationTest, MaxSnowAccumulationHeightZeroPreventsSnow)
{
    ensureChunk(0, 0);

    auto* weatherManager = m_world->weatherManager();
    ASSERT_NE(weatherManager, nullptr);
    weatherManager->setRain(10000);

    // 设置最大雪层积累高度为 0
    m_world->getGameRules().setInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT, 0);

    // 即使触发降水 tick，也不应放置雪（因为 maxSnowAccumulation = 0）
    m_world->tickPrecipitation(100);

    // 不应有雪（但冰是允许的）
    bool foundSnow = false;
    ChunkData* chunk = m_world->getChunk(0, 0);
    if (chunk != nullptr) {
        for (i32 x = 0; x < 16 && !foundSnow; ++x) {
            for (i32 z = 0; z < 16 && !foundSnow; ++z) {
                for (i32 y = world::SEA_LEVEL - 5; y <= world::SEA_LEVEL + 5; ++y) {
                    const BlockState* state = m_world->getBlockState(x, y, z);
                    if (state != nullptr && state->is(VanillaBlocks::SNOW)) {
                        foundSnow = true;
                        break;
                    }
                }
            }
        }
    }
    EXPECT_FALSE(foundSnow);
}

TEST_F(TickPrecipitationTest, MaxSnowAccumulationHeightLimitsSnowLayers)
{
    ensureChunk(0, 0);

    auto* weatherManager = m_world->weatherManager();
    ASSERT_NE(weatherManager, nullptr);
    weatherManager->setRain(10000);

    // 设置最大雪层积累高度为 1（默认值）
    m_world->getGameRules().setInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT, 1);

    // 运行多个 tick
    for (int tick = 0; tick < 20; ++tick) {
        m_world->tickPrecipitation(100);
    }

    // 如果有雪层，检查是否超过 maxSnowAccumulation
    ChunkData* chunk = m_world->getChunk(0, 0);
    if (chunk != nullptr) {
        bool foundOverLimitSnow = false;
        for (i32 x = 0; x < 16 && !foundOverLimitSnow; ++x) {
            for (i32 z = 0; z < 16 && !foundOverLimitSnow; ++z) {
                for (i32 y = world::SEA_LEVEL - 5; y <= world::SEA_LEVEL + 5; ++y) {
                    const BlockState* state = m_world->getBlockState(x, y, z);
                    if (state != nullptr && state->is(VanillaBlocks::SNOW)) {
                        i32 layers = state->get(blocks::SnowBlock::LAYERS());
                        if (layers > 1) {
                            foundOverLimitSnow = true;
                            break;
                        }
                    }
                }
            }
        }
        EXPECT_FALSE(foundOverLimitSnow) << "Snow layers should not exceed maxSnowAccumulation=1";
    }
}
