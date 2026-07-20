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

#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/TriState.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/FlowerPotBlock.hpp"
#include "common/world/block/blocks/pale_garden/EyeblossomBlock.hpp"
#include "common/world/block/blocks/pale_garden/EyeblossomEnvironment.hpp"
#include "common/world/block/registry/FlowerPotBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using namespace mc::SoundEvents;

// ============================================================================
// 测试用 IWorld 实现
//
// 支持方块状态存储、TickManager、可控的 dayTime/dimension、
// 以及 addTrailParticle / playSound / gameEvent 的捕获。
// ============================================================================

class EyeblossomTestWorld final : public IBlockReader {
public:
    EyeblossomTestWorld() { VanillaBlocks::initialize(); }

    using IBlockReader::getBlockState;
    using IBlockReader::setBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            // 直接存储状态指针（BlockState 在项目中是单例，由 Block::defaultState() 返回稳定引用）
            // 这样 getBlockState 返回的指针与 setBlockState 传入的指针一致，
            // 支持 EyeblossomBlock::tryChangingState 中的指针比较逻辑
            m_blocks[pos] = state;
        }
        return true;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override { return m_dimension; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<EyeblossomTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // ========== 测试控制接口 ==========

    void setDayTime(i64 dayTime) { m_dayTime = dayTime; }
    void setDimension(DimensionId dim) { m_dimension = dim; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

    // ========== 捕获接口 ==========

    /// 捕获到的最后一次 addTrailParticle 调用
    struct TrailParticleCall {
        Vector3 pos;
        Vector3d target;
        u32 color;
        i32 durationInTicks;
    };
    std::vector<TrailParticleCall> trailParticleCalls;

    /// 捕获到的 playSound 调用
    struct PlaySoundCall {
        ResourceLocation soundId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };
    std::vector<PlaySoundCall> playSoundCalls;

    /// 捕获到的 gameEvent 调用次数
    u32 gameEventCallCount = 0;

    /// 捕获到的 setBlockState 调用次数
    u32 setBlockStateCallCount = 0;

    void addTrailParticle(const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks) override
    {
        trailParticleCalls.push_back({pos, targetPosition, color, durationInTicks});
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        playSoundCalls.push_back({soundEventId, category, position, volume, pitch});
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        (void)event;
        (void)pos;
        (void)context;
        ++gameEventCallCount;
    }

    // 重置捕获计数
    void resetCaptures()
    {
        trailParticleCalls.clear();
        playSoundCalls.clear();
        gameEventCallCount = 0;
        setBlockStateCallCount = 0;
    }

private:
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
    i64 m_dayTime = 0;
    DimensionId m_dimension = DimensionId(0); // 主世界
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Easy;
    bool m_clientSide = false;
};

// ============================================================================
// 辅助函数
// ============================================================================

namespace mc::blocks {

/// 通过 BlockRegistry 获取 open_eyeblossom 方块指针
static const EyeblossomBlock* getOpenEyeblossomBlock()
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "open_eyeblossom"));
    return dynamic_cast<const EyeblossomBlock*>(block);
}

/// 通过 BlockRegistry 获取 closed_eyeblossom 方块指针
static const EyeblossomBlock* getClosedEyeblossomBlock()
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "closed_eyeblossom"));
    return dynamic_cast<const EyeblossomBlock*>(block);
}

/// 通过 BlockRegistry 获取 potted_open_eyeblossom 方块指针
static const FlowerPotBlock* getPottedOpenEyeblossomBlock()
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "potted_open_eyeblossom"));
    return dynamic_cast<const FlowerPotBlock*>(block);
}

/// 通过 BlockRegistry 获取 potted_closed_eyeblossom 方块指针
static const FlowerPotBlock* getPottedClosedEyeblossomBlock()
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "potted_closed_eyeblossom"));
    return dynamic_cast<const FlowerPotBlock*>(block);
}

} // namespace mc::blocks

// ============================================================================
// EyeblossomBlock 注册与基础属性测试
// ============================================================================

class EyeblossomBehaviorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        openBlock_ = getOpenEyeblossomBlock();
        closedBlock_ = getClosedEyeblossomBlock();
    }

    const EyeblossomBlock* openBlock_ = nullptr;
    const EyeblossomBlock* closedBlock_ = nullptr;
    EyeblossomTestWorld world_;
};

TEST_F(EyeblossomBehaviorTest, Registered_BothTypesExist)
{
    ASSERT_NE(openBlock_, nullptr);
    ASSERT_NE(closedBlock_, nullptr);
}

TEST_F(EyeblossomBehaviorTest, Type_Open)
{
    EXPECT_EQ(openBlock_->type(), EyeblossomBlock::Type::Open);
    EXPECT_TRUE(openBlock_->isOpen());
}

TEST_F(EyeblossomBehaviorTest, Type_Closed)
{
    EXPECT_EQ(closedBlock_->type(), EyeblossomBlock::Type::Closed);
    EXPECT_FALSE(closedBlock_->isOpen());
}

TEST_F(EyeblossomBehaviorTest, TicksRandomly_BothTypes)
{
    EXPECT_TRUE(openBlock_->ticksRandomly());
    EXPECT_TRUE(closedBlock_->ticksRandomly());
}

TEST_F(EyeblossomBehaviorTest, LightLevel_OpenIs1)
{
    EXPECT_EQ(openBlock_->getLightLevel(openBlock_->defaultState(), &world_, nullptr), 1);
}

TEST_F(EyeblossomBehaviorTest, LightLevel_ClosedIs0)
{
    EXPECT_EQ(closedBlock_->getLightLevel(closedBlock_->defaultState(), &world_, nullptr), 0);
}

TEST_F(EyeblossomBehaviorTest, Transform_OpenToClosed)
{
    const EyeblossomBlock* transformed = openBlock_->transform();
    ASSERT_NE(transformed, nullptr);
    EXPECT_EQ(transformed->type(), EyeblossomBlock::Type::Closed);
}

TEST_F(EyeblossomBehaviorTest, Transform_ClosedToOpen)
{
    const EyeblossomBlock* transformed = closedBlock_->transform();
    ASSERT_NE(transformed, nullptr);
    EXPECT_EQ(transformed->type(), EyeblossomBlock::Type::Open);
}

TEST_F(EyeblossomBehaviorTest, ParticleColor_Open)
{
    EXPECT_EQ(EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Open), 0xFFFCBE22u);
}

TEST_F(EyeblossomBehaviorTest, ParticleColor_Closed)
{
    EXPECT_EQ(EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Closed), 0xFF5F498Fu);
}

TEST_F(EyeblossomBehaviorTest, LongSwitchSound_Open)
{
    EXPECT_EQ(EyeblossomBlock::longSwitchSoundOf(EyeblossomBlock::Type::Open), SoundEvents::BLOCK_EYEBLOSSOM_OPEN_LONG);
}

TEST_F(EyeblossomBehaviorTest, LongSwitchSound_Closed)
{
    EXPECT_EQ(
        EyeblossomBlock::longSwitchSoundOf(EyeblossomBlock::Type::Closed), SoundEvents::BLOCK_EYEBLOSSOM_CLOSE_LONG);
}

TEST_F(EyeblossomBehaviorTest, ShortSwitchSound_Open)
{
    EXPECT_EQ(EyeblossomBlock::shortSwitchSoundOf(EyeblossomBlock::Type::Open), SoundEvents::BLOCK_EYEBLOSSOM_OPEN);
}

TEST_F(EyeblossomBehaviorTest, ShortSwitchSound_Closed)
{
    EXPECT_EQ(EyeblossomBlock::shortSwitchSoundOf(EyeblossomBlock::Type::Closed), SoundEvents::BLOCK_EYEBLOSSOM_CLOSE);
}

// ============================================================================
// EyeblossomEnvironment 环境属性查询测试
// ============================================================================

class EyeblossomEnvironmentTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EyeblossomTestWorld world_;
};

TEST_F(EyeblossomEnvironmentTest, Overworld_Day_ReturnsFalse)
{
    // 白天：tick = 0（黎明）
    world_.setDimension(DimensionId(0));
    world_.setDayTime(0);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::False);

    // 白天：tick = 12599（黄昏前 1 tick）
    world_.setDayTime(12599);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::False);
}

TEST_F(EyeblossomEnvironmentTest, Overworld_Night_ReturnsTrue)
{
    world_.setDimension(DimensionId(0));

    // 夜晚开始：tick = 12600（关键帧）
    world_.setDayTime(12600);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::True);

    // 午夜：tick = 18000
    world_.setDayTime(18000);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::True);

    // 夜晚结束前 1 tick：tick = 23400
    world_.setDayTime(23400);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::True);
}

TEST_F(EyeblossomEnvironmentTest, Overworld_Boundary_23401_ReturnsFalse)
{
    // 关键帧 23401：切换回 False（白天开始）
    world_.setDimension(DimensionId(0));
    world_.setDayTime(23401);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::False);
}

TEST_F(EyeblossomEnvironmentTest, Nether_ReturnsDefault)
{
    // 下界有固定时间，EYEBLOSSOM_OPEN 为 Default
    world_.setDimension(DimensionId(-1));
    world_.setDayTime(12600); // 即便是"夜晚"时间，下界也不切换
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::Default);
}

TEST_F(EyeblossomEnvironmentTest, End_ReturnsDefault)
{
    // 末地有固定时间，EYEBLOSSOM_OPEN 为 Default
    world_.setDimension(DimensionId(1));
    world_.setDayTime(12600);
    EXPECT_EQ(eyeblossom_environment::getEyeblossomOpen(world_, BlockPos(0, 0, 0)), util::TriState::Default);
}

TEST_F(EyeblossomEnvironmentTest, TriStateToBoolean_Fallback)
{
    // True -> true
    EXPECT_TRUE(util::triStateToBoolean(util::TriState::True, false));
    EXPECT_TRUE(util::triStateToBoolean(util::TriState::True, true));

    // False -> false
    EXPECT_FALSE(util::triStateToBoolean(util::TriState::False, false));
    EXPECT_FALSE(util::triStateToBoolean(util::TriState::False, true));

    // Default -> fallback
    EXPECT_TRUE(util::triStateToBoolean(util::TriState::Default, true));
    EXPECT_FALSE(util::triStateToBoolean(util::TriState::Default, false));
}

// ============================================================================
// EyeblossomBlock randomTick 状态切换测试
// ============================================================================

class EyeblossomRandomTickTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        openBlock_ = getOpenEyeblossomBlock();
        closedBlock_ = getClosedEyeblossomBlock();
        world_.setDimension(DimensionId(0)); // 主世界
    }

    const EyeblossomBlock* openBlock_ = nullptr;
    const EyeblossomBlock* closedBlock_ = nullptr;
    EyeblossomTestWorld world_;
};

TEST_F(EyeblossomRandomTickTest, ClosedToOpen_AtNight)
{
    // 主世界夜晚：EYEBLOSSOM_OPEN=True，闭合眼眸花应切换为开放
    world_.setDayTime(18000); // 午夜
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &closedBlock_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, pos, stateRef, random);

    // 验证方块已切换为 open_eyeblossom
    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), openBlock_);

    // 验证播放了长音效（Closed -> Open，新状态 Open 的长音效）
    ASSERT_FALSE(world_.playSoundCalls.empty());
    EXPECT_EQ(world_.playSoundCalls.back().soundId, SoundEvents::BLOCK_EYEBLOSSOM_OPEN_LONG);

    // 验证生成了转换粒子（颜色为新状态 Open 的颜色）
    ASSERT_FALSE(world_.trailParticleCalls.empty());
    EXPECT_EQ(world_.trailParticleCalls.back().color, EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Open));

    // 验证触发了 BLOCK_CHANGE 游戏事件
    EXPECT_GE(world_.gameEventCallCount, 1u);
}

TEST_F(EyeblossomRandomTickTest, OpenToClosed_Daytime)
{
    // 主世界白天：EYEBLOSSOM_OPEN=False，开放眼眸花应切换为闭合
    world_.setDayTime(6000); // 正午
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &openBlock_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(openBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), closedBlock_);

    // 长音效（Open -> Closed，新状态 Closed 的长音效）
    ASSERT_FALSE(world_.playSoundCalls.empty());
    EXPECT_EQ(world_.playSoundCalls.back().soundId, SoundEvents::BLOCK_EYEBLOSSOM_CLOSE_LONG);

    // 转换粒子颜色为 Closed 的颜色
    ASSERT_FALSE(world_.trailParticleCalls.empty());
    EXPECT_EQ(world_.trailParticleCalls.back().color, EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Closed));
}

TEST_F(EyeblossomRandomTickTest, NoSwitch_Night_OpenStaysOpen)
{
    // 主世界夜晚：EYEBLOSSOM_OPEN=True，开放眼眸花保持开放（环境与状态一致）
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &openBlock_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(openBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), openBlock_);

    // 未播放音效、未生成粒子、未触发游戏事件
    EXPECT_TRUE(world_.playSoundCalls.empty());
    EXPECT_TRUE(world_.trailParticleCalls.empty());
    EXPECT_EQ(world_.gameEventCallCount, 0u);
}

TEST_F(EyeblossomRandomTickTest, NoSwitch_Daytime_ClosedStaysClosed)
{
    // 主世界白天：EYEBLOSSOM_OPEN=False，闭合眼眸花保持闭合（环境与状态一致）
    world_.setDayTime(6000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &closedBlock_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), closedBlock_);
    EXPECT_TRUE(world_.playSoundCalls.empty());
    EXPECT_TRUE(world_.trailParticleCalls.empty());
}

TEST_F(EyeblossomRandomTickTest, NoSwitch_Nether_OpenStaysOpen)
{
    // 下界：EYEBLOSSOM_OPEN=Default，回退到当前状态（Open），不切换
    world_.setDimension(DimensionId(-1));
    world_.setDayTime(18000); // 即便时间符合，下界也不切换
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &openBlock_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(openBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), openBlock_);
    EXPECT_TRUE(world_.playSoundCalls.empty());
}

TEST_F(EyeblossomRandomTickTest, NoSwitch_Nether_ClosedStaysClosed)
{
    // 下界：闭合眼眸花保持闭合
    world_.setDimension(DimensionId(-1));
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &closedBlock_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), closedBlock_);
}

TEST_F(EyeblossomRandomTickTest, NoSwitch_End_OpenStaysOpen)
{
    // 末地：与下界相同，EYEBLOSSOM_OPEN=Default
    world_.setDimension(DimensionId(1));
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &openBlock_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(openBlock_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), openBlock_);
}

// ============================================================================
// 连锁触发测试
// ============================================================================

class EyeblossomChainTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        openBlock_ = getOpenEyeblossomBlock();
        closedBlock_ = getClosedEyeblossomBlock();
        world_.setDimension(DimensionId(0));
        world_.setDayTime(18000); // 夜晚，EYEBLOSSOM_OPEN=True，Closed -> Open
    }

    const EyeblossomBlock* openBlock_ = nullptr;
    const EyeblossomBlock* closedBlock_ = nullptr;
    EyeblossomTestWorld world_;
};

TEST_F(EyeblossomChainTest, ChainTrigger_SchedulesNeighborTicks)
{
    // 在原点放置 closed_eyeblossom（夜晚会切换为 open），并在 3×2×3 范围内放置若干同种 closed 方块
    const BlockPos origin(0, 64, 0);
    world_.setBlockAt(origin, &closedBlock_->defaultState());

    // 范围内的邻居（距离 1，同为 closed）
    const BlockPos neighbor1(1, 64, 0);
    const BlockPos neighbor2(-1, 64, 0);
    const BlockPos neighbor3(0, 65, 0); // y+1 在 2 范围内
    const BlockPos neighbor4(0, 64, 1);
    world_.setBlockAt(neighbor1, &closedBlock_->defaultState());
    world_.setBlockAt(neighbor2, &closedBlock_->defaultState());
    world_.setBlockAt(neighbor3, &closedBlock_->defaultState());
    world_.setBlockAt(neighbor4, &closedBlock_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(origin));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, origin, stateRef, random);

    // 验证邻居被调度了 tick（连锁触发）
    auto& tickManager = world_.tickManager();
    EXPECT_TRUE(tickManager.isBlockTickScheduled(neighbor1, *closedBlock_));
    EXPECT_TRUE(tickManager.isBlockTickScheduled(neighbor2, *closedBlock_));
    EXPECT_TRUE(tickManager.isBlockTickScheduled(neighbor3, *closedBlock_));
    EXPECT_TRUE(tickManager.isBlockTickScheduled(neighbor4, *closedBlock_));
}

TEST_F(EyeblossomChainTest, ChainTrigger_OutsideRange_NotScheduled)
{
    // 3×2×3 范围外的方块不应被调度
    const BlockPos origin(0, 64, 0);
    world_.setBlockAt(origin, &closedBlock_->defaultState());

    // 范围外的位置（半径 3/2/3 闭区间之外）
    const BlockPos outside(4, 64, 0);  // x=4 超出 [-3, 3]
    const BlockPos outside2(0, 67, 0); // y=67 超出 [62, 66]（origin.y=64, [-2, +2]）
    world_.setBlockAt(outside, &closedBlock_->defaultState());
    world_.setBlockAt(outside2, &closedBlock_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(origin));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, origin, stateRef, random);

    auto& tickManager = world_.tickManager();
    EXPECT_FALSE(tickManager.isBlockTickScheduled(outside, *closedBlock_));
    EXPECT_FALSE(tickManager.isBlockTickScheduled(outside2, *closedBlock_));
}

TEST_F(EyeblossomChainTest, ChainTrigger_OnlySameState)
{
    // 范围内但状态不同（open）的方块不应被调度
    const BlockPos origin(0, 64, 0);
    world_.setBlockAt(origin, &closedBlock_->defaultState());

    // 邻居是 open_eyeblossom（状态不同，不应连锁）
    const BlockPos openNeighbor(1, 64, 0);
    world_.setBlockAt(openNeighbor, &openBlock_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(origin));
    math::Random random(12345);
    const_cast<EyeblossomBlock*>(closedBlock_)->randomTick(world_, origin, stateRef, random);

    auto& tickManager = world_.tickManager();
    EXPECT_FALSE(tickManager.isBlockTickScheduled(openNeighbor, *closedBlock_));
    EXPECT_FALSE(tickManager.isBlockTickScheduled(openNeighbor, *openBlock_));
}

// ============================================================================
// FlowerPotBlock 眼眸花盆栽测试
// ============================================================================

class FlowerPotEyeblossomTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        pottedOpen_ = getPottedOpenEyeblossomBlock();
        pottedClosed_ = getPottedClosedEyeblossomBlock();
        openBlock_ = getOpenEyeblossomBlock();
        closedBlock_ = getClosedEyeblossomBlock();
        world_.setDimension(DimensionId(0));
    }

    const FlowerPotBlock* pottedOpen_ = nullptr;
    const FlowerPotBlock* pottedClosed_ = nullptr;
    const EyeblossomBlock* openBlock_ = nullptr;
    const EyeblossomBlock* closedBlock_ = nullptr;
    EyeblossomTestWorld world_;
};

TEST_F(FlowerPotEyeblossomTest, Registered_BothTypesExist)
{
    ASSERT_NE(pottedOpen_, nullptr);
    ASSERT_NE(pottedClosed_, nullptr);
}

TEST_F(FlowerPotEyeblossomTest, PottedContent_IsEyeblossom)
{
    ASSERT_NE(pottedOpen_->getPotted(), nullptr);
    EXPECT_EQ(pottedOpen_->getPotted()->blockLocation(), ResourceLocation("minecraft", "open_eyeblossom"));

    ASSERT_NE(pottedClosed_->getPotted(), nullptr);
    EXPECT_EQ(pottedClosed_->getPotted()->blockLocation(), ResourceLocation("minecraft", "closed_eyeblossom"));
}

TEST_F(FlowerPotEyeblossomTest, TicksRandomly_PottedOpen_ReturnsTrue)
{
    // 关键集成点：FlowerPotBlock::ticksRandomly() override 应返回 true
    // 这保证 ChunkSection 的 m_blockTickRefCount 会增加，randomTick 会被调用
    EXPECT_TRUE(pottedOpen_->ticksRandomly());
}

TEST_F(FlowerPotEyeblossomTest, TicksRandomly_PottedClosed_ReturnsTrue)
{
    EXPECT_TRUE(pottedClosed_->ticksRandomly());
}

TEST_F(FlowerPotEyeblossomTest, TicksRandomly_NormalPot_ReturnsFalse)
{
    // 普通花盆（非眼眸花）不响应随机刻
    Block* emptyPot = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "flower_pot"));
    ASSERT_NE(emptyPot, nullptr);
    const FlowerPotBlock* emptyPotBlock = dynamic_cast<const FlowerPotBlock*>(emptyPot);
    ASSERT_NE(emptyPotBlock, nullptr);
    EXPECT_FALSE(emptyPotBlock->ticksRandomly());
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_ClosedToOpen_AtNight)
{
    // 主世界夜晚：EYEBLOSSOM_OPEN=True，盆栽闭合眼眸花应切换为开放
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &pottedClosed_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(pottedClosed_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), pottedOpen_);

    // 盆栽版使用长音效（Closed -> Open，新状态 Open 的长音效）
    ASSERT_FALSE(world_.playSoundCalls.empty());
    EXPECT_EQ(world_.playSoundCalls.back().soundId, SoundEvents::BLOCK_EYEBLOSSOM_OPEN_LONG);

    // 盆栽版也生成转换粒子（颜色为新状态 Open）
    ASSERT_FALSE(world_.trailParticleCalls.empty());
    EXPECT_EQ(world_.trailParticleCalls.back().color, EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Open));
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_OpenToClosed_Daytime)
{
    // 主世界白天：EYEBLOSSOM_OPEN=False，盆栽开放眼眸花应切换为闭合
    world_.setDayTime(6000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &pottedOpen_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(pottedOpen_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), pottedClosed_);

    ASSERT_FALSE(world_.playSoundCalls.empty());
    EXPECT_EQ(world_.playSoundCalls.back().soundId, SoundEvents::BLOCK_EYEBLOSSOM_CLOSE_LONG);

    ASSERT_FALSE(world_.trailParticleCalls.empty());
    EXPECT_EQ(world_.trailParticleCalls.back().color, EyeblossomBlock::particleColorOf(EyeblossomBlock::Type::Closed));
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_NoSwitch_Night_OpenStaysOpen)
{
    // 主世界夜晚：EYEBLOSSOM_OPEN=True，盆栽开放眼眸花保持开放（环境与状态一致）
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &pottedOpen_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(pottedOpen_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), pottedOpen_);
    EXPECT_TRUE(world_.playSoundCalls.empty());
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_NoChain_NeighborsNotScheduled)
{
    // 盆栽版不连锁触发周围方块
    // 夜晚放置 potted_closed_eyeblossom（会切换），邻居也是 potted_closed_eyeblossom
    world_.setDayTime(18000);
    const BlockPos origin(0, 64, 0);
    world_.setBlockAt(origin, &pottedClosed_->defaultState());

    // 在范围内放置另一个盆栽眼眸花
    const BlockPos neighbor(1, 64, 0);
    world_.setBlockAt(neighbor, &pottedClosed_->defaultState());

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(origin));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(pottedClosed_)->randomTick(world_, origin, stateRef, random);

    // 邻居不应被调度 tick（盆栽版不连锁）
    auto& tickManager = world_.tickManager();
    EXPECT_FALSE(tickManager.isBlockTickScheduled(neighbor, *pottedClosed_));
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_NoSwitch_Nether)
{
    // 下界：盆栽眼眸花保持当前状态
    world_.setDimension(DimensionId(-1));
    world_.setDayTime(18000);
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &pottedOpen_->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(pottedOpen_)->randomTick(world_, pos, stateRef, random);

    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), pottedOpen_);
    EXPECT_TRUE(world_.playSoundCalls.empty());
}

TEST_F(FlowerPotEyeblossomTest, RandomTick_NormalPot_NoOp)
{
    // 普通花盆（非眼眸花）的 randomTick 不应做任何事
    Block* emptyPot = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "flower_pot"));
    ASSERT_NE(emptyPot, nullptr);
    const FlowerPotBlock* emptyPotBlock = dynamic_cast<const FlowerPotBlock*>(emptyPot);
    ASSERT_NE(emptyPotBlock, nullptr);

    // 即便调用 randomTick 也不应切换（ticksRandomly 返回 false 的早返回）
    const BlockPos pos(0, 64, 0);
    world_.setBlockAt(pos, &emptyPotBlock->defaultState());

    world_.resetCaptures();

    BlockState& stateRef = const_cast<BlockState&>(*world_.getBlockAt(pos));
    math::Random random(12345);
    const_cast<FlowerPotBlock*>(emptyPotBlock)->randomTick(world_, pos, stateRef, random);

    // 方块未变化
    const BlockState* newState = world_.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->getBlock(), emptyPotBlock);
    EXPECT_TRUE(world_.playSoundCalls.empty());
    EXPECT_TRUE(world_.trailParticleCalls.empty());
}

// ============================================================================
// EyeblossomBlock::onEntityCollision 实体碰撞集成测试
//
// 端到端覆盖 MC 1.21.11 EyeblossomBlock#entityInside 行为：
// - 蜜蜂接触开放眼眸花 → 25 tick Poison I
// - 蜜蜂接触闭合眼眸花 → 不中毒（不在 BEE_ATTRACTIVE 标签中）
// - 和平难度跳过
// - 客户端世界跳过
// - 已中毒蜜蜂不重复施加
// - 非蜜蜂实体不中毒
// ============================================================================

class EyeblossomBeeCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        openBlock_ = getOpenEyeblossomBlock();
        closedBlock_ = getClosedEyeblossomBlock();

        // 默认服务端 + 简单难度（非和平）
        world_.setClientSide(false);
        world_.setDifficulty(Difficulty::Easy);
    }

    /// 创建一只蜜蜂并绑定到世界
    std::unique_ptr<BeeEntity> makeBee()
    {
        auto bee = std::make_unique<BeeEntity>(EntityInstanceId(1));
        bee->setWorld(&world_);
        return bee;
    }

    const EyeblossomBlock* openBlock_ = nullptr;
    const EyeblossomBlock* closedBlock_ = nullptr;
    EyeblossomTestWorld world_;
};

TEST_F(EyeblossomBeeCollisionTest, OpenEyeblossom_AppliesPoisonToBee)
{
    // 蜜蜂接触开放眼眸花：应获得 25 tick Poison I (amplifier=0)
    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    EXPECT_FALSE(bee->hasEffect(entity::effect::EffectType::Poison));

    openBlock_->onEntityCollision(state, world_, pos, *bee);

    // 验证中毒效果已施加
    EXPECT_TRUE(bee->hasEffect(entity::effect::EffectType::Poison));
    const auto* effect = bee->getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(effect, nullptr);
    EXPECT_EQ(effect->type(), entity::effect::EffectType::Poison);
    EXPECT_EQ(effect->duration(), 25);
    EXPECT_EQ(effect->amplifier(), 0);
}

TEST_F(EyeblossomBeeCollisionTest, ClosedEyeblossom_DoesNotApplyPoison)
{
    // 闭合眼眸花不在 BEE_ATTRACTIVE 标签中，蜜蜂接触不中毒
    const BlockPos pos(0, 64, 0);
    const BlockState& state = closedBlock_->defaultState();
    auto bee = makeBee();

    EXPECT_FALSE(bee->hasEffect(entity::effect::EffectType::Poison));

    closedBlock_->onEntityCollision(state, world_, pos, *bee);

    EXPECT_FALSE(bee->hasEffect(entity::effect::EffectType::Poison));
}

TEST_F(EyeblossomBeeCollisionTest, PeacefulDifficulty_SkipsPoison)
{
    // 和平难度下，开放眼眸花也不对蜜蜂施加中毒
    world_.setDifficulty(Difficulty::Peaceful);

    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    openBlock_->onEntityCollision(state, world_, pos, *bee);

    EXPECT_FALSE(bee->hasEffect(entity::effect::EffectType::Poison));
}

TEST_F(EyeblossomBeeCollisionTest, ClientSide_SkipsPoison)
{
    // 客户端世界不处理状态变更，不施加中毒
    world_.setClientSide(true);

    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    openBlock_->onEntityCollision(state, world_, pos, *bee);

    EXPECT_FALSE(bee->hasEffect(entity::effect::EffectType::Poison));
}

TEST_F(EyeblossomBeeCollisionTest, AlreadyPoisoned_DoesNotRefresh)
{
    // 已中毒的蜜蜂再次接触开放眼眸花，不应刷新剩余时间或等级
    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    // 先施加一个 100 tick Poison II 的中毒效果
    ASSERT_TRUE(bee->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 1)));
    ASSERT_TRUE(bee->hasEffect(entity::effect::EffectType::Poison));
    const auto* before = bee->getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(before, nullptr);
    EXPECT_EQ(before->duration(), 100);
    EXPECT_EQ(before->amplifier(), 1);

    // 触发碰撞
    openBlock_->onEntityCollision(state, world_, pos, *bee);

    // 中毒效果不应被覆盖为 25 tick Poison I
    const auto* after = bee->getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->duration(), 100);
    EXPECT_EQ(after->amplifier(), 1);
}

TEST_F(EyeblossomBeeCollisionTest, NonBeeEntity_DoesNotApplyPoison)
{
    // 非蜜蜂实体接触开放眼眸花不中毒
    // 使用一个最小的 AnimalEntity 子类作为非蜜蜂实体，验证 dynamic_cast<BeeEntity*> 失败时的早返回
    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();

    struct NonBeeAnimal : public AnimalEntity {
        explicit NonBeeAnimal(EntityInstanceId id)
            : AnimalEntity(id)
        {}
        // AnimalEntity::spawnBaby 是纯虚，必须实现
        std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& /*partner*/) override { return nullptr; }
    };

    NonBeeAnimal animal(EntityInstanceId(2));
    animal.setWorld(&world_);

    EXPECT_FALSE(animal.hasEffect(entity::effect::EffectType::Poison));

    openBlock_->onEntityCollision(state, world_, pos, animal);

    EXPECT_FALSE(animal.hasEffect(entity::effect::EffectType::Poison));
}

TEST_F(EyeblossomBeeCollisionTest, HardDifficulty_AppliesPoison)
{
    // 困难难度同样施加中毒（仅和平跳过）
    world_.setDifficulty(Difficulty::Hard);

    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    openBlock_->onEntityCollision(state, world_, pos, *bee);

    EXPECT_TRUE(bee->hasEffect(entity::effect::EffectType::Poison));
    const auto* effect = bee->getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(effect, nullptr);
    EXPECT_EQ(effect->duration(), 25);
    EXPECT_EQ(effect->amplifier(), 0);
}

TEST_F(EyeblossomBeeCollisionTest, NormalDifficulty_AppliesPoison)
{
    // 普通难度同样施加中毒
    world_.setDifficulty(Difficulty::Normal);

    const BlockPos pos(0, 64, 0);
    const BlockState& state = openBlock_->defaultState();
    auto bee = makeBee();

    openBlock_->onEntityCollision(state, world_, pos, *bee);

    EXPECT_TRUE(bee->hasEffect(entity::effect::EffectType::Poison));
}
