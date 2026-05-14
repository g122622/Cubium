#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"

namespace {

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class SkyLightChunkProvider : public mc::StarLightLightingProvider {
public:
    SkyLightChunkProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
    {}

    void setChunk(mc::ChunkData* chunk) { m_chunk = chunk; }

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    mc::IWorld* getWorld() override { return nullptr; }

    const mc::IWorld* getWorld() const override { return nullptr; }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    bool hasSkyLight() const override { return true; }

    mc::i32 getMinBuildHeight() const override { return m_minBuildHeight; }

    mc::i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    mc::i32 getSectionCount() const override { return (m_maxBuildHeight - m_minBuildHeight) >> 4; }

private:
    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
};

TEST(SkyLightRegressionTest, FloatingStoneUndersideHasNonZeroSkyLight)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态，使光照引擎可以使用它
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine(&provider);

    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlockState(8, 70, 8, stoneState);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    // 使用 light() 方法初始化区块光照（这会设置缓存）
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // Y=69 在 section 4 (Y=64-79)
    // LIGHT_SECTIONS = 18, 索引 0 对应 section -1, 索引 i 对应 section (i-1)
    // Section 4 在 LIGHT_SECTIONS 数组中的索引是 4 - (-1) = 5
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5]; // section 4
    ASSERT_NE(nibble, nullptr);

    // 检查状态
    EXPECT_TRUE(nibble->isInitializedUpdating()) << "Nibble array should be initialized";

    // 读取 (8, 69, 8) 位置的光照值
    // localY = 69 - 64 = 5 (section 4 从 Y=64 开始)
    mc::u8 belowLight = nibble->getUpdating(8, 69 - 64, 8);
    EXPECT_GT(belowLight, static_cast<mc::u8>(0)) << "Light at (8, 69, 8) should be > 0";
}

TEST(SkyLightRegressionTest, LightRebuildsSkyEmptinessMapWhenMissing)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    chunk.setSkyEmptinessMap(nullptr);
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine(&provider);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    engine.light(&provider, &chunk, false);

    const bool* emptinessMap = chunk.getSkyEmptinessMap();
    ASSERT_NE(emptinessMap, nullptr);

    for (mc::i32 sectionIndex = 0; sectionIndex < mc::world::CHUNK_SECTIONS; ++sectionIndex) {
        EXPECT_TRUE(emptinessMap[sectionIndex]);
    }

    for (mc::i32 sectionIndex = mc::world::CHUNK_SECTIONS; sectionIndex < mc::ChunkData::LIGHT_SECTIONS;
        ++sectionIndex) {
        EXPECT_FALSE(emptinessMap[sectionIndex]);
    }
}

TEST(SkyLightRegressionTest, SealedRoofDropsCaveSkyLightBelow15)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine(&provider);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();

    // 在 section=4 顶层铺满石头，封闭下方空间。
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stoneState);
        }
    }

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // Y=78 在 section 4 (Y=64-79)，在 LIGHT_SECTIONS 数组中索引为 5 (section 4 + 1)
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    // Section 4 在 LIGHT_SECTIONS 数组中的索引是 4 - minLightSection(-1) + 1 = 5
    mc::SWMRNibbleArray* nibble = nibbles[5]; // section 4
    ASSERT_NE(nibble, nullptr);

    // 读取 (8, 78, 8) 位置的光照值
    // localY = 78 % 16 = 14
    mc::u8 caveSkyLight = nibble->getUpdating(8, 14, 8);
    EXPECT_LT(caveSkyLight, static_cast<mc::u8>(15));
}

TEST(SkyLightRegressionTest, OpeningRoofRestoresCaveSkyLight)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine(&provider);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* airState = &mc::VanillaBlocks::AIR->defaultState();

    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stoneState);
        }
    }

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5]; // section 4
    ASSERT_NE(nibble, nullptr);

    mc::u8 before = nibble->getUpdating(8, 14, 8); // Y=78 -> localY=14
    EXPECT_LT(before, static_cast<mc::u8>(15));

    // 打开一个洞
    chunk.setBlockState(8, 79, 8, airState);

    // 重新运行光照检查
    engine.light(&provider, &chunk, false);

    mc::u8 after = nibble->getUpdating(8, 14, 8);
    EXPECT_GT(after, before);
}

TEST(SkyLightRegressionTest, CheckBlockMatchesCheckBlock)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine(&provider);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlockState(8, 70, 8, stoneState);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5]; // section 4
    ASSERT_NE(nibble, nullptr);

    // Y=69 -> localY=5
    mc::u8 light = nibble->getUpdating(8, 5, 8);
    EXPECT_GT(light, static_cast<mc::u8>(0));
}

} // namespace
