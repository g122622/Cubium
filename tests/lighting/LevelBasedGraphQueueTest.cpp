#include <gtest/gtest.h>

#include "LightingTestHelpers.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"

using namespace mc;
using namespace lighting_test;

namespace {

class LevelBasedGraphQueueTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ensureVanillaBlocksInitialized();
    }
};

} // namespace

TEST_F(LevelBasedGraphQueueTest, NullToUninitialisedTransitionWorks)
{
    SWMRNibbleArray array = SWMRNibbleArray::nullNibble();

    EXPECT_TRUE(array.isNullNibbleUpdating());
    EXPECT_TRUE(array.isNullNibbleVisible());

    array.setNonNull();

    EXPECT_TRUE(array.isUninitialisedUpdating());
    EXPECT_TRUE(array.isNullNibbleVisible());
    EXPECT_TRUE(array.updateVisible());
    EXPECT_TRUE(array.isUninitialisedVisible());

    const auto vanilla = array.toVanillaNibble();
    ASSERT_TRUE(vanilla.has_value());
    EXPECT_TRUE(vanilla->isEmpty());
}

TEST_F(LevelBasedGraphQueueTest, ZeroDataSaveStateFallsBackToUninitialised)
{
    SWMRNibbleArray array;

    array.setZero();
    EXPECT_TRUE(array.updateVisible());

    const auto saveState = array.getSaveState();
    ASSERT_TRUE(saveState.has_value());
    EXPECT_EQ(saveState->state, SWMRNibbleArray::INIT_STATE_UNINIT);
    EXPECT_TRUE(saveState->data.empty());
}

TEST_F(LevelBasedGraphQueueTest, HiddenStateRemainsHiddenAfterPublish)
{
    SWMRNibbleArray array;

    array.setFull();
    EXPECT_TRUE(array.updateVisible());
    EXPECT_TRUE(array.isInitialisedVisible());

    array.setHidden();
    EXPECT_TRUE(array.isHiddenUpdating());
    EXPECT_TRUE(array.updateVisible());
    EXPECT_TRUE(array.isHiddenVisible());
    EXPECT_FALSE(array.toVanillaNibble().has_value());
}
