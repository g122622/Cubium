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

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace {

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

/**
 * @brief 主线程读路径测试用的光照提供者
 *
 * 持有单个区块，主线程读经 getChunkForLight 取区块后直接读 nibble visible 侧。
 * 维度配置同主世界（有天空光、-64~320）。
 */
class MainThreadReadProvider final : public mc::StarLightLightingProvider {
public:
    explicit MainThreadReadProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
    {}

    void setChunk(mc::ChunkData* chunk) { m_chunk = chunk; }

    [[nodiscard]] mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override { return resolve(x, z); }

    [[nodiscard]] const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        return resolve(x, z);
    }

    [[nodiscard]] const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override
    {
        if (m_chunk == nullptr || pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    [[nodiscard]] mc::IWorld* getWorld() override { return nullptr; }
    [[nodiscard]] const mc::IWorld* getWorld() const override { return nullptr; }
    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}
    [[nodiscard]] bool hasSkyLight() const override { return true; }
    [[nodiscard]] mc::i32 getMinBuildHeight() const override { return m_minBuildHeight; }
    [[nodiscard]] mc::i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }
    [[nodiscard]] mc::i32 getSectionCount() const override { return (m_maxBuildHeight - m_minBuildHeight) >> 4; }

private:
    [[nodiscard]] mc::ChunkData* resolve(mc::ChunkCoord x, mc::ChunkCoord z) const
    {
        if (m_chunk == nullptr || m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
};

} // namespace

// ============================================================================
// ③-2a 回归测试：主线程读路径必须读 visible 侧真实光照值
//
// 历史 bug：getLightSubtracted/getBlockLight/getSkyLight 经引擎 getLightFor→
// getLightLevel→getNibbleFromCache+getUpdating。主线程不调 setupCaches，
// m_nibbleCache 为空 → getNibbleFromCache 返回 nullptr → getLightLevel 返回
// 默认值（sky=15/block=0），主线程读不到真实光照。该 bug 被 m_mutex 掩盖。
//
// 修复后主线程读直接从 ChunkData nibble 数组的 visible 侧（atomic acquire）读取，
// 不经引擎缓存、不持锁。本测试验证修复后主线程读到的是真实光照值而非默认值。
//
// 写路径（updateSectionStatus/lightChunk/checkBlock）经 TLS 引擎池直接调引擎，
// engine->lightChunk 末尾内部 updateVisible 发布 visible 侧，主线程读即读到。
// ============================================================================

// 测试1：方块光主线程读返回真实光源传播值（非默认 0）
TEST(MainThreadVisibleReadTest, BlockLightReadReturnsPropagatedValue)
{
    ensureVanillaBlocksInitialized();

    MainThreadReadProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager manager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    auto* engine = mc::WorldLightManager::acquireBlockLightEngine();
    engine->updateSectionStatus(sectionPos, false);
    engine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseBlockLightEngine(engine);

    // 主线程读：(8,70,8) 是光源本身，光等级 = 萤石发光等级（15）。旧 bug 会返回 0。
    const mc::u8 source = manager.getBlockLight(8, 70, 8);
    EXPECT_EQ(source, static_cast<mc::u8>(15)) << "光源位置主线程读应为 15，旧 bug 返回 0";

    // 邻位受光源传播，应有非零光照（< 15，衰减 1 级）。旧 bug 返回 0。
    const mc::u8 neighbor = manager.getBlockLight(9, 70, 8);
    EXPECT_GT(neighbor, static_cast<mc::u8>(0)) << "邻位应受光源传播有非零光照";
    EXPECT_LT(neighbor, source) << "邻位光照应低于光源";
}

// 测试2：天空光主线程读返回真实遮挡值（非默认满亮 15）
//
// 在 section 4 顶层铺满石头封闭下方，下方天空光应 < 15。旧 bug 无条件返回 15。
TEST(MainThreadVisibleReadTest, SkyLightReadReturnsOccludedValueBelowRoof)
{
    ensureVanillaBlocksInitialized();

    MainThreadReadProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager manager(&provider, false, true);
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();

    // 在 section 4 顶层（Y=79）铺满石头，封闭 Y=78 及以下
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stone);
        }
    }

    const mc::SectionPos sectionPos(0, 4, 0);
    auto* engine = mc::WorldLightManager::acquireSkyLightEngine();
    engine->updateSectionStatus(sectionPos, false);
    engine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseSkyLightEngine(engine);

    // 主线程读 Y=78（屋顶下）天空光，应被遮挡 < 15。旧 bug 返回 15。
    const mc::u8 caveSky = manager.getSkyLight(8, 78, 8);
    EXPECT_LT(caveSky, static_cast<mc::u8>(15)) << "屋顶下天空光应 < 15，旧 bug 返回 15";
}

// 测试3：天空光在无遮挡开阔地表应满亮 15（验证不因 bug 误读 0 或漏读）
TEST(MainThreadVisibleReadTest, SkyLightReadReturnsFifteenAtOpenSurface)
{
    ensureVanillaBlocksInitialized();

    MainThreadReadProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager manager(&provider, false, true);
    const mc::SectionPos sectionPos(0, 4, 0);
    auto* engine = mc::WorldLightManager::acquireSkyLightEngine();
    engine->updateSectionStatus(sectionPos, false);
    engine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseSkyLightEngine(engine);

    // 主线程读地表开阔处天空光应满亮 15。
    const mc::u8 openSky = manager.getSkyLight(8, 78, 8);
    EXPECT_EQ(openSky, static_cast<mc::u8>(15));
}

// 测试4：getLightSubtracted 综合 sky+block 取最大，主线程读返回真实值
//
// 屋顶下方放萤石：天空光被遮挡 < 15，方块光来自萤石。getLightSubtracted 应返回
// 两者最大值（萤石方块光）。旧 bug sky=15 短路返回 15。
TEST(MainThreadVisibleReadTest, LightSubtractedReturnsMaxOfSkyAndBlock)
{
    ensureVanillaBlocksInitialized();

    MainThreadReadProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager manager(&provider, true, true);
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();

    // 屋顶
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stone);
        }
    }
    // 屋顶下放萤石光源
    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    // 天空光 + 方块光都需 light() 发布 visible 侧（与原 WorldLightManager::lightChunk 顺序一致）
    auto* skyEngine = mc::WorldLightManager::acquireSkyLightEngine();
    skyEngine->updateSectionStatus(sectionPos, false);
    skyEngine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseSkyLightEngine(skyEngine);

    auto* blockEngine = mc::WorldLightManager::acquireBlockLightEngine();
    blockEngine->updateSectionStatus(sectionPos, false);
    blockEngine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseBlockLightEngine(blockEngine);

    // (8,70,8) 萤石：方块光=15，天空光被屋顶遮挡<15。getLightSubtracted(skyDarkening=0)
    // 应 = max(block, sky) = 15。旧 bug sky=15 短路也返回 15（巧合相等，但路径错误）。
    const mc::i32 atSource = manager.getLightSubtracted(mc::BlockPos(8, 70, 8), 0);
    EXPECT_EQ(atSource, 15);

    // 萤石邻位 (9,70,8)：方块光 = 15-1 = 14（衰减），天空光被遮挡 < 15。
    // getLightSubtracted 应 = max(14, sky<15)。若 sky <= 14 则结果 = 14。
    // 关键：旧 bug sky 短路返回 15（错误地比真实值高），修复后返回真实 max。
    const mc::i32 neighbor = manager.getLightSubtracted(mc::BlockPos(9, 70, 8), 0);
    const mc::u8 neighborSky = manager.getSkyLight(9, 70, 8);
    const mc::u8 neighborBlock = manager.getBlockLight(9, 70, 8);
    EXPECT_EQ(neighbor, std::max(static_cast<mc::i32>(neighborSky), static_cast<mc::i32>(neighborBlock)));
    // 邻位方块光应非零（受萤石传播）
    EXPECT_GT(neighborBlock, static_cast<mc::u8>(0));
}

// 测试5：增量 checkBlock 后主线程读反映更新（验证读路径持续读 visible 侧最新发布值）
TEST(MainThreadVisibleReadTest, ReadReflectsIncrementalCheckBlock)
{
    ensureVanillaBlocksInitialized();

    MainThreadReadProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager manager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* air = &mc::VanillaBlocks::AIR->defaultState();

    chunk.setBlockState(8, 70, 8, glowstone);
    const mc::SectionPos sectionPos(0, 4, 0);
    auto* engine = mc::WorldLightManager::acquireBlockLightEngine();
    engine->updateSectionStatus(sectionPos, false);
    engine->light(&provider, &chunk, false);
    mc::WorldLightManager::releaseBlockLightEngine(engine);

    // 初始：光源位方块光 = 15
    EXPECT_EQ(manager.getBlockLight(8, 70, 8), static_cast<mc::u8>(15));

    // 移除光源，增量更新（blocksChangedInChunk 是自带缓存管理的高层入口，
    // 内部 propagateBlockChanges+updateVisible 发布 visible 侧，主线程读即反映）
    chunk.setBlockState(8, 70, 8, air);
    auto* deltaEngine = mc::WorldLightManager::acquireBlockLightEngine();
    deltaEngine->blocksChangedInChunk(
        &provider, 8 >> mc::world::CHUNK_SHIFT, 8 >> mc::world::CHUNK_SHIFT, {mc::BlockPos(8, 70, 8)}, {});
    mc::WorldLightManager::releaseBlockLightEngine(deltaEngine);

    // 主线程读应反映移除后的 0（旧 bug 读路径不经引擎缓存，移除前后都返回 0，
    // 无法区分；修复后初始读 15、移除后读 0，证明读的是真实 visible 侧数据）
    EXPECT_EQ(manager.getBlockLight(8, 70, 8), static_cast<mc::u8>(0));
}
