#include <gtest/gtest.h>

#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"

namespace {

void ensureVanillaBlocksInitialized() {
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class BlockLightChunkProvider : public mc::StarLightLightingProvider {
public:
    BlockLightChunkProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
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
        return false;
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

void processBlockWork(mc::BlockStarLightEngine& engine) {
    for (int i = 0; i < 16 && engine.hasWork(); ++i) {
        engine.tick(65536, false, true);
    }
}

void initSectionData(mc::BlockStarLightEngine& engine, mc::ChunkData& chunk, const mc::SectionPos& sectionPos) {
    mc::ChunkSection* section = chunk.getSection(sectionPos.y);
    ASSERT_NE(section, nullptr);
    engine.updateSectionStatus(sectionPos, false);
    engine.setData(sectionPos, section->blockLightNibble().copy(), false);
}

TEST(BlockLightRegressionTest, EmissiveBlockPropagatesToNeighbors) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlock(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.checkBlock(&provider, 8, 70, 8);
    processBlockWork(engine);

    const mc::u8 source = engine.getLightFor(8, 70, 8);
    const mc::u8 east = engine.getLightFor(9, 70, 8);
    const mc::u8 east2 = engine.getLightFor(10, 70, 8);

    EXPECT_GT(source, static_cast<mc::u8>(0));
    EXPECT_GT(east, static_cast<mc::u8>(0));
    EXPECT_GT(east2, static_cast<mc::u8>(0));
    EXPECT_GE(source, east);
    EXPECT_GE(east, east2);
}

TEST(BlockLightRegressionTest, RemovingSourceDarkensNearbyCells) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* air = &mc::VanillaBlocks::AIR->defaultState();

    chunk.setBlock(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.checkBlock(&provider, 8, 70, 8);
    processBlockWork(engine);

    EXPECT_GT(engine.getLightFor(9, 70, 8), static_cast<mc::u8>(0));

    chunk.setBlock(8, 70, 8, air);
    engine.checkBlock(&provider, 8, 70, 8);
    processBlockWork(engine);

    EXPECT_EQ(engine.getLightFor(8, 70, 8), static_cast<mc::u8>(0));
    EXPECT_EQ(engine.getLightFor(9, 70, 8), static_cast<mc::u8>(0));
}

TEST(BlockLightRegressionTest, InsertingOpaqueBlockReducesBehindLight) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();

    chunk.setBlock(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.checkBlock(&provider, 8, 70, 8);
    processBlockWork(engine);

    const mc::u8 before = engine.getLightFor(10, 70, 8);
    EXPECT_GT(before, static_cast<mc::u8>(0));

    chunk.setBlock(9, 70, 8, stone);
    engine.checkBlock(&provider, 9, 70, 8);
    processBlockWork(engine);

    const mc::u8 after = engine.getLightFor(10, 70, 8);
    EXPECT_LT(after, before);
}

TEST(BlockLightRegressionTest, RemovingOpaqueBlockRestoresBehindLight) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* air = &mc::VanillaBlocks::AIR->defaultState();

    chunk.setBlock(8, 70, 8, glowstone);
    chunk.setBlock(9, 70, 8, stone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.checkBlock(&provider, 8, 70, 8);
    engine.checkBlock(&provider, 9, 70, 8);
    processBlockWork(engine);

    const mc::u8 blocked = engine.getLightFor(10, 70, 8);

    chunk.setBlock(9, 70, 8, air);
    engine.checkBlock(&provider, 9, 70, 8);
    processBlockWork(engine);

    const mc::u8 restored = engine.getLightFor(10, 70, 8);
    EXPECT_GT(restored, blocked);
}

TEST(BlockLightRegressionTest, EmissionIncreaseEventQueuesPropagation) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlock(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.onBlockEmissionIncrease(&provider, 8, 70, 8, 15);
    processBlockWork(engine);

    const mc::u8 east = engine.getLightFor(9, 70, 8);
    EXPECT_GT(east, static_cast<mc::u8>(0));
}

TEST(BlockLightRegressionTest, CheckBlockMatchesCheckBlock) {
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(0, 256);
    mc::ChunkData chunk(0, 0);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine(&provider);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlock(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    initSectionData(engine, chunk, sectionPos);

    engine.checkBlock(&provider, 8, 70, 8);
    processBlockWork(engine);

    EXPECT_GT(engine.getLightFor(9, 70, 8), static_cast<mc::u8>(0));
}

} // namespace
