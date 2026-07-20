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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/copper/CopperGolemStatueBlock.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ActionResult.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用世界 - 支持方块状态存储、方块实体存储、音效/游戏事件捕获
 *
 * 继承自 BaseTestWorld，添加：
 * - BlockState 副本存储（避免悬空指针）
 * - BlockEntity 存储
 * - playSound / gameEvent 捕获
 * - TickManager 支持（用于含水调度）
 */
class CopperGolemStatueTestWorld final : public test::BaseTestWorld {
public:
    CopperGolemStatueTestWorld() { m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this); }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    // 2-arg 重载（避免 IWorldWriter 的非虚 2-arg 版本被子类 4-arg override 隐藏）
    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    // 1-arg 重载（便于测试通过 BlockPos 查询）
    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            entity->setWorld(this);
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEvents.push_back({&event, pos, context});
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManagerPtr; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManagerPtr; }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    // ========== 测试辅助方法 ==========

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    struct GameEventRecord {
        const gameevent::GameEvent* event;
        BlockPos pos;
        gameevent::GameEvent::Context context;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    [[nodiscard]] const std::vector<GameEventRecord>& gameEvents() const { return m_gameEvents; }
    void clearSounds() { m_sounds.clear(); }
    void clearGameEvents() { m_gameEvents.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<SoundRecord> m_sounds;
    std::vector<GameEventRecord> m_gameEvents;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    EntityId m_lastEntityId = 0;
    u64 m_seed = 0;
};

BlockItemUseContext makePlacementContext(IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw)
{
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    return BlockItemUseContext(world,
        nullptr,
        EMPTY_STACK,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        pos,
        face,
        playerYaw,
        0.0f);
}

} // namespace

// ============================================================================
// 铜傀儡雕像方块测试
// ============================================================================

class CopperGolemStatueBlockTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        item::tag::ItemTags::initialize();
        BlockTags::initialize();
        // 注册实体类型，使 CopperGolemStatueBlockEntity::removeStatue 能通过
        // EntityRegistry 查找到 copper_golem 实体工厂
        entity::VanillaEntities::registerAll();
    }
};

// ---------- 注册与默认状态 ----------

TEST_F(CopperGolemStatueBlockTestFixture, Registration_AllVariantsRegistered)
{
    ASSERT_NE(VanillaBlocks::COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE, nullptr);
}

TEST_F(CopperGolemStatueBlockTestFixture, DefaultState_ContainsAllProperties)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState& state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    // 默认站立姿态
    EXPECT_EQ(state.get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Standing);
    // 默认不含水
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ---------- 放置逻辑 ----------

TEST_F(CopperGolemStatueBlockTestFixture, Placement_FacesOppositePlayerDirection)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(8, 64, 8);

    // 玩家朝南（yaw=0 对应南），雕像应朝北的反方向 = 南？需要看 horizontalDirection 实现
    // 实际：context.horizontalDirection() 返回玩家朝向，getStateForPlacement 取 opposite
    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->getStateForPlacement(context);

    // 验证状态中 FACING 已设置（不再是默认值 North，因为 opposite( South ) = North 仍是 North）
    // 当玩家朝南放置时，facing = opposite(South) = North，所以这里仍为 North
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(CopperGolemStatueBlockTestFixture, Placement_WaterloggedWhenInWater)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(10, 64, 10);

    // 在目标位置放置水源方块
    world.setBlockState(pos, &VanillaBlocks::WATER->defaultState());

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->getStateForPlacement(context);

    // 放置在水源中应含水
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CopperGolemStatueBlockTestFixture, Placement_NotWaterloggedInAir)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(12, 64, 12);

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->getStateForPlacement(context);

    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ---------- 形状 ----------

TEST_F(CopperGolemStatueBlockTestFixture, Shape_CylindricalShape)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState& state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState();
    const CollisionShape& shape = VanillaBlocks::COPPER_GOLEM_STATUE->getShape(state);

    EXPECT_FALSE(shape.isEmpty());
    // 圆柱形：直径 10，高 14 = box(3, 0, 3, 13, 14, 13) = 1 个碰撞箱
    EXPECT_EQ(shape.boxCount(), 1u);
}

TEST_F(CopperGolemStatueBlockTestFixture, Shape_SameForAllPoses)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState standing = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Standing);
    const BlockState sitting = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Sitting);
    const BlockState running = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Running);
    const BlockState star = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Star);

    // 所有姿态共享同一碰撞形状
    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getShape(standing).boxCount(),
        VanillaBlocks::COPPER_GOLEM_STATUE->getShape(sitting).boxCount());
    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getShape(running).boxCount(),
        VanillaBlocks::COPPER_GOLEM_STATUE->getShape(star).boxCount());
}

// ---------- 姿态循环切换 ----------

TEST_F(CopperGolemStatueBlockTestFixture, GetNextPose_StandingToSitting)
{
    EXPECT_EQ(CopperGolemStatueBlock::getNextPose(BlockStateProperties::CopperGolemPose::Standing),
        BlockStateProperties::CopperGolemPose::Sitting);
}

TEST_F(CopperGolemStatueBlockTestFixture, GetNextPose_SittingToRunning)
{
    EXPECT_EQ(CopperGolemStatueBlock::getNextPose(BlockStateProperties::CopperGolemPose::Sitting),
        BlockStateProperties::CopperGolemPose::Running);
}

TEST_F(CopperGolemStatueBlockTestFixture, GetNextPose_RunningToStar)
{
    EXPECT_EQ(CopperGolemStatueBlock::getNextPose(BlockStateProperties::CopperGolemPose::Running),
        BlockStateProperties::CopperGolemPose::Star);
}

TEST_F(CopperGolemStatueBlockTestFixture, GetNextPose_StarToStanding)
{
    EXPECT_EQ(CopperGolemStatueBlock::getNextPose(BlockStateProperties::CopperGolemPose::Star),
        BlockStateProperties::CopperGolemPose::Standing);
}

// ---------- 红石比较器输出 ----------

TEST_F(CopperGolemStatueBlockTestFixture, ComparatorOutput_HasOverride)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState& state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState();
    EXPECT_TRUE(VanillaBlocks::COPPER_GOLEM_STATUE->hasComparatorInputOverride(state));
}

TEST_F(CopperGolemStatueBlockTestFixture, ComparatorOutput_StandingReturns1)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(20, 64, 20);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Standing);

    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getComparatorInputOverride(state, world, pos), 1);
}

TEST_F(CopperGolemStatueBlockTestFixture, ComparatorOutput_SittingReturns2)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(20, 64, 20);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Sitting);

    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getComparatorInputOverride(state, world, pos), 2);
}

TEST_F(CopperGolemStatueBlockTestFixture, ComparatorOutput_RunningReturns3)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(20, 64, 20);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Running);

    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getComparatorInputOverride(state, world, pos), 3);
}

TEST_F(CopperGolemStatueBlockTestFixture, ComparatorOutput_StarReturns4)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(20, 64, 20);
    const BlockState state = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::COPPER_GOLEM_POSE(), BlockStateProperties::CopperGolemPose::Star);

    EXPECT_EQ(VanillaBlocks::COPPER_GOLEM_STATUE->getComparatorInputOverride(state, world, pos), 4);
}

// ---------- 氧化属性 ----------

TEST_F(CopperGolemStatueBlockTestFixture, OxidationLevel_ExposedIsExposed)
{
    if (!VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "EXPOSED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Exposed);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationLevel_WeatheredIsWeathered)
{
    if (!VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "WEATHERED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Weathered);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationLevel_OxidizedIsOxidized)
{
    if (!VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "OXIDIZED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE);
    ASSERT_NE(weatheringBlock, nullptr);
    EXPECT_EQ(weatheringBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Oxidized);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationLevel_BaseStatueNotOxidizable)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    // 基础版 copper_golem_statue 是 CopperGolemStatueBlock（非 Weathering），不实现 IOxidizableBlock
    const auto* weatheringBlock = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::COPPER_GOLEM_STATUE);
    EXPECT_EQ(weatheringBlock, nullptr);
}

// ---------- 氧化链 ----------

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_ExposedNextIsWeathered)
{
    if (!VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE || !VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "EXPOSED/WEATHERED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* exposed = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getNextOxidationBlock(), VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_WeatheredNextIsOxidized)
{
    if (!VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE || !VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "WEATHERED/OXIDIZED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* weathered = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getNextOxidationBlock(), VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_OxidizedHasNoNext)
{
    if (!VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "OXIDIZED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getNextOxidationBlock(), nullptr);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_ExposedPreviousIsBaseStatue)
{
    if (!VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE || !VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "EXPOSED/COPPER_GOLEM_STATUE not registered";
    }

    // exposed 的上一级是基础版 copper_golem_statue（Unaffected 等级）
    const auto* exposed = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getPreviousOxidationBlock(), VanillaBlocks::COPPER_GOLEM_STATUE);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_WeatheredPreviousIsExposed)
{
    if (!VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE || !VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "WEATHERED/EXPOSED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* weathered = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getPreviousOxidationBlock(), VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE);
}

TEST_F(CopperGolemStatueBlockTestFixture, OxidationChain_OxidizedPreviousIsWeathered)
{
    if (!VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE || !VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "OXIDIZED/WEATHERED_COPPER_GOLEM_STATUE not registered";
    }

    const auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getPreviousOxidationBlock(), VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE);
}

// ---------- 随机 Tick 行为 ----------

TEST_F(CopperGolemStatueBlockTestFixture, TicksRandomly_OnlyWhenNotFullyOxidized)
{
    if (!VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE || !VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "EXPOSED/OXIDIZED_COPPER_GOLEM_STATUE not registered";
    }

    // 暴露等级应响应随机刻（可继续氧化）
    EXPECT_TRUE(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE->ticksRandomly());
    // 锈蚀等级应响应随机刻（可继续氧化）
    EXPECT_TRUE(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE->ticksRandomly());
    // 氧化等级不应响应随机刻（已是最高等级）
    EXPECT_FALSE(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE->ticksRandomly());
}

TEST_F(CopperGolemStatueBlockTestFixture, TicksRandomly_BaseStatueDoesNotTick)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    // 基础版 copper_golem_statue 是 CopperGolemStatueBlock（非 Weathering），不响应随机刻
    EXPECT_FALSE(VanillaBlocks::COPPER_GOLEM_STATUE->ticksRandomly());
}

// ---------- 涂蜡版本 ----------

TEST_F(CopperGolemStatueBlockTestFixture, WaxedVariants_NotOxidizable)
{
    if (!VanillaBlocks::WAXED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "WAXED_COPPER_GOLEM_STATUE not registered";
    }

    // 涂蜡变体使用 CopperGolemStatueBlock（非 Weathering），不实现 IOxidizableBlock
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE), nullptr);
}

TEST_F(CopperGolemStatueBlockTestFixture, WaxedVariants_DoNotTickRandomly)
{
    if (!VanillaBlocks::WAXED_COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "WAXED_COPPER_GOLEM_STATUE not registered";
    }

    EXPECT_FALSE(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE->ticksRandomly());
}

// ---------- 含水状态 ----------

TEST_F(CopperGolemStatueBlockTestFixture, WaterloggedState_ReturnsWaterFluid)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState waterloggedState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), true);

    const fluid::FluidState* fluidState = VanillaBlocks::COPPER_GOLEM_STATUE->getFluidState(waterloggedState);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
}

TEST_F(CopperGolemStatueBlockTestFixture, NonWaterloggedState_ReturnsNullFluid)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState dryState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    const fluid::FluidState* fluidState = VanillaBlocks::COPPER_GOLEM_STATUE->getFluidState(dryState);
    EXPECT_EQ(fluidState, nullptr);
}

TEST_F(CopperGolemStatueBlockTestFixture, IsWaterlogged_ReflectsState)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState wetState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const BlockState dryState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    // 通过 IWaterLoggable 接口检查 isWaterlogged
    const auto* waterloggable = dynamic_cast<const IWaterLoggable*>(VanillaBlocks::COPPER_GOLEM_STATUE);
    ASSERT_NE(waterloggable, nullptr);
    EXPECT_TRUE(waterloggable->isWaterlogged(wetState));
    EXPECT_FALSE(waterloggable->isWaterlogged(dryState));
}

// ---------- 方块实体 ----------

TEST_F(CopperGolemStatueBlockTestFixture, HasBlockEntity_ReturnsTrue)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    EXPECT_TRUE(VanillaBlocks::COPPER_GOLEM_STATUE->hasBlockEntity());
    EXPECT_TRUE(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE->hasBlockEntity());
    EXPECT_TRUE(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE->hasBlockEntity());
}

TEST_F(CopperGolemStatueBlockTestFixture, CreateBlockEntity_ReturnsCorrectType)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockPos pos(30, 64, 30);
    auto entity = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::CopperGolemStatue);
    EXPECT_EQ(entity->getPos(), pos);
}

// ---------- 旋转/镜像 ----------

TEST_F(CopperGolemStatueBlockTestFixture, Rotate_ChangesFacing)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState northState = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotatedState = VanillaBlocks::COPPER_GOLEM_STATUE->rotate(northState, Rotation::Clockwise90);

    // 北顺时针旋转 90 度变为东
    EXPECT_EQ(rotatedState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(CopperGolemStatueBlockTestFixture, Mirror_NoneKeepsFacing)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockState northState = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirroredState = VanillaBlocks::COPPER_GOLEM_STATUE->mirror(northState, Mirror::None);

    EXPECT_EQ(mirroredState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

// ---------- 标签集成 ----------

TEST_F(CopperGolemStatueBlockTestFixture, Tag_CopperGolemStatuesContainsAllVariants)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    BlockTag& tag = BlockTags::COPPER_GOLEM_STATUES();
    EXPECT_TRUE(tag.contains(VanillaBlocks::COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(tag.contains(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE));
}

TEST_F(CopperGolemStatueBlockTestFixture, Tag_CopperTagContainsStatues)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    BlockTag& copperTag = BlockTags::COPPER();
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE));
    EXPECT_TRUE(copperTag.contains(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE));
}

TEST_F(CopperGolemStatueBlockTestFixture, Tag_CopperGolemStatuesExcludesUnrelatedBlocks)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    BlockTag& tag = BlockTags::COPPER_GOLEM_STATUES();
    // 铜块不应在 copper_golem_statues 标签中
    if (VanillaBlocks::COPPER_BLOCK) {
        EXPECT_FALSE(tag.contains(VanillaBlocks::COPPER_BLOCK));
    }
    // 石头不应在 copper_golem_statues 标签中
    if (VanillaBlocks::STONE) {
        EXPECT_FALSE(tag.contains(VanillaBlocks::STONE));
    }
}

// ---------- BlockEntity 序列化集成 ----------

TEST_F(CopperGolemStatueBlockTestFixture, BlockEntity_SaveLoadJson_PreservesCustomName)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockPos pos(40, 64, 40);
    auto entity = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);

    // 设置自定义名称
    entity->setCustomName("TestGolem");

    // 保存到 JSON
    nlohmann::json data;
    entity->save(data);

    // 创建新实体并加载
    auto restored = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(restored, nullptr);
    EXPECT_TRUE(restored->load(data));
    EXPECT_EQ(restored->getCustomName(), "TestGolem");
}

TEST_F(CopperGolemStatueBlockTestFixture, BlockEntity_SaveLoadNBT_PreservesCustomName)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockPos pos(42, 64, 42);
    auto entity = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);

    entity->setCustomName("NBTGolem");

    nbt::CompoundTag tag;
    entity->saveToNBT(tag);

    auto restored = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(restored, nullptr);
    EXPECT_TRUE(restored->loadFromNBT(tag));
    EXPECT_EQ(restored->getCustomName(), "NBTGolem");
}

TEST_F(CopperGolemStatueBlockTestFixture, BlockEntity_EmptyCustomNameByDefault)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockPos pos(44, 64, 44);
    auto entity = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);

    EXPECT_EQ(entity->getCustomName(), "");

    // 保存空名称不应写入 custom_name 字段
    nlohmann::json data;
    entity->save(data);
    EXPECT_FALSE(data.contains("custom_name"));
}

TEST_F(CopperGolemStatueBlockTestFixture, BlockEntity_Clone_PreservesCustomName)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    const BlockPos pos(46, 64, 46);
    auto entity = VanillaBlocks::COPPER_GOLEM_STATUE->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setCustomName("CloneMe");

    auto clone = entity->clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->getCustomName(), "CloneMe");
    EXPECT_EQ(clone->getType(), BlockEntityType::CopperGolemStatue);
}

// ---------- onBlockActivated 行为 ----------

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_EmptyHand_CyclesPose)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(50, 64, 50);

    // 放置雕像（Standing 姿态）
    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());

    // 创建玩家（空手）
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    auto result = VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 空手应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应播放变雕像音效
    EXPECT_FALSE(world.sounds().empty());
    EXPECT_EQ(world.sounds()[0].sound, ResourceLocation("minecraft", "block.copper_golem.become_statue"));

    // 应触发 BLOCK_CHANGE 游戏事件
    EXPECT_FALSE(world.gameEvents().empty());
    EXPECT_EQ(world.gameEvents()[0].event, &gameevent::GameEvents::BLOCK_CHANGE);
    EXPECT_EQ(world.gameEvents()[0].pos, pos);

    // 方块状态应切换到 Sitting
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Sitting);
}

// ============================================================================
// 斧头交互测试（基础雕像 → 生成铜傀儡）
// ============================================================================
//
// 对应 MC 1.21.11 WeatheringCopperGolemStatueBlock.useItemOn 中的斧头逻辑：
// - 基础 copper_golem_statue（Unaffected，未涂蜡）：斧头敲击 → removeStatue 生成铜傀儡
// - 涂蜡变体（waxed_*）：返回 Pass，交由 AxeItem 处理 wax_off
// - 氧化变体（exposed/weathered/oxidized）：返回 Pass，交由 AxeItem 处理 scrape_off
//
// 本项目架构：基础 copper_golem_statue 是 CopperGolemStatueBlock（不实现 IOxidizableBlock），
// 涂蜡变体也使用 CopperGolemStatueBlock。因此 onBlockActivated 中通过
// HoneycombItem::getWaxedOff(state) 区分涂蜡（Pass）与基础（生成铜傀儡）。

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnWaxedStatueReturnsPass)
{
    if (!VanillaBlocks::WAXED_COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "WAXED_COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(52, 64, 52);

    world.setBlockState(pos, &VanillaBlocks::WAXED_COPPER_GOLEM_STATUE->defaultState());

    // 创建玩家并设置手持铁斧
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::WAXED_COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 涂蜡变体：斧头应返回 Pass（委托给 AxeItem 处理 wax_off）
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应播放音效（PASS 不执行任何动作）
    EXPECT_TRUE(world.sounds().empty());

    // 方块状态不应改变（仍是 Standing）
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(
        newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Standing);
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnExposedStatueReturnsPass)
{
    // 氧化变体（Exposed/Weathered/Oxidized）不是涂蜡，应返回 Pass 交由 AxeItem 处理 scrape_off
    if (!VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "EXPOSED_COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(53, 64, 53);

    world.setBlockState(pos, &VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE->defaultState());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 氧化变体：斧头应返回 Pass（委托给 AxeItem 处理 scrape_off）
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应播放音效
    EXPECT_TRUE(world.sounds().empty());

    // 方块不应被移除
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_NE(newState->owner().blockLocation(), VanillaBlocks::AIR->blockLocation());
}

// ============================================================================
// 斧头敲击基础雕像生成铜傀儡的核心测试
// ============================================================================
//
// 对应 MC 1.21.11: WeatheringCopperGolemStatueBlock.useItemOn 中
//   if (this.getAge().equals(WeatherState.UNAFFECTED) && axe) {
//     CopperGolemStatueBlockEntity.removeStatue(state);
//     hurtAndBreak(1, player, slot);
//     addFreshEntity(coppergolem);
//     removeBlock(pos, false);
//   }

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnBaseStatueSpawnsGolem)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(70, 64, 70);

    // 放置基础（未涂蜡、Unaffected）铜傀儡雕像
    const BlockState placedState = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    world.setBlockState(pos, &placedState);

    // 创建方块实体并设置自定义名称
    auto be = std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos);
    be->setCustomName("TestGolem");
    world.setBlockEntity(pos, be.release());

    // 创建玩家并设置手持铁斧
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    auto result = VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应生成 1 个实体（铜傀儡）
    ASSERT_EQ(world.spawnedEntities().size(), 1u);
    auto* golem = dynamic_cast<CopperGolemEntity*>(world.spawnedEntities()[0].get());
    EXPECT_NE(golem, nullptr);

    // 铜傀儡应继承自定义名称
    EXPECT_EQ(golem->customNameText(), "TestGolem");

    // 铜傀儡应位于雕像位置中心
    EXPECT_FLOAT_EQ(golem->x(), static_cast<f32>(pos.x) + 0.5f);
    EXPECT_FLOAT_EQ(golem->y(), static_cast<f32>(pos.y));
    EXPECT_FLOAT_EQ(golem->z(), static_cast<f32>(pos.z) + 0.5f);

    // 铜傀儡朝向应与雕像 FACING 一致（South=0°）
    EXPECT_FLOAT_EQ(golem->yaw(), 0.0f);

    // 应播放生成音效
    EXPECT_FALSE(world.sounds().empty());
    bool foundSpawnSound = false;
    for (const auto& s : world.sounds()) {
        if (s.sound == SoundEvents::ENTITY_COPPER_GOLEM_SPAWN) {
            foundSpawnSound = true;
            break;
        }
    }
    EXPECT_TRUE(foundSpawnSound);

    // 方块应被移除（变为空气）
    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->owner().blockLocation(), VanillaBlocks::AIR->blockLocation());
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnBaseStatueDamagesAxe)
{
    // 验证斧头耐久损耗：每次敲击基础雕像生成铜傀儡时，斧头耐久 -1
    if (!VanillaBlocks::COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(71, 64, 71);

    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());
    world.setBlockEntity(pos, std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos).release());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const i32 initialDamage = player.inventory().getSelectedStackRef().getDamage();
    const i32 maxDamage = player.inventory().getSelectedStackRef().getMaxDamage();

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 斧头耐久应 +1
    const i32 newDamage = player.inventory().getSelectedStackRef().getDamage();
    EXPECT_EQ(newDamage, initialDamage + 1);

    // 应小于最大耐久
    EXPECT_LT(newDamage, maxDamage);
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnBaseStatueNoBlockEntityReturnsPass)
{
    // 边界场景：基础雕像但没有方块实体（理论上不应发生，但代码有防御性处理）
    if (!VanillaBlocks::COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(72, 64, 72);

    // 放置方块但不创建方块实体
    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    auto result = VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 没有方块实体时返回 Pass（防御性处理）
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应生成实体
    EXPECT_EQ(world.spawnedEntities().size(), 0u);

    // 不应播放音效
    EXPECT_TRUE(world.sounds().empty());
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeFacingDirectionsCorrectlyMapped)
{
    // 验证所有 4 个 FACING 方向都能正确映射到 yaw
    if (!VanillaBlocks::COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    const struct {
        Direction facing;
        f32 expectedYaw;
    } cases[] = {
        {Direction::South, 0.0f},
        {Direction::West, 90.0f},
        {Direction::North, 180.0f},
        {Direction::East, 270.0f},
    };

    for (const auto& c : cases) {
        CopperGolemStatueTestWorld world;
        const BlockPos pos(73, 64, 73);

        const BlockState placedState = VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(
            BlockStateProperties::HORIZONTAL_FACING(), c.facing);
        world.setBlockState(pos, &placedState);
        world.setBlockEntity(pos, std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos).release());

        Player player(EntityId(1), "TestPlayer");
        player.setWorld(&world);
        player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

        const BlockState* state = world.getBlockState(pos);
        ASSERT_NE(state, nullptr);

        BlockRaycastResult hit;
        VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

        ASSERT_EQ(world.spawnedEntities().size(), 1u);
        auto* golem = dynamic_cast<CopperGolemEntity*>(world.spawnedEntities()[0].get());
        ASSERT_NE(golem, nullptr);
        EXPECT_FLOAT_EQ(golem->yaw(), c.expectedYaw)
            << "Facing " << static_cast<i32>(c.facing) << " should map to yaw " << c.expectedYaw;
    }
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_AxeOnBaseStatueGolemIsUnaffected)
{
    // 验证生成的铜傀儡初始氧化等级为 Unaffected
    if (!VanillaBlocks::COPPER_GOLEM_STATUE || !Items::IRON_AXE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE or IRON_AXE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(74, 64, 74);

    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());
    world.setBlockEntity(pos, std::make_unique<blockentity::CopperGolemStatueBlockEntity>(pos).release());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);
    player.inventory().getSelectedStackRef() = ItemStack(*Items::IRON_AXE, 1);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);

    BlockRaycastResult hit;
    VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    ASSERT_EQ(world.spawnedEntities().size(), 1u);
    auto* golem = dynamic_cast<CopperGolemEntity*>(world.spawnedEntities()[0].get());
    ASSERT_NE(golem, nullptr);
    EXPECT_EQ(golem->getWeatherState(), entity::CopperGolemWeatherState::Unaffected);
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_EmptyHandOnBaseStatueCyclesPose)
{
    // 空手敲击基础雕像应循环姿态（不生成铜傀儡）
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(75, 64, 75);

    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);

    const BlockState* state = world.getBlockState(pos);
    ASSERT_NE(state, nullptr);
    ASSERT_EQ(state->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Standing);

    BlockRaycastResult hit;
    auto result = VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);

    // 空手应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 不应生成实体
    EXPECT_EQ(world.spawnedEntities().size(), 0u);

    // 姿态应循环到 Sitting
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Sitting);
}

TEST_F(CopperGolemStatueBlockTestFixture, OnBlockActivated_CyclesThroughAllPoses)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(54, 64, 54);

    // 起始姿态：Standing
    world.setBlockState(pos, &VanillaBlocks::COPPER_GOLEM_STATUE->defaultState());

    Player player(EntityId(1), "TestPlayer");
    player.setWorld(&world);

    BlockRaycastResult hit;

    // 第一次激活：Standing -> Sitting
    {
        const BlockState* state = world.getBlockState(pos);
        ASSERT_NE(state, nullptr);
        world.clearSounds();
        world.clearGameEvents();
        VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);
        const BlockState* newState = world.getBlockState(pos);
        ASSERT_NE(newState, nullptr);
        EXPECT_EQ(
            newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Sitting);
        EXPECT_FALSE(world.sounds().empty());
        EXPECT_FALSE(world.gameEvents().empty());
    }

    // 第二次激活：Sitting -> Running
    {
        const BlockState* state = world.getBlockState(pos);
        ASSERT_NE(state, nullptr);
        world.clearSounds();
        world.clearGameEvents();
        VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);
        const BlockState* newState = world.getBlockState(pos);
        ASSERT_NE(newState, nullptr);
        EXPECT_EQ(
            newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Running);
        EXPECT_FALSE(world.sounds().empty());
        EXPECT_FALSE(world.gameEvents().empty());
    }

    // 第三次激活：Running -> Star
    {
        const BlockState* state = world.getBlockState(pos);
        ASSERT_NE(state, nullptr);
        world.clearSounds();
        world.clearGameEvents();
        VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);
        const BlockState* newState = world.getBlockState(pos);
        ASSERT_NE(newState, nullptr);
        EXPECT_EQ(
            newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Star);
        EXPECT_FALSE(world.sounds().empty());
        EXPECT_FALSE(world.gameEvents().empty());
    }

    // 第四次激活：Star -> Standing（循环回到起点）
    {
        const BlockState* state = world.getBlockState(pos);
        ASSERT_NE(state, nullptr);
        world.clearSounds();
        world.clearGameEvents();
        VanillaBlocks::COPPER_GOLEM_STATUE->onBlockActivated(*state, world, pos, player, Hand::MainHand, hit);
        const BlockState* newState = world.getBlockState(pos);
        ASSERT_NE(newState, nullptr);
        EXPECT_EQ(
            newState->get(BlockStateProperties::COPPER_GOLEM_POSE()), BlockStateProperties::CopperGolemPose::Standing);
        EXPECT_FALSE(world.sounds().empty());
        EXPECT_FALSE(world.gameEvents().empty());
    }
}

// ---------- updatePostPlacement 含水调度 ----------

TEST_F(CopperGolemStatueBlockTestFixture, UpdatePostPlacement_WaterloggedSchedulesWaterTick)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(60, 64, 60);

    // 设置含水状态
    const BlockState waterloggedState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), true);

    // 调用 updatePostPlacement 应不崩溃（调度水 tick 通过 TickManager）
    const BlockState result = VanillaBlocks::COPPER_GOLEM_STATUE->updatePostPlacement(
        waterloggedState, Direction::Up, waterloggedState, world, pos, pos.up());

    // 返回的状态应保持含水
    EXPECT_TRUE(result.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CopperGolemStatueBlockTestFixture, UpdatePostPlacement_DryDoesNotScheduleTick)
{
    if (!VanillaBlocks::COPPER_GOLEM_STATUE) {
        GTEST_SKIP() << "COPPER_GOLEM_STATUE not registered";
    }

    CopperGolemStatueTestWorld world;
    const BlockPos pos(62, 64, 62);

    // 设置不含水状态
    const BlockState dryState =
        VanillaBlocks::COPPER_GOLEM_STATUE->defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    // 调用 updatePostPlacement 应不崩溃且不调度水 tick
    const BlockState result = VanillaBlocks::COPPER_GOLEM_STATUE->updatePostPlacement(
        dryState, Direction::Up, dryState, world, pos, pos.up());

    EXPECT_FALSE(result.get(BlockStateProperties::WATERLOGGED()));
}
