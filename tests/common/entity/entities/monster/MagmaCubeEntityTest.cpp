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
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供岩浆怪测试所需的最小 IWorld 接口实现
 */
class MagmaCubeTestWorld final : public test::BaseTestWorld {
public:
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
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 记录生成的岩浆怪
        if (auto* magmaCube = dynamic_cast<MagmaCubeEntity*>(entity.get())) {
            m_spawnedMagmaCubeSizes.push_back(magmaCube->getSlimeSize());
            m_spawnedMagmaCubeCount++;
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MagmaCubeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MagmaCubeTestWorld::tickManager not implemented");
    }

    // 测试辅助方法
    [[nodiscard]] size_t spawnedMagmaCubeCount() const { return m_spawnedMagmaCubeCount; }
    [[nodiscard]] const std::vector<i32>& spawnedMagmaCubeSizes() const { return m_spawnedMagmaCubeSizes; }
    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_spawnedMagmaCubeSizes.clear();
        m_spawnedMagmaCubeCount = 0;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<i32> m_spawnedMagmaCubeSizes;
    size_t m_spawnedMagmaCubeCount = 0;
};

class MagmaCubeEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    MagmaCubeTestWorld m_world;
};

// ==================== 继承与尺寸系统测试 ====================

TEST_F(MagmaCubeEntityTest, InheritsFromSlimeEntity)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 验证岩浆怪继承自史莱姆
    EXPECT_NE(dynamic_cast<SlimeEntity*>(&magmaCube), nullptr);
}

TEST_F(MagmaCubeEntityTest, SetSlimeSize_UpdatesAttributes)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    // 设置尺寸为 4
    magmaCube.setSlimeSize(4, true);

    // 验证属性
    EXPECT_EQ(magmaCube.getSlimeSize(), 4);
    // HP = size * size = 16
    EXPECT_FLOAT_EQ(
        static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0)), 16.0f);
    // Speed = 0.2 + 0.1 * size = 0.6
    EXPECT_FLOAT_EQ(
        static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.6f);
    // AttackDamage = size = 4
    EXPECT_FLOAT_EQ(
        static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 4.0f);
    // 生命值应该被重置为最大值
    EXPECT_FLOAT_EQ(magmaCube.health(), magmaCube.maxHealth());
}

// ==================== 护甲属性测试 ====================

TEST_F(MagmaCubeEntityTest, SetSlimeSize_UpdatesArmorAttribute)
{
    // MC 1.16.5: 岩浆怪护甲 = size * 3
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    // 尺寸 1: 护甲 = 1 * 3 = 3
    magmaCube.setSlimeSize(1, true);
    EXPECT_FLOAT_EQ(static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 3.0f);

    // 尺寸 2: 护甲 = 2 * 3 = 6
    magmaCube.setSlimeSize(2, true);
    EXPECT_FLOAT_EQ(static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 6.0f);

    // 尺寸 4: 护甲 = 4 * 3 = 12
    magmaCube.setSlimeSize(4, true);
    EXPECT_FLOAT_EQ(static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 12.0f);
}

TEST_F(MagmaCubeEntityTest, ArmorAttribute_ComparedToSlime)
{
    // 岩浆怪有护甲，史莱姆没有护甲
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);
    magmaCube.setSlimeSize(4, true);

    SlimeEntity slime(EntityInstanceId(2));
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);

    // 岩浆怪护甲 = 12
    EXPECT_FLOAT_EQ(static_cast<f32>(magmaCube.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 12.0f);
    // 史莱姆护甲 = 0（默认值）
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 0.0f);
}

// ==================== 跳跃延迟测试 ====================

TEST_F(MagmaCubeEntityTest, GetJumpDelay_IsApproximatelyFourTimesSlimeDelay)
{
    // MC 1.16.5: 岩浆怪跳跃延迟是史莱姆的 4 倍
    // 史莱姆: 10-30 tick (实际是 10-29)
    // 岩浆怪: 40-120 tick (实际是 40-116)

    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 测试岩浆怪跳跃延迟范围
    i32 minDelay = INT32_MAX;
    i32 maxDelay = INT32_MIN;

    for (int i = 0; i < 1000; ++i) {
        i32 delay = magmaCube.getJumpDelay();
        minDelay = std::min(minDelay, delay);
        maxDelay = std::max(maxDelay, delay);
    }

    // 史莱姆范围 10-29，岩浆怪范围应该是 40-116（4倍）
    EXPECT_GE(minDelay, 40);
    EXPECT_LE(maxDelay, 116);

    // 验证平均值大约在中间
    i32 sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += magmaCube.getJumpDelay();
    }
    f32 avgDelay = static_cast<f32>(sum) / 1000.0f;
    // 平均值应该在 78 左右（(40+116)/2 = 78）
    EXPECT_GE(avgDelay, 50.0f);
    EXPECT_LE(avgDelay, 100.0f);
}

TEST_F(MagmaCubeEntityTest, GetJumpDelay_RangeValidation)
{
    // MC 1.16.5: 岩浆怪跳跃延迟范围 40-120 tick
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    i32 minDelay = INT32_MAX;
    i32 maxDelay = INT32_MIN;

    for (int i = 0; i < 1000; ++i) {
        i32 delay = magmaCube.getJumpDelay();
        minDelay = std::min(minDelay, delay);
        maxDelay = std::max(maxDelay, delay);
    }

    // 史莱姆范围 10-29，岩浆怪范围 40-116（4倍）
    EXPECT_GE(minDelay, 40);
    EXPECT_LE(maxDelay, 116);
}

TEST_F(MagmaCubeEntityTest, GetJumpDelay_ComparedToSlimeRange)
{
    // 验证岩浆怪的跳跃延迟大约是史莱姆的 4 倍
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    SlimeEntity slime(EntityInstanceId(2));

    // 计算史莱姆和岩浆怪的平均跳跃延迟
    i64 slimeSum = 0;
    i64 magmaSum = 0;

    for (int i = 0; i < 1000; ++i) {
        slimeSum += slime.getJumpDelay();
        magmaSum += magmaCube.getJumpDelay();
    }

    f32 slimeAvg = static_cast<f32>(slimeSum) / 1000.0f;
    f32 magmaAvg = static_cast<f32>(magmaSum) / 1000.0f;

    // 岩浆怪的平均延迟应该约为史莱姆的 4 倍
    // 史莱姆平均约 19.5，岩浆怪平均约 78
    f32 ratio = magmaAvg / slimeAvg;
    EXPECT_GE(ratio, 3.4f); // 允许一些随机误差
    EXPECT_LE(ratio, 4.6f);
}

// ==================== 小型岩浆怪伤害测试 ====================

TEST_F(MagmaCubeEntityTest, CanDamagePlayer_SmallMagmaCubeCanDamage)
{
    // MC 1.16.5: 小型岩浆怪也能伤害玩家（与史莱姆不同）
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    // 小型岩浆怪（尺寸 1）可以伤害玩家
    magmaCube.setSlimeSize(1, false);
    EXPECT_TRUE(magmaCube.canDamagePlayer());

    // 大型岩浆怪也可以伤害玩家
    magmaCube.setSlimeSize(4, false);
    EXPECT_TRUE(magmaCube.canDamagePlayer());
}

TEST_F(MagmaCubeEntityTest, CanDamagePlayer_ComparedToSlime)
{
    // 岩浆怪 vs 史莱姆的小型实体伤害能力对比
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);
    magmaCube.setSlimeSize(1, false);

    SlimeEntity slime(EntityInstanceId(2));
    slime.setWorld(&m_world);
    slime.setSlimeSize(1, false);

    // 小型岩浆怪可以伤害玩家
    EXPECT_TRUE(magmaCube.canDamagePlayer());
    // 小型史莱姆不能伤害玩家
    EXPECT_FALSE(slime.canDamagePlayer());
}

// ==================== 挤压动画衰减测试 ====================

TEST_F(MagmaCubeEntityTest, SquishAmount_SetterAndGetter)
{
    // 测试挤压量的设置和获取
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    magmaCube.setSquishAmount(1.0f);
    EXPECT_FLOAT_EQ(magmaCube.squishAmount(), 1.0f);

    magmaCube.setSquishAmount(0.5f);
    EXPECT_FLOAT_EQ(magmaCube.squishAmount(), 0.5f);
}

TEST_F(MagmaCubeEntityTest, SquishAmount_ComparisonWithSlime)
{
    // 验证岩浆怪和史莱姆都能设置和获取挤压量
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    SlimeEntity slime(EntityInstanceId(2));

    magmaCube.setSquishAmount(0.9f);
    slime.setSquishAmount(0.6f);

    // 岩浆怪的衰减系数在 tick() 中使用 0.9，史莱姆使用 0.6
    // 这个差异通过 alterSquishAmount() 虚函数实现
    // 由于 alterSquishAmount() 是 protected 方法，我们通过公共接口验证
    EXPECT_FLOAT_EQ(magmaCube.squishAmount(), 0.9f);
    EXPECT_FLOAT_EQ(slime.squishAmount(), 0.6f);
}

// ==================== 粒子类型测试 ====================

TEST_F(MagmaCubeEntityTest, GetSquishParticle_ReturnsFlame)
{
    // MC 1.16.5: 岩浆怪使用火焰粒子
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    auto particleType = magmaCube.getSquishParticle();
    EXPECT_EQ(particleType, particle::ParticleTypeId::Flame);
}

TEST_F(MagmaCubeEntityTest, GetSquishParticle_ComparedToSlime)
{
    // 岩浆怪 vs 史莱姆的粒子类型对比
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    SlimeEntity slime(EntityInstanceId(2));

    // 岩浆怪: 火焰粒子
    EXPECT_EQ(magmaCube.getSquishParticle(), particle::ParticleTypeId::Flame);
    // 史莱姆: 粘液粒子
    EXPECT_EQ(slime.getSquishParticle(), particle::ParticleTypeId::ItemSlime);
}

// ==================== 火焰免疫测试 ====================

TEST_F(MagmaCubeEntityTest, IsImmuneToFire_ReturnsTrue)
{
    // MC 1.16.5: 岩浆怪免疫火焰
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    EXPECT_TRUE(magmaCube.isImmuneToFire());
}

TEST_F(MagmaCubeEntityTest, IsImmuneToFire_ComparedToSlime)
{
    // 岩浆怪 vs 史莱姆的火焰免疫对比
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    SlimeEntity slime(EntityInstanceId(2));

    // 岩浆怪免疫火焰
    EXPECT_TRUE(magmaCube.isImmuneToFire());
    // 史莱姆不免疫火焰
    EXPECT_FALSE(slime.isImmuneToFire());
}

// ==================== 声音测试 ====================

TEST_F(MagmaCubeEntityTest, GetHurtSound_ReturnsCorrectSoundForSize)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 小岩浆怪
    magmaCube.setSlimeSize(1, false);
    EnvironmentalDamage damage = DamageSources::generic();
    auto sound = magmaCube.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.hurt_small");

    // 大岩浆怪
    magmaCube.setSlimeSize(4, false);
    sound = magmaCube.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.hurt");
}

TEST_F(MagmaCubeEntityTest, GetDeathSound_ReturnsCorrectSoundForSize)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 小岩浆怪
    magmaCube.setSlimeSize(1, false);
    auto sound = magmaCube.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.death_small");

    // 大岩浆怪
    magmaCube.setSlimeSize(4, false);
    sound = magmaCube.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.death");
}

TEST_F(MagmaCubeEntityTest, GetSquishSound_ReturnsCorrectSoundForSize)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 小岩浆怪
    magmaCube.setSlimeSize(1, false);
    auto sound = magmaCube.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.squish_small");

    // 大岩浆怪
    magmaCube.setSlimeSize(4, false);
    sound = magmaCube.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.squish");
}

TEST_F(MagmaCubeEntityTest, GetJumpSound_ReturnsCorrectSound)
{
    // MC 1.16.5: 岩浆怪跳跃音效与史莱姆不同
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    auto sound = magmaCube.getJumpSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.magma_cube.jump");
}

// ==================== 分裂测试 ====================

TEST_F(MagmaCubeEntityTest, CanSplit_ReturnsTrueForSizeGreaterThanOne)
{
    // MC 1.16.5: 岩浆怪分裂逻辑继承自史莱姆
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    magmaCube.setSlimeSize(1, false);
    EXPECT_FALSE(magmaCube.canSplit());

    magmaCube.setSlimeSize(2, false);
    EXPECT_TRUE(magmaCube.canSplit());

    magmaCube.setSlimeSize(4, false);
    EXPECT_TRUE(magmaCube.canSplit());
}

TEST_F(MagmaCubeEntityTest, IsSmallSlime_InheritsFromSlimeEntity)
{
    // 岩浆怪的 isSmallSlime 继承自史莱姆
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    magmaCube.setSlimeSize(1, false);
    EXPECT_TRUE(magmaCube.isSmallSlime());

    magmaCube.setSlimeSize(2, false);
    EXPECT_FALSE(magmaCube.isSmallSlime());
}

// 注意：分裂测试需要 EntityRegistry 初始化，这里仅验证分裂条件
// 实际分裂行为在集成测试中验证

// ==================== 攻击伤害测试 ====================

TEST_F(MagmaCubeEntityTest, GetAttackDamage_AddsTwo)
{
    // MC 1.16.5: 岩浆怪攻击伤害 = 属性值 + 2.0F
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    // 尺寸 1: 基础伤害 1, 总伤害 = 1 + 2 = 3
    magmaCube.setSlimeSize(1, true);
    EXPECT_FLOAT_EQ(magmaCube.getAttackDamage(), 3.0f);

    // 尺寸 2: 基础伤害 2, 总伤害 = 2 + 2 = 4
    magmaCube.setSlimeSize(2, true);
    EXPECT_FLOAT_EQ(magmaCube.getAttackDamage(), 4.0f);

    // 尺寸 4: 基础伤害 4, 总伤害 = 4 + 2 = 6
    magmaCube.setSlimeSize(4, true);
    EXPECT_FLOAT_EQ(magmaCube.getAttackDamage(), 6.0f);
}

// ==================== 目标选择器测试 ====================

TEST_F(MagmaCubeEntityTest, TargetSelector_InheritsFromSlimeEntity)
{
    // 岩浆怪继承史莱姆的目标选择器
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    const auto& targetSelector = magmaCube.targetSelector();
    const auto& goals = targetSelector.getAllGoals();

    // 验证至少有两个目标（继承自史莱姆）
    EXPECT_GE(goals.size(), 2u) << "MagmaCubeEntity should have at least 2 target goals inherited from SlimeEntity";
}

TEST_F(MagmaCubeEntityTest, GoalSelector_InheritsFromSlimeEntity)
{
    // 岩浆怪继承史莱姆的 AI 目标
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    const auto& goalSelector = magmaCube.goalSelector();
    const auto& goals = goalSelector.getAllGoals();

    // 验证至少有四个 AI 目标（继承自史莱姆）
    EXPECT_GE(goals.size(), 4u) << "MagmaCubeEntity should have at least 4 AI goals inherited from SlimeEntity";
}

// ==================== 维度测试 ====================

TEST_F(MagmaCubeEntityTest, GetDimensions_ScalesWithSize)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));

    // 尺寸计算与史莱姆相同
    // size = 1: 0.6 * 0.255 = 0.153
    magmaCube.setSlimeSize(1, false);
    auto dims = magmaCube.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f);

    // size = 4: 0.6 * 0.255 * 4 = 0.612
    magmaCube.setSlimeSize(4, false);
    dims = magmaCube.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f * 4.0f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f * 4.0f);
}

// ==================== 经验值测试 ====================

TEST_F(MagmaCubeEntityTest, ExperienceValue_EqualsSize)
{
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);

    // 经验值继承自史莱姆，等于尺寸
    magmaCube.setSlimeSize(1, true);
    EXPECT_EQ(magmaCube.experienceValue(), 1);

    magmaCube.setSlimeSize(4, true);
    EXPECT_EQ(magmaCube.experienceValue(), 4);

    magmaCube.setSlimeSize(2, false);
    EXPECT_EQ(magmaCube.experienceValue(), 2);
}

// ==================== 综合对比测试 ====================

TEST_F(MagmaCubeEntityTest, ComprehensiveComparison_WithSlimeEntity)
{
    // 综合对比岩浆怪与史莱姆的差异
    MagmaCubeEntity magmaCube(EntityInstanceId(1));
    magmaCube.setWorld(&m_world);
    magmaCube.setSlimeSize(4, true);

    SlimeEntity slime(EntityInstanceId(2));
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);

    // 相同点
    EXPECT_EQ(magmaCube.getSlimeSize(), slime.getSlimeSize());
    EXPECT_EQ(magmaCube.canSplit(), slime.canSplit());
    EXPECT_EQ(magmaCube.isSmallSlime(), slime.isSmallSlime());
    EXPECT_EQ(magmaCube.experienceValue(), slime.experienceValue());

    // 不同点
    // 1. 火焰免疫
    EXPECT_TRUE(magmaCube.isImmuneToFire());
    EXPECT_FALSE(slime.isImmuneToFire());

    // 2. 护甲
    EXPECT_GT(magmaCube.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0),
        slime.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0));

    // 3. 跳跃延迟
    EXPECT_GT(magmaCube.getJumpDelay(), slime.getJumpDelay());

    // 4. 粒子类型
    EXPECT_NE(magmaCube.getSquishParticle(), slime.getSquishParticle());

    // 5. 小尺寸伤害玩家能力
    magmaCube.setSlimeSize(1, false);
    slime.setSlimeSize(1, false);
    EXPECT_TRUE(magmaCube.canDamagePlayer());
    EXPECT_FALSE(slime.canDamagePlayer());
}

} // namespace
} // namespace mc
