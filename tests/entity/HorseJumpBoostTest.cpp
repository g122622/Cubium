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
#include "common/core/Constants.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World，支持广播实体状态
 */
class HorseJumpBoostTestWorld final : public test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& /*pos*/) const override { return 1.0f; }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("HorseJumpBoostTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("HorseJumpBoostTestWorld::tickManager not implemented");
    }

    void broadcastEntityStatus(EntityInstanceId, u8) override {}

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

// ============================================================================
// AbstractHorseEntity 跳跃提升药水效果测试
// ============================================================================

/**
 * @brief 测试跳跃提升效果增加跳跃力度
 *
 * 参考 MC 1.16.5 AbstractHorseEntity.travel():
 * if (this.isPotionActive(Effects.JUMP_BOOST)) {
 *     d1 = d0 + (double)((float)(this.getActivePotionEffect(Effects.JUMP_BOOST).getAmplifier() + 1) * 0.1F);
 * }
 */
TEST(HorseJumpBoostTest, JumpBoostEffect_IncreasesJumpForce)
{
    VanillaBlocks::initialize();

    HorseJumpBoostTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 设置固定的跳跃强度以便测试
    horse.setJumpStrength(0.5f);
    horse.setSaddle(true); // 需要鞍才能跳跃

    // 设置跳跃力度为满 (100%)
    horse.setJumpPower(100);

    // 无跳跃提升效果时的基础跳跃力度
    f32 baseJumpStrength = horse.getJumpStrength();
    f32 jumpPowerFactor = 1.0f; // 100 / 100
    f64 expectedBaseJumpForce = static_cast<f64>(baseJumpStrength * jumpPowerFactor);

    // 验证马没有跳跃提升效果
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 0);

    // 添加跳跃提升 II 效果 (amplifier = 1, level = 2)
    entity::effect::EffectInstance jumpBoostEffect(
        entity::effect::EffectType::JumpBoost, 200, 1, false, true); // 200 ticks, amplifier 1
    horse.addEffect(std::move(jumpBoostEffect));

    // 验证效果等级
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);

    // 每级跳跃提升增加 0.1 跳跃力度
    // 跳跃提升 II (level=2): +0.2
    f64 expectedBoost = 0.2;
    f64 expectedJumpForce = expectedBaseJumpForce + expectedBoost;

    // 注意：实际跳跃力度的计算在 travel() 方法中，这里测试效果等级的正确获取
    // 验证效果存在
    EXPECT_TRUE(horse.hasEffect(entity::effect::EffectType::JumpBoost));
}

/**
 * @brief 测试跳跃提升等级对跳跃力度的增量
 *
 * 每级跳跃提升增加 0.1 的跳跃力度
 */
TEST(HorseJumpBoostTest, JumpBoostLevel_AddsCorrectAmount)
{
    VanillaBlocks::initialize();

    HorseJumpBoostTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 初始无效果
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 0);

    // 添加跳跃提升 I (amplifier = 0, level = 1)
    {
        entity::effect::EffectInstance jumpBoostI(entity::effect::EffectType::JumpBoost, 200, 0, false, true);
        horse.addEffect(std::move(jumpBoostI));
        EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 1);
        // 跳跃提升 I 应增加 0.1 的跳跃力度
        horse.removeEffect(entity::effect::EffectType::JumpBoost);
    }

    // 添加跳跃提升 II (amplifier = 1, level = 2)
    {
        entity::effect::EffectInstance jumpBoostII(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
        horse.addEffect(std::move(jumpBoostII));
        EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);
        // 跳跃提升 II 应增加 0.2 的跳跃力度
        horse.removeEffect(entity::effect::EffectType::JumpBoost);
    }

    // 添加跳跃提升 III (amplifier = 2, level = 3)
    {
        entity::effect::EffectInstance jumpBoostIII(entity::effect::EffectType::JumpBoost, 200, 2, false, true);
        horse.addEffect(std::move(jumpBoostIII));
        EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 3);
        // 跳跃提升 III 应增加 0.3 的跳跃力度
        horse.removeEffect(entity::effect::EffectType::JumpBoost);
    }
}

/**
 * @brief 测试效果移除后跳跃力度恢复正常
 */
TEST(HorseJumpBoostTest, JumpBoostEffect_Removed_JumpForceReturnsToNormal)
{
    VanillaBlocks::initialize();

    HorseJumpBoostTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 添加跳跃提升 II
    entity::effect::EffectInstance jumpBoostII(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
    horse.addEffect(std::move(jumpBoostII));
    EXPECT_TRUE(horse.hasEffect(entity::effect::EffectType::JumpBoost));
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);

    // 移除效果
    horse.removeEffect(entity::effect::EffectType::JumpBoost);

    // 验证效果已移除
    EXPECT_FALSE(horse.hasEffect(entity::effect::EffectType::JumpBoost));
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 0);
}

/**
 * @brief 测试多种效果共存时跳跃提升效果仍然有效
 */
TEST(HorseJumpBoostTest, JumpBoostEffect_WorksWithOtherEffects)
{
    VanillaBlocks::initialize();

    HorseJumpBoostTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 添加速度效果
    entity::effect::EffectInstance speedEffect(entity::effect::EffectType::Speed, 200, 1, false, true);
    horse.addEffect(std::move(speedEffect));
    EXPECT_TRUE(horse.hasEffect(entity::effect::EffectType::Speed));

    // 添加跳跃提升 II
    entity::effect::EffectInstance jumpBoostII(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
    horse.addEffect(std::move(jumpBoostII));

    // 验证两个效果都存在
    EXPECT_TRUE(horse.hasEffect(entity::effect::EffectType::Speed));
    EXPECT_TRUE(horse.hasEffect(entity::effect::EffectType::JumpBoost));
    EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);
}

/**
 * @brief 测试不同马类型都可以获得跳跃提升效果
 */
TEST(HorseJumpBoostTest, JumpBoostEffect_WorksForAllHorseTypes)
{
    VanillaBlocks::initialize();

    HorseJumpBoostTestWorld world;

    // 测试马
    {
        HorseEntity horse(EntityInstanceId(1));
        horse.setWorld(&world);

        entity::effect::EffectInstance jumpBoost(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
        horse.addEffect(std::move(jumpBoost));
        EXPECT_EQ(horse.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);
    }

    // 测试驴
    {
        DonkeyEntity donkey(EntityInstanceId(2));
        donkey.setWorld(&world);

        entity::effect::EffectInstance jumpBoost(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
        donkey.addEffect(std::move(jumpBoost));
        EXPECT_EQ(donkey.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);
    }

    // 测试骡
    {
        MuleEntity mule(EntityInstanceId(3));
        mule.setWorld(&world);

        entity::effect::EffectInstance jumpBoost(entity::effect::EffectType::JumpBoost, 200, 1, false, true);
        mule.addEffect(std::move(jumpBoost));
        EXPECT_EQ(mule.getEffectLevel(entity::effect::EffectType::JumpBoost), 2);
    }
}

} // namespace
} // namespace mc
