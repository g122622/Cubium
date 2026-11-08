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

#include "common/world/block/blocks/door/DoorBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/block/blocks/door/WeatheringCopperDoorBlock.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "util/property/Properties.hpp"
#include <optional>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== DoorBlock 测试 ==========

class DoorBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建木门方块
        door_ = std::make_unique<DoorBlock>(
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f), BlockSetTypes::OAK);
    }

    std::unique_ptr<DoorBlock> door_;
};

TEST_F(DoorBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(door_, nullptr);
}

TEST_F(DoorBlockTest, IsOpen_UsesStaticMethod)
{
    const auto& state = door_->defaultState();
    // 默认关闭
    EXPECT_FALSE(DoorBlock::isOpen(state));
}

TEST_F(DoorBlockTest, IsIronDoor_ReturnsFalse)
{
    EXPECT_FALSE(door_->isIronDoor());
}

TEST_F(DoorBlockTest, GetPushReaction_ReturnsDestroy)
{
    const auto& state = door_->defaultState();
    EXPECT_EQ(door_->getPushReaction(state), Material::PushReaction::Destroy);
}

TEST_F(DoorBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = door_->defaultState();
    const auto& shape = door_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(DoorBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = door_->defaultState();
    const auto& shape = door_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

namespace {
class _DoorInteractionWorld final : public test::BaseTestWorld {
public:
    std::optional<BlockState> lower;
    std::optional<BlockState> upper;
    std::string lastSound;
    f32 lastPitch = 0.0f;

    const BlockState* getBlockState(i32, i32 y, i32) const override
    {
        return y == 64 ? (lower ? &*lower : nullptr) : (upper ? &*upper : nullptr);
    }

    bool setBlockState(i32, i32 y, i32, const BlockState* state) override
    {
        (y == 64 ? lower : upper) = *state;
        return true;
    }

    void playSound(const ResourceLocation& sound, sound::SoundCategory, const Vector3&, f32, f32 pitch) override
    {
        lastSound = sound.toString();
        lastPitch = pitch;
    }
};
} // namespace

TEST(DoorInteractionTest, CopperOpensByHandAndUsesCopperSound)
{
    _DoorInteractionWorld world;
    WeatheringCopperDoorBlock door(BlockProperties(Material::IRON), BlockStateProperties::OxidationLevel::Weathered);
    world.lower = door.defaultState();
    world.upper = door.defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    Player player(1, "tester", *world.entityRegistry());
    const auto result = door.useWithoutItem(*world.lower, world, BlockPos(0, 64, 0), player, BlockRaycastResult{});
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world.lower->get(BlockStateProperties::OPEN()));
    EXPECT_TRUE(world.upper->get(BlockStateProperties::OPEN()));
    EXPECT_EQ(world.lastSound, "minecraft:block.copper_door.open");
    EXPECT_GE(world.lastPitch, 0.9f);
    EXPECT_LE(world.lastPitch, 1.0f);
}

TEST(DoorInteractionTest, IronRejectsHandInteraction)
{
    _DoorInteractionWorld world;
    DoorBlock door(BlockProperties(Material::IRON), BlockSetTypes::IRON);
    world.lower = door.defaultState();
    Player player(1, "tester", *world.entityRegistry());
    EXPECT_EQ(door.useWithoutItem(*world.lower, world, BlockPos(0, 64, 0), player, BlockRaycastResult{}),
        ActionResultType::Pass);
    EXPECT_FALSE(world.lower->get(BlockStateProperties::OPEN()));
    EXPECT_TRUE(world.lastSound.empty());
}
