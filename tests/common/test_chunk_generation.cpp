#include <gtest/gtest.h>
#include <cmath>

#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "common/world/gen/noise/ImprovedNoiseGenerator.hpp"
#include "common/world/gen/noise/OctavesNoiseGenerator.hpp"
#include "common/world/gen/settings/NoiseSettings.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;

// ============================================================================
// ChunkStatus 测试
// ============================================================================

TEST(ChunkStatus, BasicProperties) {
    // EMPTY 是根
    EXPECT_EQ(ChunkStatuses::EMPTY.name(), "empty");
    EXPECT_EQ(ChunkStatuses::EMPTY.ordinal(), 0);
    EXPECT_EQ(ChunkStatuses::EMPTY.parent(), &ChunkStatuses::EMPTY);

    // BIOMES 在新顺序中
    EXPECT_EQ(ChunkStatuses::BIOMES.name(), "biomes");
    EXPECT_EQ(ChunkStatuses::BIOMES.ordinal(), 3);
    EXPECT_EQ(ChunkStatuses::BIOMES.parent(), &ChunkStatuses::STRUCTURE_REFERENCES);

    // FULL 是最后一个
    EXPECT_EQ(ChunkStatuses::FULL.name(), "full");
    EXPECT_EQ(ChunkStatuses::FULL.ordinal(), 12);
    EXPECT_EQ(ChunkStatuses::FULL.parent(), &ChunkStatuses::HEIGHTMAPS);
}

TEST(ChunkStatus, Ordering) {
    // isAtLeast 测试
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::EMPTY));
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::BIOMES));
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::FULL));

    EXPECT_FALSE(ChunkStatuses::EMPTY.isAtLeast(ChunkStatuses::FULL));
    EXPECT_FALSE(ChunkStatuses::BIOMES.isAtLeast(ChunkStatuses::NOISE));

    // isBefore 测试
    EXPECT_TRUE(ChunkStatuses::EMPTY.isBefore(ChunkStatuses::FULL));
    EXPECT_TRUE(ChunkStatuses::BIOMES.isBefore(ChunkStatuses::NOISE));
    EXPECT_FALSE(ChunkStatuses::FULL.isBefore(ChunkStatuses::EMPTY));

    // 比较运算符
    EXPECT_TRUE(ChunkStatuses::EMPTY < ChunkStatuses::FULL);
    EXPECT_TRUE(ChunkStatuses::BIOMES <= ChunkStatuses::BIOMES);
    EXPECT_TRUE(ChunkStatuses::FULL > ChunkStatuses::EMPTY);
}

TEST(ChunkStatus, TaskRange) {
    // STRUCTURE_REFERENCES 阶段需要邻居区块
    EXPECT_EQ(ChunkStatuses::STRUCTURE_REFERENCES.taskRange(), 8);

    // NOISE 阶段需要邻居区块（用于生物群系平滑）
    EXPECT_EQ(ChunkStatuses::NOISE.taskRange(), 8);

    // FEATURES 阶段需要邻居区块
    EXPECT_EQ(ChunkStatuses::FEATURES.taskRange(), 8);

    // 其他阶段不需要邻居或需要较少邻居
    EXPECT_EQ(ChunkStatuses::EMPTY.taskRange(), -1);  // 特殊值
    EXPECT_EQ(ChunkStatuses::BIOMES.taskRange(), 0);
    EXPECT_EQ(ChunkStatuses::FULL.taskRange(), 0);
}

TEST(ChunkStatus, GetAll) {
    const auto& all = ChunkStatus::getAll();
    EXPECT_EQ(all.size(), 13u);  // 13个阶段

    // 验证顺序
    EXPECT_EQ(all[0], ChunkStatuses::EMPTY);
    EXPECT_EQ(all[1], ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(all[2], ChunkStatuses::STRUCTURE_REFERENCES);
    EXPECT_EQ(all[3], ChunkStatuses::BIOMES);
    EXPECT_EQ(all[4], ChunkStatuses::NOISE);
    EXPECT_EQ(all[5], ChunkStatuses::SURFACE);
    EXPECT_EQ(all[6], ChunkStatuses::CARVERS);
    EXPECT_EQ(all[7], ChunkStatuses::LIQUID_CARVERS);
    EXPECT_EQ(all[8], ChunkStatuses::FEATURES);
    EXPECT_EQ(all[9], ChunkStatuses::LIGHT);
    EXPECT_EQ(all[10], ChunkStatuses::SPAWN);
    EXPECT_EQ(all[11], ChunkStatuses::HEIGHTMAPS);
    EXPECT_EQ(all[12], ChunkStatuses::FULL);
}

TEST(ChunkStatus, NewStages) {
    // 验证新增的阶段
    EXPECT_EQ(ChunkStatuses::STRUCTURE_STARTS.name(), "structure_starts");
    EXPECT_EQ(ChunkStatuses::STRUCTURE_REFERENCES.name(), "structure_references");
    EXPECT_EQ(ChunkStatuses::LIQUID_CARVERS.name(), "liquid_carvers");
    EXPECT_EQ(ChunkStatuses::SPAWN.name(), "spawn");

    // 验证阶段顺序
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_STARTS.isBefore(ChunkStatuses::STRUCTURE_REFERENCES));
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_REFERENCES.isBefore(ChunkStatuses::BIOMES));
    EXPECT_TRUE(ChunkStatuses::CARVERS.isBefore(ChunkStatuses::LIQUID_CARVERS));
    EXPECT_TRUE(ChunkStatuses::LIGHT.isBefore(ChunkStatuses::SPAWN));
    EXPECT_TRUE(ChunkStatuses::SPAWN.isBefore(ChunkStatuses::HEIGHTMAPS));
}

TEST(ChunkStatus, HeightmapFlags) {
    // 验证高度图标志
    EXPECT_TRUE(hasFlag(ChunkStatuses::EMPTY.heightmaps(), HeightmapFlag::PRE_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::BIOMES.heightmaps(), HeightmapFlag::PRE_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::NOISE.heightmaps(), HeightmapFlag::PRE_FEATURES));

    EXPECT_TRUE(hasFlag(ChunkStatuses::FEATURES.heightmaps(), HeightmapFlag::POST_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::LIGHT.heightmaps(), HeightmapFlag::POST_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::FULL.heightmaps(), HeightmapFlag::POST_FEATURES));
}

TEST(ChunkStatus, ChunkType) {
    // FULL 是 LEVELCHUNK 类型
    EXPECT_EQ(ChunkStatuses::FULL.type(), ChunkType::LEVELCHUNK);

    // 其他阶段是 PROTOCHUNK 类型
    EXPECT_EQ(ChunkStatuses::EMPTY.type(), ChunkType::PROTOCHUNK);
    EXPECT_EQ(ChunkStatuses::BIOMES.type(), ChunkType::PROTOCHUNK);
    EXPECT_EQ(ChunkStatuses::FEATURES.type(), ChunkType::PROTOCHUNK);
}

TEST(ChunkStatus, ByNameAndOrdinal) {
    // 按名称查找
    const ChunkStatus* status = ChunkStatus::byName("empty");
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatuses::EMPTY);

    status = ChunkStatus::byName("features");
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatuses::FEATURES);

    status = ChunkStatus::byName("structure_starts");
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatuses::STRUCTURE_STARTS);

    // 按序号查找
    status = ChunkStatus::byOrdinal(0);
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatuses::EMPTY);

    status = ChunkStatus::byOrdinal(12);
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatuses::FULL);

    // 无效名称
    status = ChunkStatus::byName("invalid");
    EXPECT_EQ(status, nullptr);

    // 无效序号
    status = ChunkStatus::byOrdinal(-1);
    EXPECT_EQ(status, nullptr);
    status = ChunkStatus::byOrdinal(100);
    EXPECT_EQ(status, nullptr);
}

// ============================================================================
// ChunkPrimer 测试
// ============================================================================

class ChunkPrimerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }
};

TEST_F(ChunkPrimerTest, Creation) {
    ChunkPrimer primer(10, 20);
    EXPECT_EQ(primer.x(), 10);
    EXPECT_EQ(primer.z(), 20);
    EXPECT_EQ(primer.getChunkStatus(), ChunkStatuses::EMPTY);
}

TEST_F(ChunkPrimerTest, SetStatus) {
    ChunkPrimer primer(0, 0);

    primer.setChunkStatus(ChunkStatuses::BIOMES);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::EMPTY));
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::BIOMES));
    EXPECT_FALSE(primer.hasCompletedStatus(ChunkStatuses::NOISE));

    primer.setChunkStatus(ChunkStatuses::FULL);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::EMPTY));
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::BIOMES));
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::NOISE));
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatuses::FULL));
}

// ============================================================================
// SingleChunkLifecycleManager 测试
// ============================================================================

TEST(SingleChunkLifecycleManagerTest, Creation) {
    SingleChunkLifecycleManager holder(5, 10);
    EXPECT_EQ(holder.x(), 5);
    EXPECT_EQ(holder.z(), 10);
    EXPECT_EQ(holder.getStatus(), ChunkStatuses::EMPTY);
    EXPECT_EQ(holder.getLevel(), 33);  // 默认级别
}

TEST(SingleChunkLifecycleManagerTest, SetStatus) {
    SingleChunkLifecycleManager holder(0, 0);

    holder.setStatus(ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(holder.getStatus(), ChunkStatuses::STRUCTURE_STARTS);

    holder.setStatus(ChunkStatuses::NOISE);
    EXPECT_EQ(holder.getStatus(), ChunkStatuses::NOISE);
    EXPECT_TRUE(holder.hasCompletedStatus(ChunkStatuses::STRUCTURE_STARTS));
    EXPECT_TRUE(holder.hasCompletedStatus(ChunkStatuses::BIOMES));
    EXPECT_FALSE(holder.hasCompletedStatus(ChunkStatuses::FEATURES));
}
