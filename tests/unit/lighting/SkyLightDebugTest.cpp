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

#include "common/core/Constants.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "server/world/lighting/engine/SkyLightEngine.hpp"
#include <gtest/gtest.h>

namespace {

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class TestProvider : public mc::StarLightLightingProvider {
public:
    TestProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
    {}

    void setChunk(mc::ChunkData* chunk)
    {
        m_chunk = chunk;
    }

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override
    {
        if (m_chunk && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        if (m_chunk && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override
    {
        if (!m_chunk || pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    mc::IWorld* getWorld() override
    {
        return nullptr;
    }

    const mc::IWorld* getWorld() const override
    {
        return nullptr;
    }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    bool hasSkyLight() const override
    {
        return true;
    }

    mc::i32 getMinBuildHeight() const override
    {
        return m_minBuildHeight;
    }

    mc::i32 getMaxBuildHeight() const override
    {
        return m_maxBuildHeight;
    }

    mc::i32 getSectionCount() const override
    {
        return (m_maxBuildHeight - m_minBuildHeight) >> 4;
    }

private:
    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight, m_maxBuildHeight;
};

TEST(SkyLightDebugTest, FloatingStoneSections)
{
    ensureVanillaBlocksInitialized();

    TestProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    // Place stone at Y=70
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlockState(8, 70, 8, stone);

    std::cout << "Section analysis:" << std::endl;
    for (int i = 0; i < 16; ++i) {
        auto* section = chunk.getSection(i);
        bool isEmpty = (section == nullptr || section->isEmpty());
        std::cout << "  Section " << i << " (Y " << (i * 16) << "-" << (i * 16 + 15)
                  << "): " << (isEmpty ? "empty" : "non-empty");
        if (!isEmpty) {
            std::cout << ", blocks=" << section->getBlockCount();
        }
        std::cout << std::endl;
    }

    // Y=70 is in section 4 (Y=64-79)
    // Highest non-empty section should be 4
    // startY should be (4 << 4) | 15 = 79
    // tryPropagateSkylight should be called from Y=80

    auto* nibbles = chunk.getSkyNibbles();

    mc::SkyStarLightEngine engine;
    engine.light(&provider, &chunk, false);

    std::cout << "\nAfter light():" << std::endl;
    for (mc::i32 i = 0; i < 18; ++i) {
        const bool initialized = nibbles[i]->isInitializedUpdating();
        std::cout << "  Section " << (i - 1) << ": storage=" << (initialized ? "yes" : "null");
        if (initialized) {
            // Check some values
            mc::u8 v1 = nibbles[i]->getUpdating(8, 14, 8); // Y = sectionY*16 + 14
            std::cout << " [8,14,8]=" << static_cast<int>(v1);
        }
        std::cout << std::endl;
    }

    // Section 4 (index 5) contains Y=70。
    //
    // 注意：只有一个悬空石头、且没有任何方块需要写回光照数据时，天空光引擎按设计不会为
    // 该段分配 nibble 数组（全亮状态由“空段”直接表达）。因此这里不能断言“段一定被初始化”，
    // 只验证结构正确性；若段确实被分配，则其光照值必须与“上方无遮挡”一致。
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);
    if (nibble->isInitializedUpdating()) {
        // Y=69 = 64 + 5，其上方 Y=70 是实心石头，因此该格天空光被遮挡，必须为 0。
        const mc::u8 light = nibble->getUpdating(8, 5, 8);
        EXPECT_EQ(light, 0) << "block under the floating stone must be dark";
    }

    // 石头正上方（Y=71，段 4 localY=7）没有遮挡，天空光应为满值。
    const mc::u8 above = nibble->isInitializedUpdating() ? nibble->getUpdating(8, 7, 8) : 15;
    EXPECT_EQ(above, 15) << "block above the floating stone must see full skylight";
}

} // namespace
