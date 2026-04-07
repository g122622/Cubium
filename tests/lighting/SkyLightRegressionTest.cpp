#include <gtest/gtest.h>

#include "common/util/NibbleArray.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/storage/SkyLightStorage.hpp"

namespace {

void ensureVanillaBlocksInitialized() {
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class SkyLightChunkProvider : public mc::IChunkLightProvider {
public:
    SkyLightChunkProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight) {
    }

    void setChunk(mc::ChunkData* chunk) {
        m_chunk = chunk;
    }

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlock(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    mc::IWorld* getWorld() override {
        return nullptr;
    }

    const mc::IWorld* getWorld() const override {
        return nullptr;
    }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {
    }

    bool hasSkyLight() const override {
        return true;
    }

    mc::i32 getMinBuildHeight() const override {
        return m_minBuildHeight;
    }

    mc::i32 getMaxBuildHeight() const override {
        return m_maxBuildHeight;
    }

    mc::i32 getSectionCount() const override {
        return (m_maxBuildHeight - m_minBuildHeight) >> 4;
    }

private:
    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
};

void processSkyWork(mc::SkyLightEngine& engine) {
    for (int i = 0; i < 12 && engine.hasWork(); ++i) {
        engine.tick(65536, true, false);
    }
}

TEST(SkyLightRegressionTest, FloatingStoneUndersideHasNonZeroSkyLight) {
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::SkyLightEngine engine(&provider);

    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlock(8, 70, 8, stoneState);

    mc::ChunkSection* section = chunk.getSection(4);
    ASSERT_NE(section, nullptr);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setData(sectionPos, section->skyLightNibble().copy(), false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    engine.checkLight(mc::BlockPos(8, 70, 8));
    processSkyWork(engine);

    const mc::u8 belowLight = engine.getLightFor(mc::BlockPos(8, 69, 8));
    EXPECT_GT(belowLight, static_cast<mc::u8>(0))
        << "hasWork=" << engine.hasWork();
}

TEST(SkyLightRegressionTest, SealedRoofDropsCaveSkyLightBelow15) {
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::SkyLightEngine engine(&provider);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();

    // 在 section=4 顶层铺满石头，封闭下方空间。
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlock(x, 79, z, stoneState);
        }
    }

    mc::ChunkSection* section = chunk.getSection(4);
    ASSERT_NE(section, nullptr);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setData(sectionPos, section->skyLightNibble().copy(), false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            engine.checkLight(mc::BlockPos(x, 79, z));
        }
    }
    processSkyWork(engine);

    const mc::u8 caveSkyLight = engine.getLightFor(mc::BlockPos(8, 78, 8));
    EXPECT_LT(caveSkyLight, static_cast<mc::u8>(15));
}

TEST(SkyLightRegressionTest, OpeningRoofRestoresCaveSkyLight) {
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::SkyLightEngine engine(&provider);
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* airState = &mc::VanillaBlocks::AIR->defaultState();

    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlock(x, 79, z, stoneState);
        }
    }

    mc::ChunkSection* section = chunk.getSection(4);
    ASSERT_NE(section, nullptr);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);
    engine.setData(sectionPos, section->skyLightNibble().copy(), false);
    engine.setColumnEnabled(sectionPos.toColumnLong(), true);

    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            engine.checkLight(mc::BlockPos(x, 79, z));
        }
    }
    processSkyWork(engine);

    const mc::u8 before = engine.getLightFor(mc::BlockPos(8, 78, 8));
    EXPECT_LT(before, static_cast<mc::u8>(15));

    chunk.setBlock(8, 79, 8, airState);
    engine.checkLight(mc::BlockPos(8, 79, 8));
    processSkyWork(engine);

    const mc::u8 after = engine.getLightFor(mc::BlockPos(8, 78, 8));
    EXPECT_GT(after, before);
}

TEST(SkyLightRegressionTest, SurfaceTopDetectionHandlesNegativeY) {
    SkyLightChunkProvider provider(-64, 320);
    mc::SkyLightStorage storage(&provider);

    const mc::SectionPos sectionPos(0, -2, 0);
    storage.updateSectionStatus(sectionPos.toLong(), false);
    storage.processAllLevelUpdates();

    storage.setColumnEnabled(sectionPos.toColumnLong(), true);

    const mc::i64 topBlockPos = mc::LightEngineUtils::packPos(0, -17, 0);
    const mc::i64 notTopBlockPos = mc::LightEngineUtils::packPos(0, -18, 0);

    EXPECT_TRUE(storage.isAtSurfaceTop(topBlockPos));
    EXPECT_FALSE(storage.isAtSurfaceTop(notTopBlockPos));
}

} // namespace
