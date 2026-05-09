#include <gtest/gtest.h>
#include "entity/ai/pathfinding/WalkNodeProcessor.hpp"
#include "entity/ai/pathfinding/Region.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/Material.hpp"
#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

// ============================================================================
// MockRegion - 用于测试的 Region 实现
// ============================================================================

namespace {

/**
 * @brief 测试用 Mock Region
 *
 * 提供可编程的方块状态，用于测试危险方块检测。
 */
class MockRegion : public Region {
public:
    // 方块状态存储：key = (x << 32) | (y << 16) | z, value = stateId
    std::unordered_map<u64, u32> m_blockStates;

    // 特殊方块标记
    std::unordered_map<u64, bool> m_isWater;
    std::unordered_map<u64, bool> m_isLava;
    std::unordered_map<u64, bool> m_isWalkable;

    // 全局设置
    bool m_globalLoaded = true;
    i32 m_globalHeight = 64;

    // ========== 辅助方法 ==========

    static u64 makeKey(i32 x, i32 y, i32 z) {
        u64 key = 0;
        key |= (static_cast<u64>(static_cast<u32>(x)) & 0xFFFFFFFFULL) << 32;
        key |= (static_cast<u64>(static_cast<u16>(y & 0xFFFF)) << 16);
        key |= (static_cast<u64>(static_cast<u32>(z)) & 0xFFFFULL);
        return key;
    }

    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId) {
        m_blockStates[makeKey(x, y, z)] = stateId;
    }

    void setWater(i32 x, i32 y, i32 z, bool water = true) {
        m_isWater[makeKey(x, y, z)] = water;
    }

    void setLava(i32 x, i32 y, i32 z, bool lava = true) {
        m_isLava[makeKey(x, y, z)] = lava;
    }

    void setWalkable(i32 x, i32 y, i32 z, bool walkable = true) {
        m_isWalkable[makeKey(x, y, z)] = walkable;
    }

    // ========== Region 接口实现 ==========

    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const override {
        auto it = m_blockStates.find(makeKey(x, y, z));
        return it != m_blockStates.end() ? it->second : 0;  // 0 = 空气
    }

    [[nodiscard]] bool isLoaded(i32 /*x*/, i32 /*z*/) const override {
        return m_globalLoaded;
    }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/) const override {
        return m_globalHeight;
    }

    [[nodiscard]] bool isWalkable(i32 x, i32 y, i32 z) const override {
        auto it = m_isWalkable.find(makeKey(x, y, z));
        return it != m_isWalkable.end() ? it->second : false;
    }

    [[nodiscard]] bool isWater(i32 x, i32 y, i32 z) const override {
        auto it = m_isWater.find(makeKey(x, y, z));
        return it != m_isWater.end() ? it->second : false;
    }

    [[nodiscard]] bool isLava(i32 x, i32 y, i32 z) const override {
        auto it = m_isLava.find(makeKey(x, y, z));
        return it != m_isLava.end() ? it->second : false;
    }
};

} // namespace

// ============================================================================
// PathNodeType 危险类型测试
// ============================================================================

TEST(PathNodeTypeDangerTest, GetDangerReturnsCorrectType) {
    // 火焰危险
    EXPECT_EQ(getDanger(PathNodeType::DamageFire), PathNodeType::DangerFire);
    EXPECT_EQ(getDanger(PathNodeType::DangerFire), PathNodeType::DangerFire);

    // 仙人掌危险
    EXPECT_EQ(getDanger(PathNodeType::DamageCactus), PathNodeType::DangerCactus);
    EXPECT_EQ(getDanger(PathNodeType::DangerCactus), PathNodeType::DangerCactus);

    // 其他危险
    EXPECT_EQ(getDanger(PathNodeType::DamageOther), PathNodeType::DangerOther);
    EXPECT_EQ(getDanger(PathNodeType::DangerOther), PathNodeType::DangerOther);

    // 甜浆果丛危险
    EXPECT_EQ(getDanger(PathNodeType::DangerBerry), PathNodeType::DangerBerry);

    // 岩浆映射到火焰危险
    EXPECT_EQ(getDanger(PathNodeType::Lava), PathNodeType::DamageFire);

    // 非危险类型返回 Blocked
    EXPECT_EQ(getDanger(PathNodeType::Walkable), PathNodeType::Blocked);
    EXPECT_EQ(getDanger(PathNodeType::Water), PathNodeType::Blocked);
    EXPECT_EQ(getDanger(PathNodeType::Open), PathNodeType::Blocked);
}

TEST(PathNodeTypeDangerTest, IsDangerousFunction) {
    // 危险类型
    EXPECT_TRUE(isDangerous(PathNodeType::DamageFire));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerFire));
    EXPECT_TRUE(isDangerous(PathNodeType::DamageCactus));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerCactus));
    EXPECT_TRUE(isDangerous(PathNodeType::DamageOther));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerOther));
    EXPECT_TRUE(isDangerous(PathNodeType::DangerBerry));
    EXPECT_TRUE(isDangerous(PathNodeType::Lava));

    // 非危险类型
    EXPECT_FALSE(isDangerous(PathNodeType::Walkable));
    EXPECT_FALSE(isDangerous(PathNodeType::Water));
    EXPECT_FALSE(isDangerous(PathNodeType::Open));
    EXPECT_FALSE(isDangerous(PathNodeType::Blocked));
}

TEST(PathNodeTypeDangerTest, DangerBerryCostPenalty) {
    // 甜浆果丛危险区域代价为 8.0
    EXPECT_FLOAT_EQ(getPathCostPenalty(PathNodeType::DangerBerry), 8.0f);
}

// ============================================================================
// WalkNodeProcessor::isDangerous 测试
// ============================================================================

class WalkNodeProcessorDangerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_processor = std::make_unique<WalkNodeProcessor>();
        m_processor->setRegion(&m_region);
    }

    MockRegion m_region;
    std::unique_ptr<WalkNodeProcessor> m_processor;
};

TEST_F(WalkNodeProcessorDangerTest, LavaIsDangerous) {
    // 设置岩浆
    m_region.setLava(0, 0, 0, true);

    EXPECT_TRUE(m_processor->isDangerous(0, 0, 0));
    EXPECT_FALSE(m_processor->isDangerous(1, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, WaterIsNotDangerous) {
    // 水不是危险方块
    m_region.setWater(0, 0, 0, true);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, AirIsNotDangerous) {
    // 空气不是危险方块
    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, WalkableBlockIsNotDangerous) {
    // 可行走的方块不是危险方块
    m_region.setWalkable(0, 0, 0, true);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, NullRegionReturnsFalse) {
    // 空 Region 返回 false
    m_processor->setRegion(nullptr);

    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
}

// ============================================================================
// WalkNodeProcessor::getNodeType 测试
// ============================================================================

class WalkNodeProcessorNodeTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_processor = std::make_unique<WalkNodeProcessor>();
        m_processor->setRegion(&m_region);
    }

    MockRegion m_region;
    std::unique_ptr<WalkNodeProcessor> m_processor;
};

TEST_F(WalkNodeProcessorNodeTypeTest, UnloadedReturnsBlocked) {
    m_region.m_globalLoaded = false;

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Blocked);
}

TEST_F(WalkNodeProcessorNodeTypeTest, WaterReturnsWaterOrBlocked) {
    // 默认不能游泳
    m_processor->setCanSwim(false);
    m_region.setWater(0, 0, 0, true);

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Blocked);

    // 可以游泳
    m_processor->setCanSwim(true);
    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Water);
}

TEST_F(WalkNodeProcessorNodeTypeTest, LavaReturnsLava) {
    m_region.setLava(0, 0, 0, true);

    EXPECT_EQ(m_processor->getNodeType(0, 0, 0), PathNodeType::Lava);
}

TEST_F(WalkNodeProcessorNodeTypeTest, OpenGroundReturnsWalkable) {
    // 设置地面
    m_region.setWalkable(0, 63, 0, true);
    // 上方是空气（默认）

    EXPECT_EQ(m_processor->getNodeType(0, 64, 0), PathNodeType::Walkable);
}

TEST_F(WalkNodeProcessorNodeTypeTest, OpenAirReturnsOpen) {
    // 空气且没有支撑
    EXPECT_EQ(m_processor->getNodeType(0, 64, 0), PathNodeType::Open);
}

// ============================================================================
// Region::getBlockState 测试
// ============================================================================

TEST(RegionGetBlockStateTest, ReturnsNullptrForAir) {
    MockRegion region;

    // 空气返回 nullptr
    const BlockState* state = region.getBlockState(0, 0, 0);
    EXPECT_EQ(state, nullptr);
}

TEST(RegionGetBlockStateTest, ReturnsNullptrForMissingBlock) {
    MockRegion region;

    // 未设置的方块返回 nullptr（stateId = 0 = 空气）
    const BlockState* state = region.getBlockState(100, 100, 100);
    EXPECT_EQ(state, nullptr);
}

// ============================================================================
// 危险检测距离测试
// ============================================================================

TEST_F(WalkNodeProcessorDangerTest, DetectsAdjacentDanger) {
    // 设置岩浆在相邻位置
    m_region.setLava(1, 0, 0, true);

    // isDangerous 只检查单个位置
    EXPECT_FALSE(m_processor->isDangerous(0, 0, 0));
    EXPECT_TRUE(m_processor->isDangerous(1, 0, 0));
}

TEST_F(WalkNodeProcessorDangerTest, MultipleDangerTypes) {
    // 不同位置设置不同危险
    m_region.setLava(0, 0, 0, true);
    m_region.setLava(2, 0, 0, true);

    EXPECT_TRUE(m_processor->isDangerous(0, 0, 0));
    EXPECT_FALSE(m_processor->isDangerous(1, 0, 0));
    EXPECT_TRUE(m_processor->isDangerous(2, 0, 0));
}

// ============================================================================
// 危险方块代价惩罚测试
// ============================================================================

TEST(PathNodeTypeCostTest, DamageTypesHaveHigherCost) {
    // DAMAGE_FIRE 比 DANGER_FIRE 代价更高
    EXPECT_GT(getPathCostPenalty(PathNodeType::DamageFire),
              getPathCostPenalty(PathNodeType::DangerFire));

    // DAMAGE_CACTUS 不可通行，DANGER_CACTUS 高代价但可通行
    EXPECT_LT(getPathCostPenalty(PathNodeType::DamageCactus), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerCactus), 0.0f);

    // DAMAGE_OTHER 不可通行，DANGER_OTHER 高代价但可通行
    EXPECT_LT(getPathCostPenalty(PathNodeType::DamageOther), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerOther), 0.0f);
}

TEST(PathNodeTypeCostTest, AllDangerTypesPenalized) {
    // 所有危险类型都有正代价（高代价但可通行）
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerFire), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerCactus), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerOther), 0.0f);
    EXPECT_GT(getPathCostPenalty(PathNodeType::DangerBerry), 0.0f);
}
