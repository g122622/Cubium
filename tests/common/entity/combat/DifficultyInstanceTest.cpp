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

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace mc::entity::combat;
using namespace mc;

// ============================================================================
// 测试用 IWorld 桩实现
// ============================================================================

class DifficultyTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override
    {
        auto it = m_chunks.find(ChunkPos(x, z));
        return it != m_chunks.end() ? it->second.get() : nullptr;
    }
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override
    {
        return m_chunks.find(ChunkPos(x, z)) != m_chunks.end();
    }
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
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return m_gameTime; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DifficultyTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DifficultyTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // 测试辅助方法
    void setGameTime(u64 time) { m_gameTime = time; }
    void setDayTime(i64 time) { m_dayTime = time; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    ChunkData& ensureChunk(ChunkCoord x, ChunkCoord z)
    {
        ChunkPos pos(x, z);
        auto it = m_chunks.find(pos);
        if (it == m_chunks.end()) {
            it = m_chunks.emplace(pos, std::make_unique<ChunkData>(x, z)).first;
        }
        return *it->second;
    }

private:
    u64 m_gameTime = 0;
    i64 m_dayTime = 0;
    Difficulty m_difficulty = Difficulty::Normal;
    mutable math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
    world::gamerule::GameRules m_gameRules;
    std::unordered_map<ChunkPos, std::unique_ptr<ChunkData>> m_chunks;
};

// ============================================================================
// DifficultyInstance 简化构造函数测试
// ============================================================================

TEST(DifficultyInstanceTest, SimplifiedConstructor_Peaceful)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Easy)
{
    DifficultyInstance inst(Difficulty::Easy);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Easy);
    // effective = base * id = 0.75 * 1 = 0.75
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.75f);
    // 0.75 < 2.0 -> special = 0.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Normal)
{
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Normal);
    // effective = base * id = 1.0 * 2 = 2.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
    // 2.0 不小于 2.0，但 (2.0 - 2.0) / 2.0 = 0.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Hard)
{
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Hard);
    // effective = base * id = 1.0 * 3 = 3.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
    // (3.0 - 2.0) / 2.0 = 0.5
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.5f);
}

// ============================================================================
// DifficultyInstance 完整构造函数测试
// ============================================================================

TEST(DifficultyInstanceTest, FullConstructor_PeacefulAlwaysZero)
{
    // Peaceful 难度下无论时间多长，有效难度始终为 0
    DifficultyInstance inst(Difficulty::Peaceful, 1000000, 1000000, 1.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_PeacefulZeroTime)
{
    DifficultyInstance inst(Difficulty::Peaceful, 0, 0, 0.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_LongWorldTimeIncreasesDifficulty)
{
    // 同一难度下，世界时间越长有效难度越高
    DifficultyInstance instEarly(Difficulty::Normal, 0, 0, 0.0f);
    DifficultyInstance instLate(Difficulty::Normal, 1440000, 0, 0.0f);
    EXPECT_GT(instLate.getEffectiveDifficulty(), instEarly.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_HighChunkInhabitedTimeIncreasesDifficulty)
{
    // 高区块居住时间增加难度
    DifficultyInstance instLow(Difficulty::Normal, 0, 0, 0.0f);
    DifficultyInstance instHigh(Difficulty::Normal, 0, 3600000, 0.0f);
    EXPECT_GT(instHigh.getEffectiveDifficulty(), instLow.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_EasyChunkFactorHalved)
{
    // Easy 难度下区块因子减半
    // 使用相同的非零时间参数，Easy 的有效难度增长比 Normal 慢
    // 因为 chunkFactor 在 Easy 下乘以 0.5
    DifficultyInstance instEasy(Difficulty::Easy, 1440000, 3600000, 1.0f);
    DifficultyInstance instNormal(Difficulty::Normal, 1440000, 3600000, 1.0f);

    // 两者都应有非零有效难度，但 Normal 应更高
    EXPECT_GT(instEasy.getEffectiveDifficulty(), 0.0f);
    EXPECT_GT(instNormal.getEffectiveDifficulty(), instEasy.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_MoonPhaseAddsToChunkFactor)
{
    // 月相因子增加难度（需要足够的世界时间才能生效）
    // worldTime=72000 -> timeGlobalFactor=0, 月相被夹到0，无效果
    // 使用更大的 worldTime 使月相因子生效
    DifficultyInstance instNoMoon(Difficulty::Normal, 720000, 0, 0.0f);
    DifficultyInstance instFullMoon(Difficulty::Normal, 720000, 0, 1.0f);
    EXPECT_GT(instFullMoon.getEffectiveDifficulty(), instNoMoon.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_HardUsesFullChunkFactor)
{
    // Hard 难度使用完整的区块因子（1.0），而不是像 Easy/Normal 那样减半（0.75）
    DifficultyInstance instHard(Difficulty::Hard, 1440000, 3600000, 1.0f);
    DifficultyInstance instNormal(Difficulty::Normal, 1440000, 3600000, 1.0f);

    // Hard 的区块因子系数为 1.0，Normal 为 0.75
    EXPECT_GT(instHard.getEffectiveDifficulty(), instNormal.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_AllMaxFactors)
{
    // 所有因子拉满：最大世界时间 + 最大区块居住时间 + 满月
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);

    // timeGlobalFactor = clamp((1440000 - 72000) / 1440000, 0, 1) = clamp(0.95, 0, 1) = 0.95
    // f = 0.75 + 0.95 * 0.25 = 0.75 + 0.2375 = 0.9875
    // chunkFactor = 1.0 * 1.0 = 1.0 (Hard)
    // moonFactor = clamp(1.0 * 0.25, 0, 0.95) = 0.25
    // chunkFactor += 0.25 = 1.25
    // f += 1.25 = 2.2375
    // effective = 3 * 2.2375 = 6.7125
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 6.7125f);
    // 6.7125 > 4.0 -> special = 1.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_ZeroTimeZeroChunk)
{
    // 初始状态：世界时间0、区块居住时间0、月相0
    DifficultyInstance inst(Difficulty::Normal, 0, 0, 0.0f);

    // timeGlobalFactor: clamp((-72000 + 0) / 1440000, 0, 1) = clamp(-0.05, 0, 1) = 0
    // chunkFactor: clamp(0 / 3600000, 0, 1) * 0.75 = 0
    // moonFactor: clamp(0 * 0.25, 0, 0) = 0
    // f = 0.75 + 0 * 0.25 + 0 + 0 = 0.75
    // effective = 2 * 0.75 = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

// ============================================================================
// getSpecialMultiplier() 边界测试
// ============================================================================

TEST(DifficultyInstanceTest, SpecialMultiplier_BelowTwoReturnsZero)
{
    // effectiveDifficulty < 2.0 -> 0.0
    DifficultyInstance inst(Difficulty::Easy, 0, 0, 0.0f);
    // Easy: effective = 0.75 * 1 = 0.75 < 2.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_AboveFourReturnsOne)
{
    // effectiveDifficulty > 4.0 -> 1.0
    // 需要 effective > 4.0，使用 Hard + 充足时间和区块居住时间
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);
    EXPECT_GT(inst.getEffectiveDifficulty(), 4.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_ExactlyThreeReturnsHalf)
{
    // 简化构造 Hard: effective = 3.0
    // (3.0 - 2.0) / 2.0 = 0.5
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.5f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_ExactlyTwoReturnsZero)
{
    // 简化构造 Normal: effective = 2.0
    // 2.0 不小于 2.0，但 (2.0 - 2.0) / 2.0 = 0.0
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

// ============================================================================
// isHard() 测试
// ============================================================================

TEST(DifficultyInstanceTest, IsHard_EffectiveBelowThree)
{
    // Peaceful 和 Easy 的有效难度 < 3.0
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FALSE(peaceful.isHard());

    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FALSE(easy.isHard());
}

TEST(DifficultyInstanceTest, IsHard_NormalIsNotHard)
{
    // Normal 简化构造: effective = 2.0 < 3.0
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_FALSE(inst.isHard());
}

TEST(DifficultyInstanceTest, IsHard_HardSimplifiedIsHard)
{
    // Hard 简化构造: effective = 3.0 >= 3.0
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_TRUE(inst.isHard());
}

TEST(DifficultyInstanceTest, IsHard_BoundaryAtThree)
{
    // 当 effectiveDifficulty >= 3.0 时返回 true
    // 使用完整构造函数让 effective 恰好 >= 3.0
    // Hard + 部分时间因子
    DifficultyInstance inst(Difficulty::Hard, 72000, 0, 0.0f);
    // 如果 effective >= 3.0 则 isHard() = true
    // timeGlobalFactor: clamp((-72000 + 72000) / 1440000, 0, 1) = clamp(0, 0, 1) = 0
    // chunkFactor = 0, moonFactor = 0
    // f = 0.75 + 0 + 0 + 0 = 0.75
    // effective = 3 * 0.75 = 2.25 < 3.0 -> NOT hard
    EXPECT_FALSE(inst.isHard());
}

// ============================================================================
// isHarderThan() 测试
// ============================================================================

TEST(DifficultyInstanceTest, IsHarderThan_PeacefulNotHarderThanZero)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    // effective = 0.0, 0.0 > 0.0 is false
    EXPECT_FALSE(inst.isHarderThan(0.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_EasyHarderThanZero)
{
    DifficultyInstance inst(Difficulty::Easy);
    // effective = 0.75 > 0.0
    EXPECT_TRUE(inst.isHarderThan(0.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_EasyNotHarderThanOne)
{
    DifficultyInstance inst(Difficulty::Easy);
    // effective = 0.75, 0.75 > 1.0 is false
    EXPECT_FALSE(inst.isHarderThan(1.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_NormalHarderThanEasy)
{
    DifficultyInstance normal(Difficulty::Normal);
    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_TRUE(normal.isHarderThan(easy.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_HardHarderThanNormal)
{
    DifficultyInstance hard(Difficulty::Hard);
    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_TRUE(hard.isHarderThan(normal.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_NotHarderThanSelf)
{
    DifficultyInstance inst(Difficulty::Normal);
    // effective = 2.0, 2.0 > 2.0 is false (strictly greater)
    EXPECT_FALSE(inst.isHarderThan(inst.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_NegativeThreshold)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    // effective = 0.0, 0.0 > -1.0 is true
    EXPECT_TRUE(inst.isHarderThan(-1.0f));
}

// ============================================================================
// calculateDifficulty() 边界条件测试
// ============================================================================

TEST(DifficultyInstanceTest, Calculate_PeacefulIgnoresAllFactors)
{
    // 即使所有因子拉满，Peaceful 始终为 0
    DifficultyInstance zero(Difficulty::Peaceful, 0, 0, 0.0f);
    DifficultyInstance max(Difficulty::Peaceful, 1440000, 3600000, 1.0f);
    EXPECT_FLOAT_EQ(zero.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(max.getEffectiveDifficulty(), 0.0f);
}

TEST(DifficultyInstanceTest, Calculate_NegativeWorldTimeBeforeOffset)
{
    // worldTime < 72000 时，timeGlobalFactor = clamp((-72000 + worldTime) / 1440000, 0, 1) = 0
    // 因为 -72000 + worldTime < 0
    DifficultyInstance inst(Difficulty::Normal, 0, 0, 0.0f);
    // f = 0.75 + 0 + 0 + 0 = 0.75, effective = 2 * 0.75 = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_ExactlyAtTimeGlobalOffset)
{
    // worldTime = 72000 -> (-72000 + 72000) / 1440000 = 0.0
    DifficultyInstance inst(Difficulty::Normal, 72000, 0, 0.0f);
    // timeGlobalFactor = 0, f = 0.75, effective = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_MidWorldTime)
{
    // worldTime = 720000 -> (-72000 + 720000) / 1440000 = 648000 / 1440000 = 0.45
    DifficultyInstance inst(Difficulty::Normal, 720000, 0, 0.0f);
    // timeGlobalFactor = 0.45, f = 0.75 + 0.45 * 0.25 + 0 + 0 = 0.75 + 0.1125 = 0.8625
    // effective = 2 * 0.8625 = 1.725
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.725f);
}

TEST(DifficultyInstanceTest, Calculate_MaxWorldTimeClampsToOne)
{
    // worldTime 远超 MAX_DIFFICULTY_TIME_GLOBAL 时，timeGlobalFactor 被夹到 1.0
    DifficultyInstance inst(Difficulty::Normal, 9999999, 0, 0.0f);
    // timeGlobalFactor = clamp((-72000 + 9999999) / 1440000, 0, 1) = 1.0
    // f = 0.75 + 1.0 * 0.25 + 0 + 0 = 1.0
    // effective = 2 * 1.0 = 2.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
}

TEST(DifficultyInstanceTest, Calculate_MaxChunkInhabitedTime)
{
    // chunkInhabitedTime = MAX_DIFFICULTY_TIME_LOCAL = 3600000
    // Normal: chunkFactor = clamp(1.0, 0, 1) * 0.75 = 0.75
    DifficultyInstance inst(Difficulty::Normal, 0, 3600000, 0.0f);
    // timeGlobalFactor = 0, chunkFactor = 0.75, moonFactor = 0
    // f = 0.75 + 0 + 0.75 + 0 = 1.5
    // effective = 2 * 1.5 = 3.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
}

TEST(DifficultyInstanceTest, Calculate_ChunkTimeExceedsMaxClamped)
{
    // chunkInhabitedTime 远超 MAX_DIFFICULTY_TIME_LOCAL，被夹到 1.0
    DifficultyInstance inst(Difficulty::Normal, 0, 9999999, 0.0f);
    // 和最大值的结果相同
    DifficultyInstance instMax(Difficulty::Normal, 0, 3600000, 0.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), instMax.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseClampedByTimeGlobalFactor)
{
    // 月相因子 = clamp(moonPhaseFactor * 0.25, 0, timeGlobalFactor)
    // 当 timeGlobalFactor = 0 时，月相因子也为 0
    DifficultyInstance noTime(Difficulty::Normal, 0, 0, 1.0f);
    // timeGlobalFactor = 0, moonFactor = clamp(0.25, 0, 0) = 0
    // f = 0.75 + 0 + 0 + 0 = 0.75, effective = 1.5
    EXPECT_FLOAT_EQ(noTime.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseWithNonZeroTimeGlobalFactor)
{
    // worldTime = 720000 -> timeGlobalFactor = 0.45
    // moonPhaseFactor = 1.0 -> moonFactor = clamp(0.25, 0, 0.45) = 0.25
    DifficultyInstance inst(Difficulty::Normal, 720000, 0, 1.0f);
    // timeGlobalFactor = 0.45
    // chunkFactor = 0 (no chunk time)
    // moonFactor = 0.25
    // chunkFactor + moonFactor = 0 + 0.25 = 0.25
    // f = 0.75 + 0.45 * 0.25 + 0.25 = 0.75 + 0.1125 + 0.25 = 1.1125
    // effective = 2 * 1.1125 = 2.225
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.225f);
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseClampedWhenExceedsTimeGlobalFactor)
{
    // timeGlobalFactor = 0.1 (small), moonPhaseFactor = 1.0
    // moonFactor = clamp(0.25, 0, 0.1) = 0.1
    // 计算 worldTime 使得 timeGlobalFactor = 0.1
    // (-72000 + worldTime) / 1440000 = 0.1 -> worldTime = 72000 + 144000 = 216000
    DifficultyInstance inst(Difficulty::Normal, 216000, 0, 1.0f);
    // timeGlobalFactor = 0.1
    // chunkFactor = 0, moonFactor = clamp(0.25, 0, 0.1) = 0.1
    // chunkFactor + moonFactor = 0.1
    // f = 0.75 + 0.1 * 0.25 + 0.1 = 0.75 + 0.025 + 0.1 = 0.875
    // effective = 2 * 0.875 = 1.75
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.75f);
}

TEST(DifficultyInstanceTest, Calculate_EasyDifficultyChunkFactorHalved)
{
    // Easy 下 chunkFactor *= 0.5
    // worldTime = 1440000 -> timeGlobalFactor = (1440000 - 72000) / 1440000 = 0.95
    // chunkInhabitedTime = 3600000 (max chunk factor = 1.0)
    // moonPhaseFactor = 1.0
    DifficultyInstance inst(Difficulty::Easy, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 0.75 = 0.75 (Easy uses 0.75 for non-Hard)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 0.75 + 0.25 = 1.0
    // Easy: chunkFactor *= 0.5 -> 0.5
    // f = 0.75 + 0.95 * 0.25 + 0.5 = 0.75 + 0.2375 + 0.5 = 1.4875
    // effective = 1 * 1.4875 = 1.4875
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 1.4875f, 0.001f);
}

TEST(DifficultyInstanceTest, Calculate_NormalDifficultyFullFactors)
{
    DifficultyInstance inst(Difficulty::Normal, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 0.75 = 0.75 (Normal uses 0.75 for non-Hard)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 0.75 + 0.25 = 1.0
    // Normal: chunkFactor NOT halved
    // f = 0.75 + 0.95 * 0.25 + 1.0 = 0.75 + 0.2375 + 1.0 = 1.9875
    // effective = 2 * 1.9875 = 3.975
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 3.975f, 0.001f);
    // effective = 3.975 >= 2.0, (3.975 - 2.0) / 2.0 = 0.9875
    EXPECT_NEAR(inst.getSpecialMultiplier(), 0.9875f, 0.001f);
}

TEST(DifficultyInstanceTest, Calculate_HardDifficultyFullFactors)
{
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 1.0 = 1.0 (Hard uses 1.0)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 1.0 + 0.25 = 1.25
    // Hard: chunkFactor NOT halved
    // f = 0.75 + 0.95 * 0.25 + 1.25 = 0.75 + 0.2375 + 1.25 = 2.2375
    // effective = 3 * 2.2375 = 6.7125
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 6.7125f, 0.001f);
    // 6.7125 > 4.0 -> special = 1.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

// ============================================================================
// DifficultyInstance::at() 工厂方法测试
// ============================================================================

TEST(DifficultyInstanceAtTest, At_BasicConstructionMatchesFullConstructor)
{
    // at() 工厂方法的结果应与手动构造的完整参数版本一致
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Hard);
    world.setGameTime(1440000);
    world.setDayTime(60000); // 第2.5天，月相 = (60000/24000) % 8 = 1（亏凸月，0.75）

    BlockPos pos(0, 64, 0);
    auto& chunk = world.ensureChunk(0, 0);
    chunk.setInhabitedTime(3600000);

    DifficultyInstance fromAt = DifficultyInstance::at(world, pos);
    DifficultyInstance fromConstructor(world.difficulty(),
        static_cast<i64>(world.getGameTime()),
        chunk.inhabitedTime(),
        InternalLightUtils::getMoonBrightness(InternalLightUtils::getMoonPhase(world.dayTime())));

    EXPECT_FLOAT_EQ(fromAt.getEffectiveDifficulty(), fromConstructor.getEffectiveDifficulty());
    EXPECT_FLOAT_EQ(fromAt.getSpecialMultiplier(), fromConstructor.getSpecialMultiplier());
    EXPECT_EQ(fromAt.getDifficulty(), fromConstructor.getDifficulty());
}

TEST(DifficultyInstanceAtTest, At_PeacefulAlwaysZero)
{
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Peaceful);
    world.setGameTime(1440000);
    world.setDayTime(60000);

    DifficultyInstance inst = DifficultyInstance::at(world, BlockPos(0, 64, 0));
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceAtTest, At_NoChunkDefaultsInhabitedTimeToZero)
{
    // 当区块不存在时，inhabitedTime 默认为 0
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setGameTime(720000);
    world.setDayTime(60000);
    // 不添加任何区块 -> getChunk 返回 nullptr -> inhabitedTime = 0

    DifficultyInstance noChunk = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 与手动构造 zero inhabitedTime 对比
    const i32 moonPhase = InternalLightUtils::getMoonPhase(world.dayTime());
    const f32 moonBrightness = InternalLightUtils::getMoonBrightness(moonPhase);
    DifficultyInstance expected(Difficulty::Normal, static_cast<i64>(world.getGameTime()), 0, moonBrightness);

    EXPECT_FLOAT_EQ(noChunk.getEffectiveDifficulty(), expected.getEffectiveDifficulty());
    EXPECT_FLOAT_EQ(noChunk.getSpecialMultiplier(), expected.getSpecialMultiplier());
}

TEST(DifficultyInstanceAtTest, At_ChunkInhabitedTimeIncreasesDifficulty)
{
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setGameTime(720000);
    world.setDayTime(60000);

    // 无区块 -> inhabitedTime = 0
    DifficultyInstance noInhabit = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 有区块且 inhabitedTime 很高
    auto& chunk = world.ensureChunk(0, 0);
    chunk.setInhabitedTime(3600000);
    DifficultyInstance highInhabit = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 高居住时间应有更高的有效难度
    EXPECT_GT(highInhabit.getEffectiveDifficulty(), noInhabit.getEffectiveDifficulty());
}

TEST(DifficultyInstanceAtTest, At_DifferentMoonPhasesAffectDifficulty)
{
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setGameTime(720000);

    // dayTime = 0 -> 满月(月相0)，brightness = 1.0
    world.setDayTime(0);
    DifficultyInstance fullMoon = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // dayTime = 96000 -> 新月(月相4)，brightness = 0.0
    // (96000 / 24000 = 4, 月相4 = 新月)
    world.setDayTime(96000);
    DifficultyInstance newMoon = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 满月应增加难度
    EXPECT_GT(fullMoon.getEffectiveDifficulty(), newMoon.getEffectiveDifficulty());
}

TEST(DifficultyInstanceAtTest, At_EarlyWorldTimeNoMoonEffect)
{
    // 世界时间 < 72000 时，timeGlobalFactor = 0
    // moonPhaseFactor 被夹到 [0, 0]，月相不影响难度
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setGameTime(0);

    // dayTime = 0 -> 满月
    world.setDayTime(0);
    DifficultyInstance fullMoon = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // dayTime = 96000 -> 新月
    world.setDayTime(96000);
    DifficultyInstance newMoon = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 世界时间0时，月相应不影响难度
    EXPECT_FLOAT_EQ(fullMoon.getEffectiveDifficulty(), newMoon.getEffectiveDifficulty());
}

TEST(DifficultyInstanceAtTest, At_LongWorldTimeIncreasesDifficulty)
{
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setDayTime(0);

    // 世界初期
    world.setGameTime(0);
    DifficultyInstance early = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 世界运行很久后
    world.setGameTime(1440000);
    DifficultyInstance late = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 长时间运行应有更高难度
    EXPECT_GT(late.getEffectiveDifficulty(), early.getEffectiveDifficulty());
}

TEST(DifficultyInstanceAtTest, At_HardDifficultyWithAllFactors)
{
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Hard);
    world.setGameTime(1440000);
    world.setDayTime(0); // 满月，brightness = 1.0

    auto& chunk = world.ensureChunk(0, 0);
    chunk.setInhabitedTime(3600000);

    DifficultyInstance inst = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // effective = 3 * 2.2375 = 6.7125 (与 FullConstructor_AllMaxFactors 一致)
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 6.7125f, 0.001f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

TEST(DifficultyInstanceAtTest, At_SimplifiedConstructorDiffersFromZeroTimeAt)
{
    // 简化构造函数和 at() 在零时刻的结果不同
    // 简化构造假设时间足够久（base=1.0），at() 在零时刻实际计算 timeGlobalFactor=0
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Hard);
    world.setGameTime(0);
    world.setDayTime(96000); // 月相4 -> brightness=0.0

    DifficultyInstance fromAt = DifficultyInstance::at(world, BlockPos(0, 64, 0));
    DifficultyInstance simplified(Difficulty::Hard);

    // 简化构造: effective = 1.0 * 3 = 3.0
    // at() 在零时刻: effective = 3 * 0.75 = 2.25
    EXPECT_NE(fromAt.getEffectiveDifficulty(), simplified.getEffectiveDifficulty());
    EXPECT_FLOAT_EQ(fromAt.getEffectiveDifficulty(), 2.25f);
    EXPECT_FLOAT_EQ(simplified.getEffectiveDifficulty(), 3.0f);
}

TEST(DifficultyInstanceAtTest, At_OffsetChunkCoordinates)
{
    // 测试非零区块坐标
    DifficultyTestWorld world;
    world.setDifficulty(Difficulty::Normal);
    world.setGameTime(720000);
    world.setDayTime(0);

    // 区块 (5, 3) 居住时间长
    auto& chunk53 = world.ensureChunk(5, 3);
    chunk53.setInhabitedTime(2000000);

    // 区块 (0, 0) 居住时间短
    auto& chunk00 = world.ensureChunk(0, 0);
    chunk00.setInhabitedTime(100000);

    // BlockPos(80, 64, 48) -> 区块 (5, 3)
    DifficultyInstance at53 = DifficultyInstance::at(world, BlockPos(80, 64, 48));
    // BlockPos(0, 64, 0) -> 区块 (0, 0)
    DifficultyInstance at00 = DifficultyInstance::at(world, BlockPos(0, 64, 0));

    // 区块 (5,3) 居住时间更长，难度更高
    EXPECT_GT(at53.getEffectiveDifficulty(), at00.getEffectiveDifficulty());
}
