#include <gtest/gtest.h>

#include "entity/core/BoostHelper.hpp"
#include "entity/interfaces/IRideable.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"
#include "entity/entities/vehicle/BoatEntity.hpp"
#include "entity/entities/vehicle/MinecartEntity.hpp"
#include "entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "entity/core/DataParameter.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::math;

// ============================================================================
// BoostHelper 测试
// ============================================================================

class BoostHelperTest : public ::testing::Test {
protected:
    BoostHelper helper;
};

TEST_F(BoostHelperTest, InitialState) {
    EXPECT_FALSE(helper.saddledRaw);
    EXPECT_EQ(helper.boostingTick, 0);
    EXPECT_EQ(helper.boostTimeRaw, 0);
    EXPECT_FALSE(helper.getSaddled());
    EXPECT_EQ(helper.getBoostTime(), 0);
    EXPECT_FALSE(helper.isBoosting());
}

TEST_F(BoostHelperTest, SetSaddle) {
    helper.setSaddled(true);
    EXPECT_TRUE(helper.getSaddled());

    helper.setSaddled(false);
    EXPECT_FALSE(helper.getSaddled());
}

TEST_F(BoostHelperTest, SetBoostTime) {
    helper.setBoostTime(100);
    EXPECT_EQ(helper.getBoostTime(), 100);
    EXPECT_EQ(helper.boostTimeRaw, 100);
}

TEST_F(BoostHelperTest, BoostWhileSaddledFails) {
    helper.saddledRaw = true;  // 设置加速状态
    Random rng(12345);
    EXPECT_FALSE(helper.boost(rng));  // 已经在加速中，不能再加速
}

TEST_F(BoostHelperTest, BoostSuccess) {
    Random rng(12345);
    ASSERT_TRUE(helper.boost(rng));

    EXPECT_TRUE(helper.saddledRaw);
    EXPECT_EQ(helper.boostingTick, 0);
    EXPECT_GE(helper.boostTimeRaw, 140);
    EXPECT_LE(helper.boostTimeRaw, 980);
    EXPECT_TRUE(helper.isBoosting());
}

TEST_F(BoostHelperTest, TickUpdatesBoosting) {
    Random rng(12345);
    helper.boost(rng);

    EXPECT_TRUE(helper.tick());  // tick 1
    EXPECT_EQ(helper.boostingTick, 1);

    EXPECT_TRUE(helper.tick());  // tick 2
    EXPECT_EQ(helper.boostingTick, 2);
}

TEST_F(BoostHelperTest, TickEndsBoost) {
    // 手动设置短时间的加速
    helper.saddledRaw = true;
    helper.boostingTick = 0;
    helper.boostTimeRaw = 3;  // 3 ticks后结束

    EXPECT_TRUE(helper.tick());  // tick 1 -> boostingTick = 1
    EXPECT_EQ(helper.boostingTick, 1);
    EXPECT_TRUE(helper.saddledRaw);

    EXPECT_TRUE(helper.tick());  // tick 2 -> boostingTick = 2
    EXPECT_EQ(helper.boostingTick, 2);

    EXPECT_TRUE(helper.tick());  // tick 3 -> boostingTick = 3
    EXPECT_EQ(helper.boostingTick, 3);

    // tick 4 -> boostingTick = 4 > boostTimeRaw = 3, 结束加速
    EXPECT_FALSE(helper.tick());
    EXPECT_FALSE(helper.saddledRaw);
}

TEST_F(BoostHelperTest, GetBoostProgress) {
    helper.boostTimeRaw = 100;
    helper.boostingTick = 0;

    EXPECT_FLOAT_EQ(helper.getBoostProgress(), 0.0f);

    helper.boostingTick = 50;
    EXPECT_FLOAT_EQ(helper.getBoostProgress(), 0.5f);

    helper.boostingTick = 100;
    EXPECT_FLOAT_EQ(helper.getBoostProgress(), 1.0f);
}

TEST_F(BoostHelperTest, Reset) {
    helper.setBoostTime(200);
    helper.reset();

    EXPECT_TRUE(helper.saddledRaw);
    EXPECT_EQ(helper.boostingTick, 0);
    EXPECT_EQ(helper.boostTimeRaw, 200);  // reset() uses m_boostTime which was set to 200
}

// ============================================================================
// BoatEntity 测试
// ============================================================================

class BoatEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        boat = std::make_unique<BoatEntity>(BoatEntity::Type::OAK);
    }

    std::unique_ptr<BoatEntity> boat;
};

TEST_F(BoatEntityTest, Dimensions) {
    EXPECT_FLOAT_EQ(boat->width(), 1.375f);
    EXPECT_FLOAT_EQ(boat->height(), 0.5625f);
    EXPECT_FLOAT_EQ(boat->eyeHeight(), 0.5625f);
}

TEST_F(BoatEntityTest, InitialStatus) {
    EXPECT_EQ(boat->getStatus(), BoatStatus::InWater);
}

TEST_F(BoatEntityTest, MountedYOffset) {
    // MC 1.16.5: BoatEntity.getMountedYOffset() -> -0.1D
    EXPECT_DOUBLE_EQ(boat->getMountedYOffset(), -0.1);
}

TEST_F(BoatEntityTest, MaxPassengers) {
    EXPECT_EQ(boat->getMaxPassengers(), 2);
}

TEST_F(BoatEntityTest, BoatType) {
    EXPECT_EQ(boat->getBoatType(), BoatEntity::Type::OAK);

    boat->setBoatType(BoatEntity::Type::BIRCH);
    EXPECT_EQ(boat->getBoatType(), BoatEntity::Type::BIRCH);
}

TEST_F(BoatEntityTest, HandleInput) {
    boat->handleInput(true, false, true, false);

    // Input is stored internally - paddle states are updated during tick based on input
    // The input states are: left=true, right=false, forward=true, backward=false
    // paddle state depends on tick processing
}

TEST_F(BoatEntityTest, PaddleState) {
    // Initial paddle positions are 0
    EXPECT_FLOAT_EQ(boat->getPaddleState(0), 0.0f);
    EXPECT_FLOAT_EQ(boat->getPaddleState(1), 0.0f);

    // Invalid index returns 0
    EXPECT_FLOAT_EQ(boat->getPaddleState(-1), 0.0f);
    EXPECT_FLOAT_EQ(boat->getPaddleState(2), 0.0f);
}

// ============================================================================
// MinecartEntity 测试
// ============================================================================

class MinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        minecart = std::make_unique<AbstractMinecartEntity>(AbstractMinecartEntity::Type::Rideable, EntityId(1));
    }

    std::unique_ptr<AbstractMinecartEntity> minecart;
};

TEST_F(MinecartEntityTest, Dimensions) {
    EXPECT_FLOAT_EQ(minecart->width(), 0.98f);
    EXPECT_FLOAT_EQ(minecart->height(), 0.7f);
    EXPECT_FLOAT_EQ(minecart->eyeHeight(), 0.0f);
}

TEST_F(MinecartEntityTest, MaxSpeed) {
    EXPECT_FLOAT_EQ(minecart->getMaxSpeed(), 0.4f);
}

TEST_F(MinecartEntityTest, MaxSpeedAirLateral) {
    EXPECT_FLOAT_EQ(minecart->getMaxSpeedAirLateral(), 0.4f);
}

TEST_F(MinecartEntityTest, MaxSpeedAirVertical) {
    EXPECT_FLOAT_EQ(minecart->getMaxSpeedAirVertical(), -1.0f);
}

TEST_F(MinecartEntityTest, DragAir) {
    EXPECT_FLOAT_EQ(minecart->getDragAir(), 0.95f);
}

TEST_F(MinecartEntityTest, MountedYOffset) {
    EXPECT_DOUBLE_EQ(minecart->getMountedYOffset(), 0.0);
}

TEST_F(MinecartEntityTest, MinecartType) {
    EXPECT_EQ(minecart->getMinecartType(), AbstractMinecartEntity::Type::Rideable);
}

TEST_F(MinecartEntityTest, Damage) {
    minecart->setDamage(10);
    EXPECT_EQ(minecart->getDamage(), 10);
}

TEST_F(MinecartEntityTest, RollingAmplitude) {
    EXPECT_EQ(minecart->getRollingAmplitude(), 0);
}

TEST_F(MinecartEntityTest, RollingDirection) {
    EXPECT_EQ(minecart->getRollingDirection(), 1);
}

TEST_F(MinecartEntityTest, RailState) {
    EXPECT_FALSE(minecart->isOnRail());
    EXPECT_EQ(minecart->getRailShape(), RailShape::NorthSouth);
}

TEST_F(MinecartEntityTest, ApplyForce) {
    minecart->applyForce(0.1f, 0.2f);
    // Force should be added to velocity
}

// ============================================================================
// FurnaceMinecartEntity 测试
// ============================================================================

class FurnaceMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        furnaceMinecart = std::make_unique<FurnaceMinecartEntity>(EntityId(1));
    }

    std::unique_ptr<FurnaceMinecartEntity> furnaceMinecart;
};

TEST_F(FurnaceMinecartEntityTest, MaxSpeed) {
    // MC 1.16.5: Furnace minecart has lower max speed (0.2D)
    EXPECT_FLOAT_EQ(furnaceMinecart->getMaxSpeed(), 0.2f);
}

TEST_F(FurnaceMinecartEntityTest, NotActivatedInitially) {
    EXPECT_FALSE(furnaceMinecart->isActivated());
}

TEST_F(FurnaceMinecartEntityTest, ActivateAddsFuel) {
    furnaceMinecart->activate();
    EXPECT_TRUE(furnaceMinecart->isActivated());
    EXPECT_GT(furnaceMinecart->getFuel(), 0);
}

TEST_F(FurnaceMinecartEntityTest, AddFuel) {
    furnaceMinecart->addFuel(100);
    EXPECT_EQ(furnaceMinecart->getFuel(), 100);
}

// ============================================================================
// HopperMinecartEntity 测试
// ============================================================================

class HopperMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        hopperMinecart = std::make_unique<HopperMinecartEntity>(EntityId(1));
    }

    std::unique_ptr<HopperMinecartEntity> hopperMinecart;
};

TEST_F(HopperMinecartEntityTest, InventorySize) {
    EXPECT_EQ(hopperMinecart->getContainerSize(), 5);
}

TEST_F(HopperMinecartEntityTest, InitialInventoryEmpty) {
    EXPECT_TRUE(hopperMinecart->isInventoryEmpty());
}

TEST_F(HopperMinecartEntityTest, NotDisabledInitially) {
    EXPECT_FALSE(hopperMinecart->isDisabled());
}

TEST_F(HopperMinecartEntityTest, CanSuckItemsWhenNotDisabled) {
    EXPECT_TRUE(hopperMinecart->canSuckItems());
}

TEST_F(HopperMinecartEntityTest, DisabledCannotSuckItems) {
    hopperMinecart->setDisabled(true);
    EXPECT_TRUE(hopperMinecart->isDisabled());
    EXPECT_FALSE(hopperMinecart->canSuckItems());
}

// ============================================================================
// ChestMinecartEntity 测试（补充）
// ============================================================================

class ChestMinecartEntityRidingTest : public ::testing::Test {
protected:
    void SetUp() override {
        chestMinecart = std::make_unique<ChestMinecartEntity>(EntityId(1));
    }

    std::unique_ptr<ChestMinecartEntity> chestMinecart;
};

TEST_F(ChestMinecartEntityRidingTest, InventorySizeTest) {
    EXPECT_EQ(chestMinecart->getContainerSize(), 27);
}

TEST_F(ChestMinecartEntityRidingTest, InitialInventoryEmptyTest) {
    EXPECT_TRUE(chestMinecart->isInventoryEmpty());
}

// ============================================================================
// TNTMinecartEntity 测试
// ============================================================================

class TNTMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        tntMinecart = std::make_unique<TNTMinecartEntity>(EntityId(1));
    }

    std::unique_ptr<TNTMinecartEntity> tntMinecart;
};

TEST_F(TNTMinecartEntityTest, NotPrimedInitially) {
    EXPECT_FALSE(tntMinecart->isPrimed());
}

TEST_F(TNTMinecartEntityTest, PrimeSetsFuse) {
    tntMinecart->prime(100);
    EXPECT_TRUE(tntMinecart->isPrimed());
}

TEST_F(TNTMinecartEntityTest, PrimeDefaultFuse) {
    tntMinecart->prime();
    EXPECT_TRUE(tntMinecart->isPrimed());
    // Default fuse is 80 ticks
}

// ============================================================================
// RideableMinecartEntity 测试
// ============================================================================

class RideableMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        rideableMinecart = std::make_unique<RideableMinecartEntity>(EntityId(1));
    }

    std::unique_ptr<RideableMinecartEntity> rideableMinecart;
};

TEST_F(RideableMinecartEntityTest, IsRideable) {
    EXPECT_EQ(rideableMinecart->getMinecartType(), AbstractMinecartEntity::Type::Rideable);
}

// ============================================================================
// IRideable 接口测试
// ============================================================================

class IRideableTest : public ::testing::Test {
protected:
    // PigEntity 实现了 IRideable，可以用于测试
};

// 注意: PigEntity 测试需要完整的 World 环境，这里只测试基本接口
// 实际集成测试应在模拟世界环境中进行
