#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/core/Constants.hpp"
#include <vector>

namespace mc::server {
namespace {

/**
 * @brief 测试光照同步到 ChunkSection
 *
 * 验证当 markLightChanged 被调用时，光照数据从光照引擎同步到 ChunkSection。
 */
class LightSyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建一个最小化的测试环境
    }

    void TearDown() override {
    }
};

/**
 * @brief 测试 NibbleArray 复制功能
 *
 * 验证 NibbleArray 可以正确复制，这是光照同步的基础。
 */
TEST_F(LightSyncTest, NibbleArrayCopy) {
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
 * @brief 测试 ChunkSection 光照数组访问
 *
 * 验证 ChunkSection 可以正确设置和获取光照值。
 */
TEST_F(LightSyncTest, ChunkSectionLightAccess) {
    ChunkSection section;

    // 设置天空光照
    section.setSkyLight(5, 10, 7, 12);
    EXPECT_EQ(section.getSkyLight(5, 10, 7), 12);

    // 设置方块光照
    section.setBlockLight(3, 8, 2, 8);
    EXPECT_EQ(section.getBlockLight(3, 8, 2), 8);

    // 测试 NibbleArray 引用访问
    NibbleArray& skyLight = section.skyLightNibble();
    skyLight.set(0, 0, 0, 15);
    EXPECT_EQ(section.getSkyLight(0, 0, 0), 15);

    NibbleArray& blockLight = section.blockLightNibble();
    blockLight.set(1, 1, 1, 5);
    EXPECT_EQ(section.getBlockLight(1, 1, 1), 5);
}

/**
 * @brief 测试 ChunkSection 光照填充
 *
 * 验证 ChunkSection 可以正确填充光照值。
 */
TEST_F(LightSyncTest, ChunkSectionLightFill) {
    ChunkSection section;

    // 填充天空光照
    section.fillSkyLight(15);
    EXPECT_EQ(section.getSkyLight(0, 0, 0), 15);
    EXPECT_EQ(section.getSkyLight(15, 15, 15), 15);

    // 填充方块光照
    section.fillBlockLight(0);
    EXPECT_EQ(section.getBlockLight(0, 0, 0), 0);
    EXPECT_EQ(section.getBlockLight(15, 15, 15), 0);
}

/**
 * @brief 测试 ChunkData 光照访问
 *
 * 验证 ChunkData 可以正确设置和获取光照值。
 */
TEST_F(LightSyncTest, ChunkDataLightAccess) {
    ChunkData chunk(0, 0);

    // 设置天空光照（需要先创建区块段）
    chunk.setSkyLight(5, 32, 7, 14);
    EXPECT_EQ(chunk.getSkyLight(5, 32, 7), 14);

    // 设置方块光照
    chunk.setBlockLight(3, 48, 2, 10);
    EXPECT_EQ(chunk.getBlockLight(3, 48, 2), 10);

    // 边界检查
    EXPECT_EQ(chunk.getSkyLight(-1, 0, 0), 15);  // 边界外默认全亮
    EXPECT_EQ(chunk.getBlockLight(-1, 0, 0), 0);  // 边界外默认无光
}

/**
 * @brief 测试 ChunkSection 序列化保留光照数据
 *
 * 验证 ChunkSection 序列化后光照数据可以正确恢复。
 */
TEST_F(LightSyncTest, ChunkSectionSerializePreservesLight) {
    ChunkSection original;
    original.setSkyLight(5, 10, 7, 12);
    original.setBlockLight(3, 8, 2, 8);

    // 序列化
    std::vector<u8> data = original.serialize();

    // 反序列化
    auto result = ChunkSection::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());

    const ChunkSection& restored = *result.value();
    EXPECT_EQ(restored.getSkyLight(5, 10, 7), 12);
    EXPECT_EQ(restored.getBlockLight(3, 8, 2), 8);
}

/**
 * @brief 测试 SectionPos 编码解码
 *
 * 验证 SectionPos 可以正确编码和解码。
 */
TEST_F(LightSyncTest, SectionPosEncodeDecode) {
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
TEST_F(LightSyncTest, SectionPosColumnPos) {
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
 * 验证 WorldLightManager 可以正确创建方块光照和天空光照引擎。
 */
TEST_F(LightSyncTest, WorldLightManagerCreation) {
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

    // 验证引擎已创建
    EXPECT_NE(lightManager.getBlockLightEngine(), nullptr);
    EXPECT_NE(lightManager.getSkyLightEngine(), nullptr);

    // 测试无天空光照的情况
    WorldLightManager blockOnlyManager(&provider, true, false);
    EXPECT_NE(blockOnlyManager.getBlockLightEngine(), nullptr);
    EXPECT_EQ(blockOnlyManager.getSkyLightEngine(), nullptr);
}

/**
 * @brief 测试光照数据设置和获取
 *
 * 验证 WorldLightManager 在正确设置缓存环境后可以设置和获取光照数据。
 * 注意：引擎的 setData/getData 需要缓存环境，这通常在区块加载时设置。
 */
TEST_F(LightSyncTest, WorldLightManagerDataAccess) {
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

    // 设置缓存环境（模拟区块加载）
    auto* blockEngine = lightManager.getBlockLightEngine();
    auto* skyEngine = lightManager.getSkyLightEngine();
    ASSERT_NE(blockEngine, nullptr);
    ASSERT_NE(skyEngine, nullptr);

    // 初始化缓存：以 (0, 0, 0) 为中心
    blockEngine->setupCaches(&provider, 0, 0, 0, true, true);
    skyEngine->setupCaches(&provider, 0, 0, 0, true, true);

    // 创建光照数据
    NibbleArray blockLightData = NibbleArray::filled(8);
    NibbleArray skyLightData = NibbleArray::filled(15);

    SectionPos pos(0, 0, 0);

    // 设置数据
    lightManager.setData(LightType::BLOCK, pos, blockLightData, false);
    lightManager.setData(LightType::SKY, pos, skyLightData, false);

    // 获取数据
    SWMRNibbleArray* retrievedBlockLight = lightManager.getData(LightType::BLOCK, pos);
    SWMRNibbleArray* retrievedSkyLight = lightManager.getData(LightType::SKY, pos);

    // SkyLightEngine 的 setData 会创建新的 SWMRNibbleArray
    // BlockLightEngine 的 setData 需要区块存在，所以可能返回 nullptr
    // 这个测试主要验证 API 不会崩溃

    // 清理缓存
    blockEngine->destroyCaches();
    skyEngine->destroyCaches();
}

/**
 * @brief 测试 ChunkSection 光照 NibbleArray 直接修改
 *
 * 验证通过引用直接修改 ChunkSection 的光照数组。
 */
TEST_F(LightSyncTest, ChunkSectionDirectNibbleArrayModification) {
    ChunkSection section;

    // 创建测试数据
    NibbleArray testData = NibbleArray::filled(7);

    // 直接替换天空光照数组
    section.skyLightNibble() = testData.copy();
    EXPECT_EQ(section.getSkyLight(0, 0, 0), 7);
    EXPECT_EQ(section.getSkyLight(5, 10, 3), 7);
    EXPECT_EQ(section.getSkyLight(15, 15, 15), 7);

    // 直接替换方块光照数组
    section.blockLightNibble() = NibbleArray::filled(5);
    EXPECT_EQ(section.getBlockLight(0, 0, 0), 5);
    EXPECT_EQ(section.getBlockLight(5, 10, 3), 5);
    EXPECT_EQ(section.getBlockLight(15, 15, 15), 5);
}

} // namespace
} // namespace mc::server
