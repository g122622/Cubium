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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include "sound/SoundCategory.hpp"
#include "util/math/Vector3.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/special/SpongeBlock.hpp"
#include "world/block/blocks/special/WetSpongeBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用世界模拟器
 *
 * 提供最小化的 IWorld 实现，用于测试海绵吸水功能。
 */
class SpongeTestWorld final : public test::BaseTestWorld {
public:
    SpongeTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    void setBlockDirectly(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[packPos(pos.x, pos.y, pos.z)] = state;
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state)
    {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    void setUltraWarm(bool ultraWarm) { m_ultraWarm = ultraWarm; }

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
        m_blocks[packPos(x, y, z)] = state;
        // 记录方块变化
        m_blockChanges.push_back({BlockPos(x, y, z), state});
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // First check if there's a fluid at this position
        const auto fluidIt = m_fluids.find(packPos(x, y, z));
        if (fluidIt != m_fluids.end() && fluidIt->second != nullptr) {
            return fluidIt->second;
        }

        // Otherwise check block's fluid state
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }

        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] DimensionId dimension() const override { return m_ultraWarm ? DimensionId(-1) : DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_UNUSED(entity);
        return 0;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<SpongeTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    // 记录的方法
    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_playedEvents.push_back({eventId, pos, data});
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_playedSounds.push_back({soundId, category, position, volume, pitch});
    }

    // 访问记录的事件
    const std::vector<std::tuple<i32, BlockPos, i32>>& getPlayedEvents() const { return m_playedEvents; }
    const std::vector<std::tuple<ResourceLocation, sound::SoundCategory, Vector3, f32, f32>>& getPlayedSounds() const
    {
        return m_playedSounds;
    }
    const std::vector<std::pair<BlockPos, const BlockState*>>& getBlockChanges() const { return m_blockChanges; }

    void clearRecords()
    {
        m_playedEvents.clear();
        m_playedSounds.clear();
        m_blockChanges.clear();
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    u64 m_seed = 12345;
    bool m_ultraWarm = false;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    // 记录的事件
    std::vector<std::tuple<i32, BlockPos, i32>> m_playedEvents;
    std::vector<std::tuple<ResourceLocation, sound::SoundCategory, Vector3, f32, f32>> m_playedSounds;
    std::vector<std::pair<BlockPos, const BlockState*>> m_blockChanges;
};

} // namespace

// ========== SpongeBlock Tests ==========

class SpongeBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(SpongeBlockTest, SpongeBlockExists)
{
    ASSERT_NE(VanillaBlocks::SPONGE, nullptr);
    ASSERT_NE(VanillaBlocks::WET_SPONGE, nullptr);
}

TEST_F(SpongeBlockTest, SpongeBlockIsSpongeBlockClass)
{
    // 验证 SpongeBlock 已注册
    const Block& sponge = *VanillaBlocks::SPONGE;
    const BlockState& state = sponge.defaultState();

    // 检查材质
    EXPECT_EQ(&state.getMaterial(), &Material::SPONGE);
    EXPECT_FLOAT_EQ(state.hardness(), 0.6f);
}

TEST_F(SpongeBlockTest, WetSpongeBlockIsWetSpongeBlockClass)
{
    // 验证 WetSpongeBlock 已注册
    const Block& wetSponge = *VanillaBlocks::WET_SPONGE;
    const BlockState& state = wetSponge.defaultState();

    // 检查材质
    EXPECT_EQ(&state.getMaterial(), &Material::SPONGE);
    EXPECT_FLOAT_EQ(state.hardness(), 0.6f);
}

TEST_F(SpongeBlockTest, SpongeBlockDynamicCast)
{
    // 验证 SpongeBlock 可以正确 dynamic_cast
    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    EXPECT_NE(sponge, nullptr);
}

TEST_F(SpongeBlockTest, WetSpongeBlockDynamicCast)
{
    // 验证 WetSpongeBlock 可以正确 dynamic_cast
    WetSpongeBlock* wetSponge = dynamic_cast<WetSpongeBlock*>(VanillaBlocks::WET_SPONGE);
    EXPECT_NE(wetSponge, nullptr);
}

TEST_F(SpongeBlockTest, SpongeDoesNotAbsorbWithoutWater)
{
    SpongeTestWorld world;

    // 放置海绵
    BlockPos spongePos(0, 0, 0);
    const BlockState& spongeState = VanillaBlocks::SPONGE->defaultState();
    world.setBlockDirectly(spongePos, &spongeState);

    // 周围没有水

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);

    bool absorbed = sponge->tryAbsorbWater(world, spongePos);

    // 验证吸水失败
    EXPECT_FALSE(absorbed);

    // 验证海绵仍然是干海绵
    const BlockState* finalState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::SPONGE);
}

TEST_F(SpongeBlockTest, SpongeAbsorbsWaterSource)
{
    SpongeTestWorld world;

    // 放置海绵
    BlockPos spongePos(0, 0, 0);
    const BlockState& spongeState = VanillaBlocks::SPONGE->defaultState();
    world.setBlockDirectly(spongePos, &spongeState);

    // 在海绵旁边放置水源
    BlockPos waterPos(1, 0, 0);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(waterPos, waterState);

    // 设置水方块
    const BlockState& waterBlockState = VanillaBlocks::WATER->defaultState();
    world.setBlockDirectly(waterPos, &waterBlockState);

    // 获取海绵方块并调用吸水
    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);

    bool absorbed = sponge->tryAbsorbWater(world, spongePos);

    // 验证吸水成功
    EXPECT_TRUE(absorbed);

    // 验证海绵变成湿润海绵
    const BlockState* finalState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WET_SPONGE);

    // 验证播放了事件
    const auto& events = world.getPlayedEvents();
    EXPECT_FALSE(events.empty());
    bool foundBreakEvent = false;
    for (const auto& [eventId, pos, data] : events) {
        if (eventId == world::WorldEvents::BREAK_BLOCK_EFFECTS) {
            foundBreakEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundBreakEvent);
}

TEST_F(SpongeBlockTest, SpongeOnBlockAddedTriggersAbsorb)
{
    SpongeTestWorld world;

    // 放置水源
    BlockPos waterPos(0, 0, 1);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    const BlockState& waterBlockState = VanillaBlocks::WATER->defaultState();
    world.setFluidDirectly(waterPos, waterState);
    world.setBlockDirectly(waterPos, &waterBlockState);

    // 放置海绵（通过 onBlockAdded 触发吸水）
    BlockPos spongePos(0, 0, 0);
    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);

    const BlockState& spongeState = sponge->defaultState();
    world.setBlockDirectly(spongePos, &spongeState);

    // 调用 onBlockAdded
    sponge->onBlockAdded(world, spongePos, spongeState);

    // 验证海绵变成湿润海绵
    const BlockState* finalState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WET_SPONGE);
}

// ========== WetSpongeBlock Tests ==========

class WetSpongeBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(WetSpongeBlockTest, WetSpongeDriesInNether)
{
    SpongeTestWorld world;

    // 设置为超热维度（下界）
    world.setUltraWarm(true);
    EXPECT_TRUE(world.isUltraWarm());

    // 放置湿海绵
    BlockPos spongePos(0, 0, 0);
    WetSpongeBlock* wetSponge = dynamic_cast<WetSpongeBlock*>(VanillaBlocks::WET_SPONGE);
    ASSERT_NE(wetSponge, nullptr);

    const BlockState& wetSpongeState = wetSponge->defaultState();
    world.setBlockDirectly(spongePos, &wetSpongeState);

    // 调用 onBlockAdded
    wetSponge->onBlockAdded(world, spongePos, wetSpongeState);

    // 验证湿海绵变成干海绵
    const BlockState* finalState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::SPONGE);

    // 验证播放了蒸汽效果事件
    const auto& events = world.getPlayedEvents();
    bool foundDryEvent = false;
    for (const auto& [eventId, pos, data] : events) {
        if (eventId == world::WorldEvents::WET_SPONGE_DRY) {
            foundDryEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundDryEvent);

    // 验证播放了音效
    const auto& sounds = world.getPlayedSounds();
    EXPECT_FALSE(sounds.empty());
}

TEST_F(WetSpongeBlockTest, WetSpongeStaysWetInOverworld)
{
    SpongeTestWorld world;

    // 默认是主世界（非超热）
    EXPECT_FALSE(world.isUltraWarm());

    // 放置湿海绵
    BlockPos spongePos(0, 0, 0);
    WetSpongeBlock* wetSponge = dynamic_cast<WetSpongeBlock*>(VanillaBlocks::WET_SPONGE);
    ASSERT_NE(wetSponge, nullptr);

    const BlockState& wetSpongeState = wetSponge->defaultState();
    world.setBlockDirectly(spongePos, &wetSpongeState);

    // 调用 onBlockAdded
    wetSponge->onBlockAdded(world, spongePos, wetSpongeState);

    // 验证湿海绵保持不变
    const BlockState* finalState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WET_SPONGE);

    // 不应该有事件
    const auto& events = world.getPlayedEvents();
    EXPECT_TRUE(events.empty());
}

// ========== WorldEvents Tests ==========

class WorldEventsTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(WorldEventsTest, WetSpongeDryEventValue)
{
    // 验证事件常量值正确
    EXPECT_EQ(world::WorldEvents::WET_SPONGE_DRY, 2009);
}

TEST_F(WorldEventsTest, BreakBlockEffectsValue)
{
    // 验证事件常量值正确
    EXPECT_EQ(world::WorldEvents::BREAK_BLOCK_EFFECTS, 2001);
}
