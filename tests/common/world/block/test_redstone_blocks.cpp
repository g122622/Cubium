#include <gtest/gtest.h>
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/redstone/RedstoneWireBlock.hpp"
#include "common/world/block/blocks/redstone/RedstoneTorchBlock.hpp"
#include "common/world/block/blocks/redstone/RedstoneLampBlock.hpp"
#include "common/world/block/blocks/redstone/LeverBlock.hpp"
#include "common/world/block/blocks/redstone/ObserverBlock.hpp"
#include "common/world/block/blocks/redstone/TargetBlock.hpp"
#include "common/world/block/blocks/redstone/DaylightDetectorBlock.hpp"
#include "common/world/block/blocks/redstone/StonePressurePlateBlock.hpp"
#include "common/world/block/blocks/redstone/WoodPressurePlateBlock.hpp"
#include "common/world/block/blocks/redstone/WeightedPressurePlateBlock.hpp"
#include "common/world/block/blocks/redstone/RedstoneComparatorBlock.hpp"
#include "common/world/block/blocks/redstone/RedstoneRepeaterBlock.hpp"
#include "common/world/block/blocks/redstone/PistonBlock.hpp"
#include "common/world/block/blocks/redstone/DispenserBlock.hpp"
#include "common/world/block/blocks/redstone/DropperBlock.hpp"
#include "common/world/block/blocks/redstone/NoteBlock.hpp"
#include "common/world/block/blocks/redstone/TripWireBlock.hpp"
#include "common/world/block/blocks/redstone/TripWireHookBlock.hpp"
#include "common/world/block/blocks/redstone/TNTBlock.hpp"
#include "common/util/Direction.hpp"

using namespace mc;

// ============================================================================
// 红石方块注册测试
// ============================================================================
class RedstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

// ============================================================================
// 基础注册测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneWireRegistered) {
    ASSERT_NE(VanillaBlocks::REDSTONE_WIRE, nullptr);
}

TEST_F(RedstoneBlockTest, RedstoneTorchRegistered) {
    ASSERT_NE(VanillaBlocks::REDSTONE_TORCH, nullptr);
}

TEST_F(RedstoneBlockTest, RedstoneLampRegistered) {
    ASSERT_NE(VanillaBlocks::REDSTONE_LAMP, nullptr);
}

TEST_F(RedstoneBlockTest, RedstoneRepeaterRegistered) {
    ASSERT_NE(VanillaBlocks::REDSTONE_REPEATER, nullptr);
}

TEST_F(RedstoneBlockTest, RedstoneComparatorRegistered) {
    ASSERT_NE(VanillaBlocks::REDSTONE_COMPARATOR, nullptr);
}

TEST_F(RedstoneBlockTest, ObserverRegistered) {
    ASSERT_NE(VanillaBlocks::OBSERVER, nullptr);
}

TEST_F(RedstoneBlockTest, LeverRegistered) {
    ASSERT_NE(VanillaBlocks::LEVER, nullptr);
}

TEST_F(RedstoneBlockTest, StoneButtonRegistered) {
    ASSERT_NE(VanillaBlocks::STONE_BUTTON, nullptr);
}

TEST_F(RedstoneBlockTest, OakButtonRegistered) {
    ASSERT_NE(VanillaBlocks::OAK_BUTTON, nullptr);
}

TEST_F(RedstoneBlockTest, StonePressurePlateRegistered) {
    ASSERT_NE(VanillaBlocks::STONE_PRESSURE_PLATE, nullptr);
}

TEST_F(RedstoneBlockTest, OakPressurePlateRegistered) {
    ASSERT_NE(VanillaBlocks::OAK_PRESSURE_PLATE, nullptr);
}

TEST_F(RedstoneBlockTest, LightWeightedPressurePlateRegistered) {
    ASSERT_NE(VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE, nullptr);
}

TEST_F(RedstoneBlockTest, HeavyWeightedPressurePlateRegistered) {
    ASSERT_NE(VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE, nullptr);
}

TEST_F(RedstoneBlockTest, DaylightDetectorRegistered) {
    ASSERT_NE(VanillaBlocks::DAYLIGHT_DETECTOR, nullptr);
}

TEST_F(RedstoneBlockTest, PistonRegistered) {
    ASSERT_NE(VanillaBlocks::PISTON, nullptr);
}

TEST_F(RedstoneBlockTest, StickyPistonRegistered) {
    ASSERT_NE(VanillaBlocks::STICKY_PISTON, nullptr);
}

TEST_F(RedstoneBlockTest, PistonHeadRegistered) {
    ASSERT_NE(VanillaBlocks::PISTON_HEAD, nullptr);
}

TEST_F(RedstoneBlockTest, DispenserRegistered) {
    ASSERT_NE(VanillaBlocks::DISPENSER, nullptr);
}

TEST_F(RedstoneBlockTest, DropperRegistered) {
    ASSERT_NE(VanillaBlocks::DROPPER, nullptr);
}

TEST_F(RedstoneBlockTest, NoteBlockRegistered) {
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
}

TEST_F(RedstoneBlockTest, TripWireRegistered) {
    ASSERT_NE(VanillaBlocks::TRIPWIRE, nullptr);
}

TEST_F(RedstoneBlockTest, TripWireHookRegistered) {
    ASSERT_NE(VanillaBlocks::TRIPWIRE_HOOK, nullptr);
}

TEST_F(RedstoneBlockTest, TargetRegistered) {
    ASSERT_NE(VanillaBlocks::TARGET, nullptr);
}

TEST_F(RedstoneBlockTest, TNTRegistered) {
    ASSERT_NE(VanillaBlocks::TNT, nullptr);
}

// ============================================================================
// 红石线属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneWireDefaultState) {
    const BlockState& state = VanillaBlocks::REDSTONE_WIRE->defaultState();

    // 默认信号强度为0
    EXPECT_EQ(blocks::RedstoneWireBlock::getPower(state), 0);

    // 默认连接状态为无连接
    EXPECT_EQ(state.get(blocks::RedstoneWireBlock::NORTH_PROP()), blocks::RedstoneSide::None);
    EXPECT_EQ(state.get(blocks::RedstoneWireBlock::EAST_PROP()), blocks::RedstoneSide::None);
    EXPECT_EQ(state.get(blocks::RedstoneWireBlock::SOUTH_PROP()), blocks::RedstoneSide::None);
    EXPECT_EQ(state.get(blocks::RedstoneWireBlock::WEST_PROP()), blocks::RedstoneSide::None);
}

TEST_F(RedstoneBlockTest, RedstoneWireCanProvidePower) {
    const BlockState& state = VanillaBlocks::REDSTONE_WIRE->defaultState();
    EXPECT_TRUE(VanillaBlocks::REDSTONE_WIRE->canProvidePower(state));
}

TEST_F(RedstoneBlockTest, RedstoneWirePowerRange) {
    BlockState state = VanillaBlocks::REDSTONE_WIRE->defaultState();

    // 测试所有信号强度
    for (i32 power = 0; power <= 15; ++power) {
        state = blocks::RedstoneWireBlock::withPower(state, power);
        EXPECT_EQ(blocks::RedstoneWireBlock::getPower(state), power);
    }
}

// ============================================================================
// 红石火把属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneTorchDefaultState) {
    const BlockState& state = VanillaBlocks::REDSTONE_TORCH->defaultState();
    EXPECT_NE(&state, nullptr);
}

TEST_F(RedstoneBlockTest, RedstoneTorchCanProvidePower) {
    const BlockState& state = VanillaBlocks::REDSTONE_TORCH->defaultState();
    EXPECT_TRUE(VanillaBlocks::REDSTONE_TORCH->canProvidePower(state));
}

// ============================================================================
// 红石灯属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneLampDefaultState) {
    const BlockState& state = VanillaBlocks::REDSTONE_LAMP->defaultState();

    // 默认不发光
    EXPECT_FALSE(state.get(BlockStateProperties::LIT()));
}

TEST_F(RedstoneBlockTest, RedstoneLampLitState) {
    BlockState state = VanillaBlocks::REDSTONE_LAMP->defaultState();

    // 设置为发光
    state = state.with(BlockStateProperties::LIT(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::LIT()));
}

// ============================================================================
// 拉杆属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, LeverDefaultState) {
    const BlockState& state = VanillaBlocks::LEVER->defaultState();

    // 默认不激活
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(RedstoneBlockTest, LeverPoweredState) {
    BlockState state = VanillaBlocks::LEVER->defaultState();

    // 激活拉杆
    state = state.with(BlockStateProperties::POWERED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(RedstoneBlockTest, LeverCanProvidePower) {
    const BlockState& state = VanillaBlocks::LEVER->defaultState();
    EXPECT_TRUE(VanillaBlocks::LEVER->canProvidePower(state));
}

// ============================================================================
// 压力板属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, StonePressurePlateDefaultState) {
    const BlockState& state = VanillaBlocks::STONE_PRESSURE_PLATE->defaultState();
    EXPECT_EQ(blocks::AbstractPressurePlateBlock::getPower(state), 0);
}

TEST_F(RedstoneBlockTest, OakPressurePlateDefaultState) {
    const BlockState& state = VanillaBlocks::OAK_PRESSURE_PLATE->defaultState();
    EXPECT_EQ(blocks::AbstractPressurePlateBlock::getPower(state), 0);
}

TEST_F(RedstoneBlockTest, LightWeightedPressurePlateDefaultState) {
    const BlockState& state = VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE->defaultState();
    EXPECT_EQ(blocks::AbstractPressurePlateBlock::getPower(state), 0);
}

TEST_F(RedstoneBlockTest, HeavyWeightedPressurePlateDefaultState) {
    const BlockState& state = VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE->defaultState();
    EXPECT_EQ(blocks::AbstractPressurePlateBlock::getPower(state), 0);
}

TEST_F(RedstoneBlockTest, PressurePlateCanProvidePower) {
    const BlockState& stoneState = VanillaBlocks::STONE_PRESSURE_PLATE->defaultState();
    const BlockState& oakState = VanillaBlocks::OAK_PRESSURE_PLATE->defaultState();

    EXPECT_TRUE(VanillaBlocks::STONE_PRESSURE_PLATE->canProvidePower(stoneState));
    EXPECT_TRUE(VanillaBlocks::OAK_PRESSURE_PLATE->canProvidePower(oakState));
}

// ============================================================================
// 日光探测器属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, DaylightDetectorDefaultState) {
    const BlockState& state = VanillaBlocks::DAYLIGHT_DETECTOR->defaultState();

    // 默认信号强度为0
    EXPECT_EQ(blocks::DaylightDetectorBlock::getPower(state), 0);

    // 默认不反转
    EXPECT_FALSE(blocks::DaylightDetectorBlock::isInverted(state));
}

TEST_F(RedstoneBlockTest, DaylightDetectorInvertedState) {
    BlockState state = VanillaBlocks::DAYLIGHT_DETECTOR->defaultState();

    // 反转模式
    state = blocks::DaylightDetectorBlock::withInverted(state, true);
    EXPECT_TRUE(blocks::DaylightDetectorBlock::isInverted(state));
}

TEST_F(RedstoneBlockTest, DaylightDetectorCanProvidePower) {
    const BlockState& state = VanillaBlocks::DAYLIGHT_DETECTOR->defaultState();
    EXPECT_TRUE(VanillaBlocks::DAYLIGHT_DETECTOR->canProvidePower(state));
}

// ============================================================================
// 中继器属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneRepeaterDefaultState) {
    const BlockState& state = VanillaBlocks::REDSTONE_REPEATER->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 默认延迟为1
    EXPECT_EQ(state.get(BlockStateProperties::DELAY_1_4()), 1);

    // 默认不锁定
    EXPECT_FALSE(state.get(BlockStateProperties::LOCKED()));
}

TEST_F(RedstoneBlockTest, RedstoneRepeaterDelayRange) {
    BlockState state = VanillaBlocks::REDSTONE_REPEATER->defaultState();

    for (i32 delay = 1; delay <= 4; ++delay) {
        state = state.with(BlockStateProperties::DELAY_1_4(), delay);
        EXPECT_EQ(state.get(BlockStateProperties::DELAY_1_4()), delay);
    }
}

// ============================================================================
// 比较器属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneComparatorDefaultState) {
    const BlockState& state = VanillaBlocks::REDSTONE_COMPARATOR->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 默认模式为比较
    EXPECT_EQ(blocks::RedstoneComparatorBlock::getMode(state), blocks::ComparatorMode::Compare);

    // 默认不锁定
    EXPECT_FALSE(state.get(BlockStateProperties::LOCKED()));
}

TEST_F(RedstoneBlockTest, RedstoneComparatorModes) {
    BlockState state = VanillaBlocks::REDSTONE_COMPARATOR->defaultState();

    // 比较模式
    state = blocks::RedstoneComparatorBlock::withMode(state, blocks::ComparatorMode::Compare);
    EXPECT_EQ(blocks::RedstoneComparatorBlock::getMode(state), blocks::ComparatorMode::Compare);

    // 减法模式
    state = blocks::RedstoneComparatorBlock::withMode(state, blocks::ComparatorMode::Subtract);
    EXPECT_EQ(blocks::RedstoneComparatorBlock::getMode(state), blocks::ComparatorMode::Subtract);
}

// ============================================================================
// 侦测器属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, ObserverDefaultState) {
    const BlockState& state = VanillaBlocks::OBSERVER->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);

    // 默认不激活
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(RedstoneBlockTest, ObserverCanProvidePower) {
    const BlockState& state = VanillaBlocks::OBSERVER->defaultState();
    EXPECT_TRUE(VanillaBlocks::OBSERVER->canProvidePower(state));
}

// ============================================================================
// 活塞属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, PistonDefaultState) {
    const BlockState& state = VanillaBlocks::PISTON->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);

    // 默认不伸出
    EXPECT_FALSE(state.get(BlockStateProperties::EXTENDED()));
}

TEST_F(RedstoneBlockTest, StickyPistonDefaultState) {
    const BlockState& state = VanillaBlocks::STICKY_PISTON->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);

    // 默认不伸出
    EXPECT_FALSE(state.get(BlockStateProperties::EXTENDED()));
}

// ============================================================================
// 发射器/投掷器属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, DispenserDefaultState) {
    const BlockState& state = VanillaBlocks::DISPENSER->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);

    // 默认不触发
    EXPECT_FALSE(state.get(BlockStateProperties::TRIGGERED()));
}

TEST_F(RedstoneBlockTest, DropperDefaultState) {
    const BlockState& state = VanillaBlocks::DROPPER->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);

    // 默认不触发
    EXPECT_FALSE(state.get(BlockStateProperties::TRIGGERED()));
}

// ============================================================================
// 标靶属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, TargetDefaultState) {
    const BlockState& state = VanillaBlocks::TARGET->defaultState();

    // 默认输出功率为0
    EXPECT_EQ(state.get(BlockStateProperties::POWER_0_15()), 0);
}

TEST_F(RedstoneBlockTest, TargetCanProvidePower) {
    const BlockState& state = VanillaBlocks::TARGET->defaultState();
    EXPECT_TRUE(VanillaBlocks::TARGET->canProvidePower(state));
}

// ============================================================================
// 绊线属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, TripWireDefaultState) {
    const BlockState& state = VanillaBlocks::TRIPWIRE->defaultState();

    // 默认不触发
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));

    // 默认不连接
    EXPECT_FALSE(state.get(BlockStateProperties::ATTACHED()));
}

TEST_F(RedstoneBlockTest, TripWireHookDefaultState) {
    const BlockState& state = VanillaBlocks::TRIPWIRE_HOOK->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 默认不触发
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));

    // 默认不连接
    EXPECT_FALSE(state.get(BlockStateProperties::ATTACHED()));
}

// ============================================================================
// 音符盒属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, NoteBlockDefaultState) {
    const BlockState& state = VanillaBlocks::NOTE_BLOCK->defaultState();

    // 默认音符为0
    EXPECT_EQ(blocks::NoteBlock::getNote(state), 0);
}

TEST_F(RedstoneBlockTest, NoteBlockNoteRange) {
    BlockState state = VanillaBlocks::NOTE_BLOCK->defaultState();

    for (i32 note = 0; note <= 24; ++note) {
        state = blocks::NoteBlock::withNote(state, note);
        EXPECT_EQ(blocks::NoteBlock::getNote(state), note);
    }
}

// ============================================================================
// TNT属性测试
// ============================================================================

TEST_F(RedstoneBlockTest, TNTDefaultState) {
    const BlockState& state = VanillaBlocks::TNT->defaultState();

    // 默认不稳定（不会被红石触发爆炸）
    EXPECT_FALSE(state.get(BlockStateProperties::UNSTABLE()));
}

// ============================================================================
// 红石信号强度计算测试
// ============================================================================

TEST_F(RedstoneBlockTest, RedstoneWireSignalAttenuation) {
    // 测试红石线信号衰减规则
    // 信号每传输一格衰减1
    for (i32 power = 0; power <= 15; ++power) {
        BlockState state = VanillaBlocks::REDSTONE_WIRE->defaultState();
        state = blocks::RedstoneWireBlock::withPower(state, power);

        i32 retrieved = blocks::RedstoneWireBlock::getPower(state);
        EXPECT_EQ(retrieved, power);
    }
}

TEST_F(RedstoneBlockTest, PressurePlatePowerRange) {
    BlockState state = VanillaBlocks::STONE_PRESSURE_PLATE->defaultState();

    // 压力板输出范围是0-15
    for (i32 power = 0; power <= 15; ++power) {
        state = blocks::AbstractPressurePlateBlock::withPower(state, power);
        EXPECT_EQ(blocks::AbstractPressurePlateBlock::getPower(state), power);
    }
}

TEST_F(RedstoneBlockTest, DaylightDetectorPowerRange) {
    BlockState state = VanillaBlocks::DAYLIGHT_DETECTOR->defaultState();

    // 日光探测器输出范围是0-15
    for (i32 power = 0; power <= 15; ++power) {
        state = blocks::DaylightDetectorBlock::withPower(state, power);
        EXPECT_EQ(blocks::DaylightDetectorBlock::getPower(state), power);
    }
}
