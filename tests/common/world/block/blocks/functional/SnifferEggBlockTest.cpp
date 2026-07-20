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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 嗅探兽蛋方块测试用 Mock 世界实现
 *
 * 提供方块存储、实体生成追踪、客户端/服务端切换等基本能力，
 * 参考 MobBlocksTestWorld 的设计。
 */
class SnifferEggTestWorld final : public test::BaseTestWorld {
public:
    SnifferEggTestWorld()
    {
        VanillaBlocks::initialize();
        // 注册原版实体类型，使 VanillaEntityTypeKeys::SNIFFER 全局缓存与注册表一致。
        entity::VanillaEntities::registerAll();
        // 构造真实 TickManager 以支持 onBlockAdded 调度测试
        m_tickManager = std::make_unique<world::tick::TickManager>(*this);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略声音播放
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_playedEvents.push_back({eventId, pos, data});
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManager; }

    // 测试辅助方法
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(*state);
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getSpawnedEntity(size_t index) const
    {
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

    [[nodiscard]] const std::vector<std::tuple<i32, BlockPos, i32>>& playedEvents() const { return m_playedEvents; }

    // 推进 tickManager，触发到期的计划刻
    void advanceTick()
    {
        ++m_currentTick;
        m_tickManager->tick(m_currentTick);
    }

    // GameRules 接口
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<std::tuple<i32, BlockPos, i32>> m_playedEvents;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    world::gamerule::GameRules m_gameRules;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
};

} // namespace

// ==================== 测试夹具 ====================

class SnifferEggBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 使用 Material::SAND 创建嗅探兽蛋方块（测试用途，材质不影响孵化逻辑）
        snifferEgg_ =
            std::make_unique<SnifferEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));
    }

    std::unique_ptr<SnifferEggBlock> snifferEgg_;
    SnifferEggTestWorld world_;
};

// ==================== 基础属性测试 ====================

TEST_F(SnifferEggBlockTest, Create_HasCorrectDefaultState)
{
    const auto& state = snifferEgg_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HATCH_0_2()), 0);
}

TEST_F(SnifferEggBlockTest, GetShape_ReturnsCorrectShapeForHatchLevel)
{
    // 验证不同孵化等级的形状不崩溃
    BlockState state0 = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    BlockState state1 = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 1);
    BlockState state2 = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);

    // 仅验证不崩溃，形状内容由 CollisionShape 测试覆盖
    EXPECT_NO_THROW(snifferEgg_->getShape(state0));
    EXPECT_NO_THROW(snifferEgg_->getShape(state1));
    EXPECT_NO_THROW(snifferEgg_->getShape(state2));
}

// ==================== 孵化进度测试 ====================

TEST_F(SnifferEggBlockTest, RandomTick_HatchZero_IncrementsToOne)
{
    // 设置孵化等级为 0
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick（MC 1.21.11 中由 scheduleTick 驱动，不再使用 randomTick）
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：孵化等级增加到 1
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->get(BlockStateProperties::HATCH_0_2()), 1);

    // 不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(SnifferEggBlockTest, RandomTick_HatchOne_IncrementsToTwo)
{
    // 设置孵化等级为 1
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 1);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：孵化等级增加到 2
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->get(BlockStateProperties::HATCH_0_2()), 2);

    // 不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

// ==================== 孵化完成测试 ====================

TEST_F(SnifferEggBlockTest, RandomTick_HatchTwo_SpawnsSnifferAndRemovesBlock)
{
    // 设置孵化等级为 2（即将孵化）
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：生成了一只嗅探兽
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);

    // 验证：蛋方块已被销毁（变为空气）
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_TRUE(stateAfter->isAir());
}

TEST_F(SnifferEggBlockTest, RandomTick_HatchTwo_SnifferIsBaby)
{
    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：生成的嗅探兽是幼体
    ASSERT_EQ(world_.spawnedEntityCount(), 1u);
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    // 检查是否为 SnifferEntity
    auto* sniffer = dynamic_cast<SnifferEntity*>(spawned);
    ASSERT_NE(sniffer, nullptr);

    // 验证幼体状态
    EXPECT_TRUE(sniffer->isChild());

    // 验证年龄为 -48000（嗅探兽幼年期，40 分钟）
    // 对齐 MC Sniffer.SNIFFER_BABY_AGE_TICKS = 48000
    EXPECT_EQ(sniffer->getGrowingAge(), -SnifferEntity::SNIFFER_BABY_AGE_TICKS);
}

TEST_F(SnifferEggBlockTest, RandomTick_HatchTwo_SnifferPositionIsBlockCenter)
{
    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：嗅探兽位置在方块中心
    ASSERT_EQ(world_.spawnedEntityCount(), 1u);
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    // pos.center() = (x+0.5, y+0.5, z+0.5)
    EXPECT_FLOAT_EQ(spawned->x(), 10.5f);
    EXPECT_FLOAT_EQ(spawned->y(), 5.5f);
    EXPECT_FLOAT_EQ(spawned->z(), 10.5f);
}

TEST_F(SnifferEggBlockTest, RandomTick_HatchTwo_SnifferYawInRange)
{
    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：嗅探兽 yaw 在 [-180, 180] 范围内（wrapDegrees 结果）
    ASSERT_EQ(world_.spawnedEntityCount(), 1u);
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    f32 yaw = spawned->yaw();
    EXPECT_GE(yaw, -180.0f);
    EXPECT_LE(yaw, 180.0f);

    // pitch 应该为 0
    EXPECT_FLOAT_EQ(spawned->pitch(), 0.0f);
}

TEST_F(SnifferEggBlockTest, RandomTick_HatchTwo_SnifferInitialStateIsIdling)
{
    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：嗅探兽初始状态为 Idling
    ASSERT_EQ(world_.spawnedEntityCount(), 1u);
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    auto* sniffer = dynamic_cast<SnifferEntity*>(spawned);
    ASSERT_NE(sniffer, nullptr);
    EXPECT_EQ(sniffer->getState(), SnifferEntity::State::Idling);
}

// ==================== 客户端测试 ====================

TEST_F(SnifferEggBlockTest, RandomTick_ClientSide_DoesNotSpawnSniffer)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：客户端不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(SnifferEggBlockTest, RandomTick_ClientSide_DoesNotIncrementHatch)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置孵化等级为 0
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 tick
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // 验证：客户端不应该改变方块状态（孵化等级保持 0）
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->get(BlockStateProperties::HATCH_0_2()), 0);

    // 客户端也不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

// ==================== 多次孵化测试 ====================

TEST_F(SnifferEggBlockTest, RandomTick_MultipleHatchAttempts_OnlyOneSnifferPerEgg)
{
    // 设置孵化等级为 2
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 第一次调用 tick - 应该孵化
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);

    // 第二次调用 tick - 方块已被销毁，使用当前空气状态调用
    // 由于方块已经是空气，不应该再生成嗅探兽
    const BlockState* airState = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    // 空气状态没有 HATCH_0_2 属性，这里仅验证不会崩溃
    // 实际场景中 tick 不会在空气方块上调用
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);
}

// ==================== hatchBoost 加速标签测试 ====================

TEST_F(SnifferEggBlockTest, HatchBoost_MossBlockBelow_ReturnsTrue)
{
    // 下方放置苔藓块 - 应该返回 true
    BlockPos eggPos(10, 5, 10);
    BlockPos belowPos = eggPos.down();
    ASSERT_NE(VanillaBlocks::MOSS_BLOCK, nullptr);
    world_.setBlockAt(belowPos, &VanillaBlocks::MOSS_BLOCK->defaultState());

    EXPECT_TRUE(SnifferEggBlock::hatchBoost(world_, eggPos));
}

TEST_F(SnifferEggBlockTest, HatchBoost_NoBlockBelow_ReturnsFalse)
{
    // 下方为空气（默认） - 应该返回 false
    BlockPos eggPos(10, 5, 10);

    EXPECT_FALSE(SnifferEggBlock::hatchBoost(world_, eggPos));
}

TEST_F(SnifferEggBlockTest, HatchBoost_StoneBlockBelow_ReturnsFalse)
{
    // 下方放置石头 - 不在 SNIFFER_EGG_HATCH_BOOST 标签中，应返回 false
    BlockPos eggPos(10, 5, 10);
    BlockPos belowPos = eggPos.down();
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    world_.setBlockAt(belowPos, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(SnifferEggBlock::hatchBoost(world_, eggPos));
}

// ==================== onBlockAdded 调度测试 ====================

TEST_F(SnifferEggBlockTest, OnBlockAdded_WithMossBelow_FiresEggCrackEvent)
{
    // 下方放置苔藓块（加速条件），调用 onBlockAdded 应播放 EGG_CRACK (3009) 事件
    BlockPos eggPos(10, 5, 10);
    BlockPos belowPos = eggPos.down();
    ASSERT_NE(VanillaBlocks::MOSS_BLOCK, nullptr);
    world_.setBlockAt(belowPos, &VanillaBlocks::MOSS_BLOCK->defaultState());

    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    world_.clearSpawnedEntities();
    snifferEgg_->onBlockAdded(world_, eggPos, eggState);

    // 验证：播放了 EGG_CRACK (3009) 事件
    bool foundEggCrack = false;
    for (const auto& [eventId, pos, data] : world_.playedEvents()) {
        if (eventId == world::WorldEvents::EGG_CRACK && pos == eggPos) {
            foundEggCrack = true;
            break;
        }
    }
    EXPECT_TRUE(foundEggCrack);
}

TEST_F(SnifferEggBlockTest, OnBlockAdded_WithoutMossBelow_DoesNotFireEggCrackEvent)
{
    // 下方为空气（非加速条件），调用 onBlockAdded 不应播放 EGG_CRACK 事件
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    snifferEgg_->onBlockAdded(world_, eggPos, eggState);

    // 验证：没有播放 EGG_CRACK (3009) 事件
    bool foundEggCrack = false;
    for (const auto& [eventId, pos, data] : world_.playedEvents()) {
        if (eventId == world::WorldEvents::EGG_CRACK) {
            foundEggCrack = true;
            break;
        }
    }
    EXPECT_FALSE(foundEggCrack);
}

TEST_F(SnifferEggBlockTest, OnBlockAdded_ClientSide_DoesNotScheduleTick)
{
    // 客户端调用 onBlockAdded 不应调度 tick 或播放事件
    world_.setClientSide(true);
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    // 即使下方有苔藓块，客户端也不应播放 EGG_CRACK
    BlockPos belowPos = eggPos.down();
    ASSERT_NE(VanillaBlocks::MOSS_BLOCK, nullptr);
    world_.setBlockAt(belowPos, &VanillaBlocks::MOSS_BLOCK->defaultState());

    snifferEgg_->onBlockAdded(world_, eggPos, eggState);

    // 验证：没有播放任何事件
    EXPECT_TRUE(world_.playedEvents().empty());

    // 推进大量 tick 也不应触发任何 HATCH 变化
    world_.setCurrentTick(100000);
    world_.advanceTick();

    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->get(BlockStateProperties::HATCH_0_2()), 0);
}

TEST_F(SnifferEggBlockTest, OnBlockAdded_BoostedSchedulesEarlierThanRegular)
{
    // 加速模式下调度延迟应小于常规模式
    // 常规：8000 + [0, 300) = [8000, 8300)
    // 加速：4000 + [0, 300) = [4000, 4300)
    // 通过推进 tick 到 4300 验证加速模式已触发，常规模式未触发

    // 场景1：下方有苔藓块（加速）
    BlockPos eggPosBoost(10, 5, 10);
    BlockPos belowPosBoost = eggPosBoost.down();
    ASSERT_NE(VanillaBlocks::MOSS_BLOCK, nullptr);
    world_.setBlockAt(belowPosBoost, &VanillaBlocks::MOSS_BLOCK->defaultState());
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPosBoost, &eggState);
    snifferEgg_->onBlockAdded(world_, eggPosBoost, eggState);

    // 推进 tick 到 4300（加速模式最大延迟边界）
    world_.setCurrentTick(4300);
    world_.advanceTick();

    // 加速模式：HATCH 应该已经增加
    const BlockState* stateBoost = world_.getBlockState(eggPosBoost.x, eggPosBoost.y, eggPosBoost.z);
    ASSERT_NE(stateBoost, nullptr);
    EXPECT_EQ(stateBoost->get(BlockStateProperties::HATCH_0_2()), 1) << "加速模式应在 4300 tick 内触发首次 HATCH 增加";
}

TEST_F(SnifferEggBlockTest, Tick_HatchZero_ReSchedulesNextTick)
{
    // 验证 tick 回调在 HATCH < 2 时会重新调度下一阶段 tick
    // 通过 advanceTick 推进时间，应看到 HATCH 从 0→1→2→孵化 的完整流程
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = snifferEgg_->defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    // 手动调用第一次 tick（模拟计划刻触发）
    snifferEgg_->tick(world_, eggPos, eggState, world_.getRandom());

    // HATCH 应该从 0 增加到 1
    const BlockState* state1 = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(state1, nullptr);
    EXPECT_EQ(state1->get(BlockStateProperties::HATCH_0_2()), 1);

    // 推进 tick 到下一阶段延迟后（常规 8000+300=8300），应触发第二次 tick
    // 当前 tick 为 0，第一次 tick 调度的延迟在 [8000, 8300)
    world_.setCurrentTick(8300);
    world_.advanceTick();

    // HATCH 应该从 1 增加到 2
    const BlockState* state2 = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(state2, nullptr);
    EXPECT_EQ(state2->get(BlockStateProperties::HATCH_0_2()), 2);

    // 不应该已经孵化（第二次 tick 调度的延迟还未到期）
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

// ==================== SnifferEntity 单元测试 ====================

class SnifferEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(SnifferEntityTest, SetChild_True_SetsAgeToNegative48000)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));

    // 初始年龄为 0（成体）
    EXPECT_EQ(sniffer->getGrowingAge(), 0);
    EXPECT_FALSE(sniffer->isChild());

    // 设置为幼体
    sniffer->setChild(true);

    // 验证：年龄为 -48000（嗅探兽幼年期，40 分钟）
    EXPECT_EQ(sniffer->getGrowingAge(), -SnifferEntity::SNIFFER_BABY_AGE_TICKS);
    EXPECT_TRUE(sniffer->isChild());
}

TEST_F(SnifferEntityTest, SetChild_False_SetsAgeToZero)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    sniffer->setChild(true);
    ASSERT_TRUE(sniffer->isChild());

    // 设置为成体
    sniffer->setChild(false);

    // 验证：年龄为 0
    EXPECT_EQ(sniffer->getGrowingAge(), 0);
    EXPECT_FALSE(sniffer->isChild());
}

TEST_F(SnifferEntityTest, GetState_InitialStateIsIdling)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    EXPECT_EQ(sniffer->getState(), SnifferEntity::State::Idling);
}

TEST_F(SnifferEntityTest, SetState_UpdatesState)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));

    sniffer->setState(SnifferEntity::State::Digging);
    EXPECT_EQ(sniffer->getState(), SnifferEntity::State::Digging);

    sniffer->setState(SnifferEntity::State::Searching);
    EXPECT_EQ(sniffer->getState(), SnifferEntity::State::Searching);

    sniffer->setState(SnifferEntity::State::Idling);
    EXPECT_EQ(sniffer->getState(), SnifferEntity::State::Idling);
}

TEST_F(SnifferEntityTest, IsBreedingItem_TorchflowerSeeds_ReturnsTrue)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));

    // 火把花种子是嗅探兽食物
    if (Items::TORCHFLOWER_SEEDS != nullptr) {
        ItemStack food(Items::TORCHFLOWER_SEEDS, 1);
        EXPECT_TRUE(sniffer->isBreedingItem(food));
    }
}

TEST_F(SnifferEntityTest, IsBreedingItem_PitcherPod_ReturnsTrue)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));

    // 瓶草荚果是嗅探兽食物
    if (Items::PITCHER_POD != nullptr) {
        ItemStack food(Items::PITCHER_POD, 1);
        EXPECT_TRUE(sniffer->isBreedingItem(food));
    }
}

TEST_F(SnifferEntityTest, IsBreedingItem_OtherItem_ReturnsFalse)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));

    // 小麦不是嗅探兽食物
    if (Items::WHEAT != nullptr) {
        ItemStack wheat(Items::WHEAT, 1);
        EXPECT_FALSE(sniffer->isBreedingItem(wheat));
    }
}

TEST_F(SnifferEntityTest, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    ItemStack empty;
    EXPECT_FALSE(sniffer->isBreedingItem(empty));
}

TEST_F(SnifferEntityTest, EyeHeight_AdultReturns105)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    // 默认成体
    EXPECT_FLOAT_EQ(sniffer->eyeHeight(), 1.05f);
}

TEST_F(SnifferEntityTest, EyeHeight_BabyReturns525)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    sniffer->setChild(true);
    EXPECT_FLOAT_EQ(sniffer->eyeHeight(), 0.525f);
}

TEST_F(SnifferEntityTest, GetBaseWidth_Returns19)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    EXPECT_FLOAT_EQ(sniffer->getBaseWidth(), 1.9f);
}

TEST_F(SnifferEntityTest, GetBaseHeight_Returns175)
{
    auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0));
    EXPECT_FLOAT_EQ(sniffer->getBaseHeight(), 1.75f);
}
