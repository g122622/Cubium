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
#include "common/util/NibbleArray.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"
#include "common/world/lighting/LightEngineUtils.hpp"
#include "common/world/lighting/LightType.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/lighting/manager/WorldLightManager.hpp"
#include <vector>
#include <gtest/gtest.h>

namespace mc::server {
namespace {

/**
 * @brief 测试光照同步到 ChunkSection
 *
 * 验证当 markLightChanged 被调用时，光照数据从光照引擎同步到 ChunkSection。
 */
class LightSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }

    void SetUp() override
    {
        // 创建一个最小化的测试环境
    }

    void TearDown() override {}
};

/**
 * @brief 测试 NibbleArray 复制功能
 *
 * 验证 NibbleArray 可以正确复制，这是光照同步的基础。
 */
TEST_F(LightSyncTest, NibbleArrayCopy)
{
    // 创建一个有数据的 NibbleArray
    NibbleArray array;
    array.set(0, 0, 0, 15);
    array.set(1, 2, 3, 7);
    array.set(15, 15, 15, 3);

    // 复制
    NibbleArray copy = array.copy();

    // 验证复制后的数据一致
    EXPECT_EQ(copy.get(0, 0, 0), 15);
    EXPECT_EQ(copy.get(1, 2, 3), 7);
    EXPECT_EQ(copy.get(15, 15, 15), 3);

    // 修改原数组不应影响复制
    array.set(0, 0, 0, 0);
    EXPECT_EQ(copy.get(0, 0, 0), 15);
}

/**
 * @brief 测试 ChunkData 光照 nibble 直接访问
 *
 * 验证经 skyNibbleAt/blockNibbleAt 拿到 nibble 后可直接读写（光照引擎走的路径），
 * 以及 updateVisible 发布后可见侧可读。
 */
TEST_F(LightSyncTest, ChunkDataDirectNibbleArrayModification)
{
    ChunkData chunk(0, 0);

    // 段索引与世界的换算：(worldY - MIN_BUILD_HEIGHT) / 16，主世界 MIN_BUILD_HEIGHT=-64。
    // 段索引 2 对应 Y=32..47。
    constexpr i32 kSkySectionIndex = (32 - mc::world::MIN_BUILD_HEIGHT) / 16;

    // 天空光：首次写入前必须 materialize 成"全亮 15"，否则本段其余坐标会从 15 骤变为 0
    SWMRNibbleArray& sky = chunk.skyNibbleAt(kSkySectionIndex);
    sky.setFull();
    sky.set(0, 0, 0, 7);
    sky.updateVisible();

    EXPECT_EQ(chunk.getSkyLight(0, 32, 0), 7);
    EXPECT_EQ(chunk.getSkyLight(5, 42, 3), 15); // 未写入的坐标仍是全亮

    // 方块光：默认 0，无需 materialize。段索引 3 对应 Y=48..63。
    constexpr i32 kBlockSectionIndex = (48 - mc::world::MIN_BUILD_HEIGHT) / 16;
    SWMRNibbleArray& block = chunk.blockNibbleAt(kBlockSectionIndex);
    block.set(1, 1, 1, 5);
    block.updateVisible();

    EXPECT_EQ(chunk.getBlockLight(1, 49, 1), 5);
    EXPECT_EQ(chunk.getBlockLight(0, 48, 0), 0);
}

/**
 * @brief 测试 ChunkData 光照访问
 *
 * 光照的权威副本在 ChunkData（SWMRNibbleArray），ChunkSection 不再持有光照
 * （对齐原版 1.21.11：LevelChunkSection 只含 states + biomes）。
 */
TEST_F(LightSyncTest, ChunkDataLightAccess)
{
    ChunkData chunk(0, 0);

    // 未光照段（Null/Uninit）按默认值：天空光全亮 15、方块光无光 0
    EXPECT_EQ(chunk.getSkyLight(5, 32, 7), 15);
    EXPECT_EQ(chunk.getBlockLight(3, 48, 2), 0);

    // 逐点写入（内部会 materialize + 发布可见侧）
    chunk.setSkyLight(5, 32, 7, 14);
    EXPECT_EQ(chunk.getSkyLight(5, 32, 7), 14);
    // 同段其余坐标必须保持默认全亮，不得被零初始化拉成 0
    EXPECT_EQ(chunk.getSkyLight(6, 32, 7), 15);

    chunk.setBlockLight(3, 48, 2, 10);
    EXPECT_EQ(chunk.getBlockLight(3, 48, 2), 10);
    EXPECT_EQ(chunk.getBlockLight(4, 48, 2), 0);

    // 边界检查
    EXPECT_EQ(chunk.getSkyLight(-1, 0, 0), 15);  // 边界外默认全亮
    EXPECT_EQ(chunk.getBlockLight(-1, 0, 0), 0); // 边界外默认无光
}

/**
 * @brief 测试 ChunkSection 序列化只含方块数据
 *
 * 光照归 ChunkData（SWMRNibbleArray），故段序列化往返后方块数据保真、且字节数与
 * calculateSectionSize 的预测严格一致（光照不再计入）。
 */
TEST_F(LightSyncTest, ChunkSectionSerializeCarriesOnlyBlockData)
{
    ChunkSection original;
    original.setBlockState(5, 10, 7, &VanillaBlocks::STONE->defaultState());
    original.setBlockState(3, 8, 2, &VanillaBlocks::DIRT->defaultState());

    std::vector<u8> data = original.serialize();

    // 2(计数) + 4096*4(方块状态ID)
    EXPECT_EQ(data.size(), 2 + ChunkSection::VOLUME * sizeof(u32));

    auto result = ChunkSection::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());

    auto restored = result.value();
    ASSERT_TRUE(restored);
    ASSERT_NE(restored->getBlockState(5, 10, 7), nullptr);
    EXPECT_EQ(restored->getBlockState(5, 10, 7)->blockId(), VanillaBlocks::STONE->blockId());
    ASSERT_NE(restored->getBlockState(3, 8, 2), nullptr);
    EXPECT_EQ(restored->getBlockState(3, 8, 2)->blockId(), VanillaBlocks::DIRT->blockId());
}

/**
 * @brief 测试 SectionPos 编码解码
 *
 * 验证 SectionPos 可以正确编码和解码。
 */
TEST_F(LightSyncTest, SectionPosEncodeDecode)
{
    SectionPos pos(10, 5, -20);
    i64 encoded = pos.toLong();
    SectionPos decoded = SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, 10);
    EXPECT_EQ(decoded.y, 5);
    EXPECT_EQ(decoded.z, -20);
}

/**
 * @brief 测试 SectionPos 列位置编码
 *
 * 验证 SectionPos 可以正确计算列位置。
 */
TEST_F(LightSyncTest, SectionPosColumnPos)
{
    SectionPos pos(10, 5, -20);
    i64 columnPos = pos.toColumnLong();

    // 列位置应该忽略 Y 坐标
    SectionPos pos2(10, 100, -20);
    i64 columnPos2 = pos2.toColumnLong();

    EXPECT_EQ(columnPos, columnPos2);
}

/**
 * @brief 测试 WorldLightManager 基本创建
 *
 * 验证 WorldLightManager 正确报告维度光照配置（hasBlockLight/hasSkyLight）。
 * 引擎已改 TLS 池（③-2b），不再由管理器持有单例引擎。
 */
TEST_F(LightSyncTest, WorldLightManagerCreation)
{
    // 创建一个简单的 StarLightLightingProvider 实现
    class TestLightProvider : public StarLightLightingProvider {
    public:
        IChunk* getChunkForLight(ChunkCoord, ChunkCoord) override { return nullptr; }
        const IChunk* getChunkForLight(ChunkCoord, ChunkCoord) const override { return nullptr; }
        const BlockState* getBlockStateForLight(const BlockPos&) const override { return nullptr; }
        IWorld* getWorld() override { return nullptr; }
        const IWorld* getWorld() const override { return nullptr; }
        void markLightChanged(LightType, const SectionPos&) override {}
        bool hasSkyLight() const override { return true; }
        i32 getMinBuildHeight() const override { return 0; }
        i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }
        i32 getSectionCount() const override { return 16; }
    };

    TestLightProvider provider;
    WorldLightManager lightManager(&provider, true, true);

    // 验证维度配置
    EXPECT_TRUE(lightManager.hasBlockLight());
    EXPECT_TRUE(lightManager.hasSkyLight());

    // 测试无天空光照的情况（如下界）
    WorldLightManager blockOnlyManager(&provider, true, false);
    EXPECT_TRUE(blockOnlyManager.hasBlockLight());
    EXPECT_FALSE(blockOnlyManager.hasSkyLight());
}

/**
 * @brief 测试 TLS 引擎池
 *
 * 验证 WorldLightManager 的 thread_local 引擎池（③-2b）：acquire 惰性构造、
 * 同线程复用同一实例、release 为 no-op。对齐 Moonrise StarLightInterface 的 TLS 模型。
 */
TEST_F(LightSyncTest, WorldLightManagerTLSEnginePool)
{
    class TestLightProvider : public StarLightLightingProvider {
    public:
        IChunk* getChunkForLight(ChunkCoord, ChunkCoord) override { return nullptr; }
        const IChunk* getChunkForLight(ChunkCoord, ChunkCoord) const override { return nullptr; }
        const BlockState* getBlockStateForLight(const BlockPos&) const override { return nullptr; }
        IWorld* getWorld() override { return nullptr; }
        const IWorld* getWorld() const override { return nullptr; }
        void markLightChanged(LightType, const SectionPos&) override {}
        bool hasSkyLight() const override { return true; }
        i32 getMinBuildHeight() const override { return 0; }
        i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }
        i32 getSectionCount() const override { return 16; }
    };

    TestLightProvider provider;
    WorldLightManager lightManager(&provider, true, true);

    // acquire 惰性构造，返回非空
    auto* skyEngine1 = WorldLightManager::acquireSkyLightEngine();
    auto* blockEngine1 = WorldLightManager::acquireBlockLightEngine();
    ASSERT_NE(skyEngine1, nullptr);
    ASSERT_NE(blockEngine1, nullptr);

    // 同线程复用同一实例（TLS）
    auto* skyEngine2 = WorldLightManager::acquireSkyLightEngine();
    auto* blockEngine2 = WorldLightManager::acquireBlockLightEngine();
    EXPECT_EQ(skyEngine1, skyEngine2);
    EXPECT_EQ(blockEngine1, blockEngine2);

    // release 是 no-op，acquire 后实例不变
    WorldLightManager::releaseSkyLightEngine(skyEngine1);
    WorldLightManager::releaseBlockLightEngine(blockEngine1);
    EXPECT_EQ(WorldLightManager::acquireSkyLightEngine(), skyEngine1);
    EXPECT_EQ(WorldLightManager::acquireBlockLightEngine(), blockEngine1);

    // getData 经 provider 取区块——provider 返回 nullptr，故读路径返回 nullptr（不崩）
    SectionPos pos(0, 0, 0);
    EXPECT_EQ(lightManager.getData(LightType::BLOCK, pos), nullptr);
    EXPECT_EQ(lightManager.getData(LightType::SKY, pos), nullptr);
}

} // namespace
} // namespace mc::server
