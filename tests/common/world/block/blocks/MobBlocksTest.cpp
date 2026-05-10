#include <gtest/gtest.h>
#include "world/block/blocks/mob/MobBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "util/property/Properties.hpp"
#include "util/Direction.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== BeehiveBlock 测试 ==========

class BeehiveBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建蜂巢方块
        beehive_ = std::make_unique<BeehiveBlock>(
            BlockProperties(Material::WOOD)
                .hardness(0.6f)
                .resistance(0.6f)
        );
    }

    std::unique_ptr<BeehiveBlock> beehive_;
};

// ========== 基础属性测试 ==========

TEST_F(BeehiveBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(beehive_, nullptr);
}

TEST_F(BeehiveBlockTest, HasBlockEntity_ReturnsTrue) {
    EXPECT_TRUE(beehive_->hasBlockEntity());
}

TEST_F(BeehiveBlockTest, GetMaxHoneyLevel_Returns5) {
    EXPECT_EQ(beehive_->getMaxHoneyLevel(), 5);
}

// ========== 蜂蜜等级测试 ==========

TEST_F(BeehiveBlockTest, GetHoneyLevel_ReturnsZeroByDefault) {
    const auto& state = beehive_->defaultState();
    EXPECT_EQ(beehive_->getHoneyLevel(state), 0);
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_ReturnsCorrectState) {
    // 测试各个等级
    for (i32 level = 0; level <= 5; ++level) {
        BlockState state = beehive_->withHoneyLevel(level);
        EXPECT_EQ(beehive_->getHoneyLevel(state), level)
            << "Honey level should be " << level;
    }
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_ClampsToValidRange) {
    // 测试超出范围的值
    BlockState stateNegative = beehive_->withHoneyLevel(-1);
    EXPECT_EQ(beehive_->getHoneyLevel(stateNegative), 0)
        << "Negative level should be clamped to 0";

    BlockState stateOverflow = beehive_->withHoneyLevel(10);
    EXPECT_EQ(beehive_->getHoneyLevel(stateOverflow), 5)
        << "Overflow level should be clamped to 5";
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_PreservesOtherProperties) {
    // 获取默认状态的朝向
    const auto& defaultState = beehive_->defaultState();
    Direction defaultFacing = defaultState.get(BlockStateProperties::HORIZONTAL_FACING());

    // 设置蜂蜜等级后朝向应该保持不变
    BlockState state = beehive_->withHoneyLevel(3);
    Direction facingAfter = state.get(BlockStateProperties::HORIZONTAL_FACING());

    EXPECT_EQ(facingAfter, defaultFacing)
        << "Honey level change should not affect facing";
}

// ========== 朝向属性测试 ==========

TEST_F(BeehiveBlockTest, DefaultState_HasCorrectFacing) {
    const auto& state = beehive_->defaultState();
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North) << "Default facing should be North";
}

TEST_F(BeehiveBlockTest, Rotate_UpdatesFacingCorrectly) {
    const auto& defaultState = beehive_->defaultState();

    // 测试所有旋转
    // North -> Clockwise90 -> East
    const auto& rotated90 = beehive_->rotate(defaultState, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // North -> Clockwise180 -> South
    const auto& rotated180 = beehive_->rotate(defaultState, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // North -> CounterClockwise90 -> West
    const auto& rotated270 = beehive_->rotate(defaultState, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated270.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(BeehiveBlockTest, Rotate_PreservesHoneyLevel) {
    // 创建蜂蜜等级为 3 的状态
    BlockState state = beehive_->withHoneyLevel(3);
    i32 honeyLevelBefore = beehive_->getHoneyLevel(state);

    // 旋转
    const auto& rotated = beehive_->rotate(state, Rotation::Clockwise90);
    i32 honeyLevelAfter = beehive_->getHoneyLevel(rotated);

    EXPECT_EQ(honeyLevelAfter, honeyLevelBefore)
        << "Rotation should not affect honey level";
}

TEST_F(BeehiveBlockTest, Mirror_UpdatesFacingCorrectly) {
    const auto& defaultState = beehive_->defaultState();

    // LeftRight 镜像：沿 Z 轴镜像，东西互换，南北不变
    // North -> Mirror(LR) -> North (不变)
    const auto& mirroredLR = beehive_->mirror(defaultState, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // East -> Mirror(LR) -> West (东西互换)
    BlockState eastState = defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const auto& mirroredEastLR = beehive_->mirror(eastState, Mirror::LeftRight);
    EXPECT_EQ(mirroredEastLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);

    // FrontBack 镜像：沿 X 轴镜像，南北互换，东西不变
    // North -> Mirror(FB) -> South (南北互换)
    const auto& mirroredFB = beehive_->mirror(defaultState, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // East -> Mirror(FB) -> East (东西不变)
    const auto& mirroredEastFB = beehive_->mirror(eastState, Mirror::FrontBack);
    EXPECT_EQ(mirroredEastFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

// ========== 状态容器测试 ==========

TEST_F(BeehiveBlockTest, StateContainer_HasHoneyLevelProperty) {
    // 验证状态容器包含蜂蜜等级属性
    const auto& state = beehive_->defaultState();
    // 如果能获取属性值且不抛异常，说明属性存在
    EXPECT_NO_THROW({
        [[maybe_unused]] i32 level = state.get(BlockStateProperties::HONEY_LEVEL_0_5());
    });
}

TEST_F(BeehiveBlockTest, StateContainer_HasFacingProperty) {
    // 验证状态容器包含朝向属性
    const auto& state = beehive_->defaultState();
    EXPECT_NO_THROW({
        [[maybe_unused]] Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    });
}

// ========== TurtleEggBlock 测试（同文件中的另一个方块）==========

class TurtleEggBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        turtleEgg_ = std::make_unique<TurtleEggBlock>(
            BlockProperties(Material::SAND)
                .hardness(0.5f)
                .resistance(0.5f)
        );
    }

    std::unique_ptr<TurtleEggBlock> turtleEgg_;
};

TEST_F(TurtleEggBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(turtleEgg_, nullptr);
}

TEST_F(TurtleEggBlockTest, GetEggs_ReturnsOneByDefault) {
    const auto& state = turtleEgg_->defaultState();
    EXPECT_EQ(turtleEgg_->getEggs(state), 1);
}

TEST_F(TurtleEggBlockTest, GetHatch_ReturnsZeroByDefault) {
    const auto& state = turtleEgg_->defaultState();
    EXPECT_EQ(turtleEgg_->getHatch(state), 0);
}

TEST_F(TurtleEggBlockTest, WithEggs_ClampsToValidRange) {
    BlockState state = turtleEgg_->withEggs(0);
    EXPECT_EQ(turtleEgg_->getEggs(state), 1) << "Minimum eggs should be 1";

    state = turtleEgg_->withEggs(5);
    EXPECT_EQ(turtleEgg_->getEggs(state), 4) << "Maximum eggs should be 4";
}

TEST_F(TurtleEggBlockTest, WithHatch_ClampsToValidRange) {
    BlockState state = turtleEgg_->withHatch(-1);
    EXPECT_EQ(turtleEgg_->getHatch(state), 0) << "Minimum hatch should be 0";

    state = turtleEgg_->withHatch(5);
    EXPECT_EQ(turtleEgg_->getHatch(state), 2) << "Maximum hatch should be 2";
}

TEST_F(TurtleEggBlockTest, GetShape_ReturnsValidShape) {
    for (i32 eggs = 1; eggs <= 4; ++eggs) {
        BlockState state = turtleEgg_->withEggs(eggs);
        const auto& shape = turtleEgg_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Eggs " << eggs << " should have valid shape";
    }
}

TEST_F(TurtleEggBlockTest, TicksRandomly_ReturnsTrue) {
    EXPECT_TRUE(turtleEgg_->ticksRandomly());
}

TEST_F(TurtleEggBlockTest, IsOpaque_ReturnsFalse) {
    const auto& state = turtleEgg_->defaultState();
    EXPECT_FALSE(turtleEgg_->isOpaque(state));
}

// ========== InfestedBlock 测试 ==========

class InfestedBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建被感染方块（使用石头作为宿主）
        infested_ = std::make_unique<InfestedBlock>(
            1,  // 石头方块 ID（假设）
            BlockProperties(Material::ROCK)
                .hardness(0.75f)
                .resistance(0.75f)
        );
    }

    std::unique_ptr<InfestedBlock> infested_;
};

TEST_F(InfestedBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(infested_, nullptr);
}

TEST_F(InfestedBlockTest, GetHostBlock_ReturnsCorrectId) {
    EXPECT_EQ(infested_->getHostBlock(), 1u);
}

// ========== SpawnerBlock 测试 ==========

class SpawnerBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        spawner_ = std::make_unique<SpawnerBlock>(
            BlockProperties(Material::ROCK)
                .hardness(5.0f)
                .resistance(5.0f)
        );
    }

    std::unique_ptr<SpawnerBlock> spawner_;
};

TEST_F(SpawnerBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(spawner_, nullptr);
}

TEST_F(SpawnerBlockTest, HasBlockEntity_ReturnsTrue) {
    EXPECT_TRUE(spawner_->hasBlockEntity());
}

TEST_F(SpawnerBlockTest, IsOpaque_ReturnsFalse) {
    const auto& state = spawner_->defaultState();
    EXPECT_FALSE(spawner_->isOpaque(state));
}

// ========== DragonBreathBlock 测试 ==========

class DragonBreathBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        dragonBreath_ = std::make_unique<DragonBreathBlock>(
            BlockProperties(Material::FIRE)
                .hardness(0.0f)
                .resistance(0.0f)
        );
    }

    std::unique_ptr<DragonBreathBlock> dragonBreath_;
};

TEST_F(DragonBreathBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(dragonBreath_, nullptr);
}

TEST_F(DragonBreathBlockTest, GetShape_ReturnsEmptyShape) {
    const auto& state = dragonBreath_->defaultState();
    const auto& shape = dragonBreath_->getShape(state);
    EXPECT_TRUE(shape.isEmpty()) << "Dragon breath should have empty shape";
}

TEST_F(DragonBreathBlockTest, GetCollisionShape_ReturnsEmptyShape) {
    const auto& state = dragonBreath_->defaultState();
    const auto& shape = dragonBreath_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty()) << "Dragon breath should have no collision";
}

TEST_F(DragonBreathBlockTest, IsOpaque_ReturnsFalse) {
    const auto& state = dragonBreath_->defaultState();
    EXPECT_FALSE(dragonBreath_->isOpaque(state));
}
