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
#include "common/entity/core/Entity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/cave/BigDripleafBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using namespace mc::test;

namespace {

// ============================================================================
// 测试用常量红石方块 - 输出强度15的信号源
// ============================================================================
class ConstantPowerBlock final : public Block {
public:
    explicit ConstantPowerBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool canProvidePower(const BlockState&) const noexcept override { return true; }

    [[nodiscard]] i32 getStrongPower(const BlockState&, IWorld&, const BlockPos&, Direction) const noexcept override
    {
        return 15;
    }

    [[nodiscard]] i32 getWeakPower(const BlockState&, IWorld&, const BlockPos&, Direction) const noexcept override
    {
        return 15;
    }
};

// ============================================================================
// 测试用无信号方块 - 不输出任何红石信号
// ============================================================================
class NoPowerBlock final : public Block {
public:
    explicit NoPowerBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }
};

// ============================================================================
// 测试用实体 - 提供 onGround() 和 position()
// ============================================================================
class TestEntity final : public Entity {
public:
    explicit TestEntity(IWorld& world, ecs::EntityRegistry& registry)
        : Entity(EntityInstanceId(1), &world, registry)
    {}

    // 测试辅助：设置位置
    void setTestPosition(const Vector3& pos) { setPosition(pos); }

    // 测试辅助：设置是否在地面（公开父类protected方法）
    using Entity::setOnGround;
};

// ============================================================================
// 大滴叶测试世界
// ============================================================================
class BigDripleafTestWorld final : public BaseTestWorld {
public:
    BigDripleafTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    // 存储 BlockState 的副本并返回指针
    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
        } else {
            m_storedStates.push_back(std::make_unique<BlockState>(*state));
            m_blocks[packPos(x, y, z)] = m_storedStates.back().get();
        }
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    // 存储 BlockState 并设置
    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos, stored);
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
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<BigDripleafTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    void setSeed(u64 seed) { m_seed = seed; }

    // 音效追踪
    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_playSoundCalls.push_back({soundEventId, category, position, volume, pitch});
    }

    [[nodiscard]] size_t playSoundCount() const { return m_playSoundCalls.size(); }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_playSoundCalls.back().soundId; }

    // 游戏事件追踪
    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEventCalls.push_back({event.id(), pos});
        MC_UNUSED(context);
    }

    [[nodiscard]] size_t gameEventCount() const { return m_gameEventCalls.size(); }
    [[nodiscard]] const std::string& lastGameEventId() const { return m_gameEventCalls.back().eventId; }

    void clearTrackedCalls()
    {
        m_playSoundCalls.clear();
        m_gameEventCalls.clear();
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    struct SoundCall {
        ResourceLocation soundId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    struct GameEventCall {
        std::string eventId;
        BlockPos pos;
    };

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 12345;

    std::vector<SoundCall> m_playSoundCalls;
    std::vector<GameEventCall> m_gameEventCalls;
};

} // namespace

// ============================================================================
// 基本属性测试
// ============================================================================

class BigDripleafBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<BigDripleafBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(0.1f));
    }

    std::unique_ptr<BigDripleafBlock> block_;
};

TEST_F(BigDripleafBlockTest, Create_HasCorrectDefaultTilt)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

TEST_F(BigDripleafBlockTest, Create_HasCorrectDefaultFacing)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BigDripleafBlockTest, Create_NotWaterloggedByDefault)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(BigDripleafBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    const BlockState& state = block_->defaultState();
    EXPECT_TRUE(block_->useShapeForLightOcclusion(state));
}

TEST_F(BigDripleafBlockTest, CollisionShape_NoneState_IsFullBlock)
{
    const BlockState& state = block_->defaultState();
    // NONE状态的碰撞箱应为完整方块
    const CollisionShape& shape = block_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(BigDripleafBlockTest, CollisionShape_FullState_IsEmpty)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Full);
    // FULL状态碰撞箱为空（实体掉落）
    const CollisionShape& shape = block_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(BigDripleafBlockTest, TicksRandomly_False)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(block_->ticksRandomly());
}

// ============================================================================
// 红石信号与倾斜状态集成测试
// ============================================================================

class BigDripleafRedstoneTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<BigDripleafBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(0.1f));
        powerBlock_ =
            std::make_unique<ConstantPowerBlock>(BlockProperties(Material::ROCK).hardness(1.0f).resistance(1.0f));
        noPowerBlock_ = std::make_unique<NoPowerBlock>(BlockProperties(Material::ROCK).hardness(1.0f).resistance(1.0f));
        world_.ensureTickManager();
    }

    void setupDripleafAt(const BlockPos& pos)
    {
        // 放置下方支撑方块
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
        // 放置大滴叶
        world_.setBlockStateCopy(pos, block_->defaultState());
    }

    BlockState getTiltState(const BlockPos& pos) const
    {
        const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
        if (state == nullptr) {
            return block_->defaultState();
        }
        return *state;
    }

    std::unique_ptr<BigDripleafBlock> block_;
    std::unique_ptr<ConstantPowerBlock> powerBlock_;
    std::unique_ptr<NoPowerBlock> noPowerBlock_;
    BigDripleafTestWorld world_;
};

// neighborChanged：红石信号激活时重置倾斜为NONE
TEST_F(BigDripleafRedstoneTest, NeighborChanged_WithRedstoneSignal_ResetsTiltToNone)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 手动将倾斜设为PARTIAL
    BlockState partialState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Partial);
    world_.setBlockStateCopy(pos, partialState);
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Partial);

    // 在相邻位置放置红石信号源
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock_->defaultState());

    // 触发neighborChanged
    BlockState currentMutable = getTiltState(pos);
    block_->neighborChanged(world_, pos, *powerBlock_, powerPos, false);

    // 倾斜应被重置为NONE
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// neighborChanged：无红石信号时不改变倾斜状态
TEST_F(BigDripleafRedstoneTest, NeighborChanged_NoRedstoneSignal_DoesNotChangeTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 手动将倾斜设为PARTIAL
    BlockState partialState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Partial);
    world_.setBlockStateCopy(pos, partialState);
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Partial);

    // 在相邻位置放置无信号方块
    BlockPos noPowerPos(1, 65, 0);
    world_.setBlockState(noPowerPos, &noPowerBlock_->defaultState());

    // 触发neighborChanged
    block_->neighborChanged(world_, pos, *noPowerBlock_, noPowerPos, false);

    // 倾斜应保持PARTIAL
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Partial);
}

// neighborChanged：已经为NONE时红石信号不会产生音效
TEST_F(BigDripleafRedstoneTest, NeighborChanged_AlreadyNone_NoSound)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 默认状态就是NONE
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);

    // 放置红石信号源并触发neighborChanged
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock_->defaultState());
    block_->neighborChanged(world_, pos, *powerBlock_, powerPos, false);

    // 之前就是NONE，不应播放音效
    EXPECT_EQ(world_.playSoundCount(), 0u);
}

// tick：红石信号激活时重置倾斜为NONE
TEST_F(BigDripleafRedstoneTest, Tick_WithRedstoneSignal_ResetsTiltToNone)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 手动将倾斜设为UNSTABLE
    BlockState unstableState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
    world_.setBlockStateCopy(pos, unstableState);

    // 放置红石信号源
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock_->defaultState());

    // 触发tick
    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // 红石信号激活时应重置为NONE
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// tick：无红石信号时正常推进倾斜状态机
TEST_F(BigDripleafRedstoneTest, Tick_NoRedstoneSignal_AdvancesTiltState)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 手动将倾斜设为UNSTABLE
    BlockState unstableState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
    world_.setBlockStateCopy(pos, unstableState);

    // 不放置红石信号源
    // 触发tick
    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // 无红石信号时应推进为PARTIAL
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Partial);
}

// tick：UNSTABLE → PARTIAL 应播放音效
TEST_F(BigDripleafRedstoneTest, Tick_UnstableToPartial_PlaysTiltDownSound)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    BlockState unstableState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
    world_.setBlockStateCopy(pos, unstableState);

    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // 应播放TILT_DOWN音效
    EXPECT_GE(world_.playSoundCount(), 1u);
    EXPECT_EQ(world_.lastSoundId(), SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_DOWN);
}

// tick：FULL → NONE 自动重置应播放TILT_UP音效
TEST_F(BigDripleafRedstoneTest, Tick_FullToNone_PlaysTiltUpSound)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    BlockState fullState = block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Full);
    world_.setBlockStateCopy(pos, fullState);

    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // 应播放TILT_UP音效
    EXPECT_GE(world_.playSoundCount(), 1u);
    EXPECT_EQ(world_.lastSoundId(), SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_UP);
}

// tick：红石重置时播放TILT_UP音效（因为之前不是NONE）
TEST_F(BigDripleafRedstoneTest, Tick_RedstoneResetsFromPartial_PlaysTiltUpSound)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    BlockState partialState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Partial);
    world_.setBlockStateCopy(pos, partialState);

    // 放置红石信号源
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock_->defaultState());

    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // 红石重置应播放TILT_UP音效
    EXPECT_GE(world_.playSoundCount(), 1u);
    EXPECT_EQ(world_.lastSoundId(), SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_UP);
}

// tick：FULL倾斜触发BLOCK_CHANGE游戏事件
TEST_F(BigDripleafRedstoneTest, Tick_PartialToFull_TriggersBlockChangeEvent)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    BlockState partialState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Partial);
    world_.setBlockStateCopy(pos, partialState);

    BlockState mutableState = getTiltState(pos);
    math::Random& rng = world_.getRandom();
    block_->tick(world_, pos, mutableState, rng);

    // FULL倾斜应触发BLOCK_CHANGE游戏事件
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Full);
    EXPECT_GE(world_.gameEventCount(), 1u);
    EXPECT_EQ(world_.lastGameEventId(), gameevent::GameEvents::BLOCK_CHANGE.id());
}

// onEntityCollision：红石信号阻止实体触发倾斜
TEST_F(BigDripleafRedstoneTest, OnEntityCollision_RedstoneSignalPreventsTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 放置红石信号源
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock_->defaultState());

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(true);
    entity.setTestPosition(Vector3(0.5f, 65.6876f, 0.5f)); // 在方块上方0.6875以上

    block_->onEntityCollision(state, world_, pos, entity);

    // 红石信号激活时，不应触发倾斜
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// onEntityCollision：无红石信号时实体可触发倾斜
TEST_F(BigDripleafRedstoneTest, OnEntityCollision_NoRedstoneSignal_TriggersTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(true);
    entity.setTestPosition(Vector3(0.5f, 65.6876f, 0.5f));

    block_->onEntityCollision(state, world_, pos, entity);

    // 无红石信号时，应触发倾斜为UNSTABLE
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Unstable);
}

// ============================================================================
// canEntityTilt 实体检测测试
// ============================================================================

class BigDripleafCanEntityTiltTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<BigDripleafBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(0.1f));
        world_.ensureTickManager();
    }

    void setupDripleafAt(const BlockPos& pos)
    {
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
        world_.setBlockStateCopy(pos, block_->defaultState());
    }

    BlockState getTiltState(const BlockPos& pos) const
    {
        const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
        return state != nullptr ? *state : block_->defaultState();
    }

    std::unique_ptr<BigDripleafBlock> block_;
    BigDripleafTestWorld world_;
};

// 实体在地面上且Y > 方块Y + 0.6875 可触发倾斜
TEST_F(BigDripleafCanEntityTiltTest, OnGroundAboveThreshold_TriggersTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(true);
    entity.setTestPosition(Vector3(0.5f, 65.6876f, 0.5f)); // 刚好超过0.6875

    block_->onEntityCollision(state, world_, pos, entity);

    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Unstable);
}

// 实体不在地面上不能触发倾斜
TEST_F(BigDripleafCanEntityTiltTest, NotOnGround_DoesNotTriggerTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(false); // 不在地面
    entity.setTestPosition(Vector3(0.5f, 65.6876f, 0.5f));

    block_->onEntityCollision(state, world_, pos, entity);

    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// 实体在地面上但Y位置低于阈值不能触发倾斜
TEST_F(BigDripleafCanEntityTiltTest, OnGroundBelowThreshold_DoesNotTriggerTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(true);
    entity.setTestPosition(Vector3(0.5f, 65.5f, 0.5f)); // Y = 65.5 < 65.6875

    block_->onEntityCollision(state, world_, pos, entity);

    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// 实体碰撞已经处于非NONE状态时不触发
TEST_F(BigDripleafCanEntityTiltTest, AlreadyUnstable_DoesNotRetriggerTilt)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 先设置为UNSTABLE
    BlockState unstableState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
    world_.setBlockStateCopy(pos, unstableState);

    const BlockState& state = getTiltState(pos);
    TestEntity entity(world_, mc::test::testEcsRegistry());
    entity.setOnGround(true);
    entity.setTestPosition(Vector3(0.5f, 65.6876f, 0.5f));

    block_->onEntityCollision(state, world_, pos, entity);

    // 仍为UNSTABLE，不应改变
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Unstable);
}

// ============================================================================
// 倾斜状态机完整测试
// ============================================================================

class BigDripleafTiltStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<BigDripleafBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(1.0f));
        world_.ensureTickManager();
    }

    void setupDripleafAt(const BlockPos& pos)
    {
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
        world_.setBlockStateCopy(pos, block_->defaultState());
    }

    BlockState getTiltState(const BlockPos& pos) const
    {
        const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
        return state != nullptr ? *state : block_->defaultState();
    }

    std::unique_ptr<BigDripleafBlock> block_;
    BigDripleafTestWorld world_;
};

// tick推进完整状态机：UNSTABLE → PARTIAL → FULL → NONE
TEST_F(BigDripleafTiltStateMachineTest, Tick_AdvancesThroughFullCycle)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);
    math::Random& rng = world_.getRandom();

    // UNSTABLE → PARTIAL
    BlockState unstableState =
        block_->defaultState().with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
    world_.setBlockStateCopy(pos, unstableState);
    BlockState mutableState = getTiltState(pos);
    block_->tick(world_, pos, mutableState, rng);
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Partial);

    // PARTIAL → FULL
    mutableState = getTiltState(pos);
    block_->tick(world_, pos, mutableState, rng);
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Full);

    // FULL → NONE（自动重置）
    mutableState = getTiltState(pos);
    block_->tick(world_, pos, mutableState, rng);
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
}

// tick：NONE状态不做任何事
TEST_F(BigDripleafTiltStateMachineTest, Tick_NoneState_DoesNothing)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);
    math::Random& rng = world_.getRandom();

    // 默认就是NONE
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);

    BlockState mutableState = getTiltState(pos);
    world_.clearTrackedCalls();
    block_->tick(world_, pos, mutableState, rng);

    // 仍为NONE，不播放音效
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::None);
    EXPECT_EQ(world_.playSoundCount(), 0u);
}

// ============================================================================
// onProjectileHit 测试
// ============================================================================

class BigDripleafProjectileTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ = std::make_unique<BigDripleafBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(1.0f));
        world_.ensureTickManager();
    }

    void setupDripleafAt(const BlockPos& pos)
    {
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
        world_.setBlockStateCopy(pos, block_->defaultState());
    }

    BlockState getTiltState(const BlockPos& pos) const
    {
        const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
        return state != nullptr ? *state : block_->defaultState();
    }

    std::unique_ptr<BigDripleafBlock> block_;
    BigDripleafTestWorld world_;
};

// 投掷物命中直接设为FULL
TEST_F(BigDripleafProjectileTest, OnProjectileHit_SetsTiltToFull)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    Vector3 hitPos(0.5f, 65.5f, 0.5f);
    Direction hitFace = Direction::Up;
    BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, hitFace, 1.0f);
    TestEntity projectile(world_, mc::test::testEcsRegistry());

    block_->onProjectileHit(world_, state, hitResult, projectile);

    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Full);
}

// 投掷物命中播放TILT_DOWN音效
TEST_F(BigDripleafProjectileTest, OnProjectileHit_PlaysTiltDownSound)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    Vector3 hitPos(0.5f, 65.5f, 0.5f);
    BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, Direction::Up, 1.0f);
    TestEntity projectile(world_, mc::test::testEcsRegistry());

    world_.clearTrackedCalls();
    block_->onProjectileHit(world_, state, hitResult, projectile);

    EXPECT_GE(world_.playSoundCount(), 1u);
    EXPECT_EQ(world_.lastSoundId(), SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_DOWN);
}

// 投掷物命中触发BLOCK_CHANGE游戏事件
TEST_F(BigDripleafProjectileTest, OnProjectileHit_TriggersBlockChangeEvent)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    const BlockState& state = getTiltState(pos);
    Vector3 hitPos(0.5f, 65.5f, 0.5f);
    BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, Direction::Up, 1.0f);
    TestEntity projectile(world_, mc::test::testEcsRegistry());

    world_.clearTrackedCalls();
    block_->onProjectileHit(world_, state, hitResult, projectile);

    // FULL倾斜应触发BLOCK_CHANGE
    EXPECT_GE(world_.gameEventCount(), 1u);
    EXPECT_EQ(world_.lastGameEventId(), gameevent::GameEvents::BLOCK_CHANGE.id());
}

// 投掷物命中时即使有红石信号也设为FULL
TEST_F(BigDripleafProjectileTest, OnProjectileHit_WithRedstoneSignal_StillSetsFull)
{
    BlockPos pos(0, 65, 0);
    setupDripleafAt(pos);

    // 放置红石信号源
    ConstantPowerBlock powerBlock(BlockProperties(Material::ROCK).hardness(1.0f).resistance(1.0f));
    BlockPos powerPos(1, 65, 0);
    world_.setBlockState(powerPos, &powerBlock.defaultState());

    const BlockState& state = getTiltState(pos);
    Vector3 hitPos(0.5f, 65.5f, 0.5f);
    BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, Direction::Up, 1.0f);
    TestEntity projectile(world_, mc::test::testEcsRegistry());

    block_->onProjectileHit(world_, state, hitResult, projectile);

    // 即使有红石信号，投掷物也应设为FULL
    EXPECT_EQ(getTiltState(pos).get(BlockStateProperties::TILT()), BlockStateProperties::Tilt::Full);
}
