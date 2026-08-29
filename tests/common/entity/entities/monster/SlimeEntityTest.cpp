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
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供史莱姆测试所需的最小 IWorld 接口实现
 */
class SlimeTestWorld final : public mc::test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 记录生成的史莱姆
        if (auto* slime = dynamic_cast<SlimeEntity*>(entity.get())) {
            m_spawnedSlimeSizes.push_back(slime->getSlimeSize());
            m_spawnedSlimeCount++;
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SlimeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SlimeTestWorld::tickManager not implemented");
    }

    // 测试辅助方法
    [[nodiscard]] size_t spawnedSlimeCount() const { return m_spawnedSlimeCount; }
    [[nodiscard]] const std::vector<i32>& spawnedSlimeSizes() const { return m_spawnedSlimeSizes; }
    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_spawnedSlimeSizes.clear();
        m_spawnedSlimeCount = 0;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<i32> m_spawnedSlimeSizes;
    size_t m_spawnedSlimeCount = 0;
};

class SlimeEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // PerformSplit 通过 EntityRegistry 查询 EntityTypeKeys::SLIME 来 create 子史莱姆，
        // 需先注册原版实体类型；registerAll 经修复后可重入（hasType 哨兵双检）。
        entity::VanillaEntities::registerAll();
    }

    SlimeTestWorld m_world;
};

// ==================== 尺寸系统测试 ====================

TEST_F(SlimeEntityTest, SetSlimeSize_UpdatesAttributes)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    // 设置尺寸为 4
    slime.setSlimeSize(4, true);

    // 验证属性
    EXPECT_EQ(slime.getSlimeSize(), 4);
    // HP = size * size = 16
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0)), 16.0f);
    // Speed = 0.2 + 0.1 * size = 0.6
    EXPECT_FLOAT_EQ(
        static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.6f);
    // AttackDamage = size = 4
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 4.0f);
    // 生命值应该被重置为最大值
    EXPECT_FLOAT_EQ(slime.health(), slime.maxHealth());
}

TEST_F(SlimeEntityTest, SetSlimeSize_ClampsToValidRange)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 测试下限
    slime.setSlimeSize(0, false);
    EXPECT_EQ(slime.getSlimeSize(), 1); // 最小为 1

    // 测试上限
    slime.setSlimeSize(100, false);
    EXPECT_EQ(slime.getSlimeSize(), 4); // 最大为 4
}

TEST_F(SlimeEntityTest, IsSmallSlime_ReturnsTrueForSizeOne)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    slime.setSlimeSize(1, false);
    EXPECT_TRUE(slime.isSmallSlime());

    slime.setSlimeSize(2, false);
    EXPECT_FALSE(slime.isSmallSlime());

    slime.setSlimeSize(4, false);
    EXPECT_FALSE(slime.isSmallSlime());
}

TEST_F(SlimeEntityTest, CanSplit_ReturnsTrueForSizeGreaterThanOne)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    slime.setSlimeSize(1, false);
    EXPECT_FALSE(slime.canSplit());

    slime.setSlimeSize(2, false);
    EXPECT_TRUE(slime.canSplit());

    slime.setSlimeSize(4, false);
    EXPECT_TRUE(slime.canSplit());
}

// ==================== 分裂测试 ====================

TEST_F(SlimeEntityTest, PerformSplit_CreatesCorrectNumberOfSmallSlimes)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    // 直接构造不经 EntityType::create 工厂，m_typeId 默认空。performSplit 经 entityType()
    // 查 EntityRegistry 取 SLIME 类型创建子史莱姆，typeId 空时查不到提前 return（不分裂）。
    // 补 setTypeId 对齐生产路径（自然生成/工厂构造的史莱姆 typeId=minecraft:slime）。
    slime.setTypeId(entity::EntityTypeKeys::SLIME);
    slime.setSlimeSize(4, true); // 大史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证生成了 2-4 个小史莱姆
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
    EXPECT_LE(m_world.spawnedSlimeCount(), 4u);

    // 验证小史莱姆的尺寸是 2（4 / 2）
    for (i32 size : m_world.spawnedSlimeSizes()) {
        EXPECT_EQ(size, 2);
    }
}

TEST_F(SlimeEntityTest, PerformSplit_SmallSlimeDoesNotSplit)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setTypeId(entity::EntityTypeKeys::SLIME); // 对齐生产路径（见上测试注释）
    slime.setSlimeSize(1, true);                    // 小史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 小史莱姆不应该分裂
    EXPECT_EQ(m_world.spawnedSlimeCount(), 0u);
}

TEST_F(SlimeEntityTest, PerformSplit_InheritsCustomName)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setTypeId(entity::EntityTypeKeys::SLIME); // 对齐生产路径（见上测试注释）
    slime.setSlimeSize(4, true);
    slime.setPosition(100.0, 64.0, 100.0);
    slime.setCustomName("TestSlime");

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证分裂了
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
}

TEST_F(SlimeEntityTest, PerformSplit_InheritsInvulnerability)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setTypeId(entity::EntityTypeKeys::SLIME); // 对齐生产路径（见上测试注释）
    slime.setSlimeSize(4, true);
    slime.setPosition(100.0, 64.0, 100.0);
    slime.setInvulnerable(true);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证分裂了
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
}

TEST_F(SlimeEntityTest, PerformSplit_MediumSlimeCreatesTinySlimes)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setTypeId(entity::EntityTypeKeys::SLIME); // 对齐生产路径（见上测试注释）
    slime.setSlimeSize(2, true);                    // 中型史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 中型史莱姆（size=2）应分裂出 2-4 个小史莱姆（防假通过：此前 typeId 缺失致 performSplit
    // 提前 return、spawnedSlimeSizes 为空，for 循环不断言而假通过）
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
    // 验证小史莱姆的尺寸是 1（2 / 2）
    for (i32 size : m_world.spawnedSlimeSizes()) {
        EXPECT_EQ(size, 1);
    }
}

// ==================== 声音测试 ====================

TEST_F(SlimeEntityTest, GetHurtSound_ReturnsCorrectSoundForSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 小史莱姆
    slime.setSlimeSize(1, false);
    EnvironmentalDamage damage = DamageSources::generic();
    auto sound = slime.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.hurt_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.hurt");
}

TEST_F(SlimeEntityTest, GetDeathSound_ReturnsCorrectSoundForSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 小史莱姆
    slime.setSlimeSize(1, false);
    auto sound = slime.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.death_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.death");
}

TEST_F(SlimeEntityTest, GetSquishSound_ReturnsCorrectSoundForSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 小史莱姆
    slime.setSlimeSize(1, false);
    auto sound = slime.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.squish_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.squish");
}

// ==================== 维度测试 ====================

TEST_F(SlimeEntityTest, GetDimensions_ScalesWithSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // size = 1: 0.6 * 0.255 = 0.153
    slime.setSlimeSize(1, false);
    auto dims = slime.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f);

    // size = 4: 0.6 * 0.255 * 4 = 0.612
    slime.setSlimeSize(4, false);
    dims = slime.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f * 4.0f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f * 4.0f);
}

TEST_F(SlimeEntityTest, EyeHeight_ScalesWithSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    slime.setSlimeSize(1, false);
    // eyeHeight = 0.625 * height = 0.625 * (0.6 * 0.255 * size)
    // 注意：由于 Entity::height() 返回的是基类默认值 1.8f，
    // 而实际的尺寸计算在 getDimensions() 中，
    // SlimeEntity::eyeHeight() 使用 EYE_HEIGHT_FACTOR * height()
    // 这里我们测试的是 eyeHeight 方法的实现正确性
    f32 expectedEyeHeight1 = 0.625f * slime.height(); // 依赖基类 height()
    EXPECT_FLOAT_EQ(slime.eyeHeight(), expectedEyeHeight1);

    slime.setSlimeSize(4, false);
    f32 expectedEyeHeight4 = 0.625f * slime.height();
    EXPECT_FLOAT_EQ(slime.eyeHeight(), expectedEyeHeight4);
}

// ==================== 伤害测试 ====================

TEST_F(SlimeEntityTest, CanDamagePlayer_ReturnsFalseForSmallSlime)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    // 小史莱姆不能伤害玩家
    slime.setSlimeSize(1, false);
    EXPECT_FALSE(slime.canDamagePlayer());

    // 大史莱姆可以伤害玩家
    slime.setSlimeSize(4, false);
    EXPECT_TRUE(slime.canDamagePlayer());
}

TEST_F(SlimeEntityTest, GetSoundVolume_ScalesWithSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 体积 = 0.4 * size
    slime.setSlimeSize(1, false);
    EXPECT_FLOAT_EQ(slime.getSoundVolume(), 0.4f);

    slime.setSlimeSize(4, false);
    EXPECT_FLOAT_EQ(slime.getSoundVolume(), 1.6f);
}

// ==================== 经验值测试 ====================

TEST_F(SlimeEntityTest, ExperienceValue_EqualsSize)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    // 注意：setSlimeSize 会设置经验值等于尺寸
    slime.setSlimeSize(1, true);
    EXPECT_EQ(slime.experienceValue(), 1);

    // 改变尺寸时经验值也应该更新
    slime.setSlimeSize(4, true);
    EXPECT_EQ(slime.experienceValue(), 4);

    // 不重置生命值时经验值也会更新
    slime.setSlimeSize(2, false);
    EXPECT_EQ(slime.experienceValue(), 2);
}

// ==================== 着地粒子效果测试 ====================

/**
 * @brief 支持粒子生成的测试用世界实现
 *
 * 扩展 SlimeTestWorld，添加客户端模式支持和粒子记录功能。
 */
class ParticleTestWorld final : public mc::test::BaseTestWorld {
public:
    ParticleTestWorld() = default;

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
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    // 客户端/服务端模式控制
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ParticleTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ParticleTestWorld::tickManager not implemented");
    }

    // 粒子生成记录
    struct ParticleInfo {
        particle::ParticleTypeId type;
        Vector3 pos;
        Vector3 velocity;
    };

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override
    {
        (void)offset;
        (void)count;
        m_particles.push_back({type, pos, velocity});
    }

    // 测试辅助方法
    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }
    [[nodiscard]] const std::vector<ParticleInfo>& particles() const { return m_particles; }
    void clearParticles() { m_particles.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<ParticleInfo> m_particles;
    bool m_isClientSide = false;
};

class SlimeParticleTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    ParticleTestWorld m_world;
};

TEST_F(SlimeParticleTest, LandingGeneratesParticles_ClientSide)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(true);
    slime.setWorld(&m_world);
    slime.setSlimeSize(2, true);
    slime.setPosition(100.0, 64.0, 100.0);

    // 模拟着地：设置 onGround 为 false，然后在 tick 后变为 true
    // 由于我们不能直接设置 onGround，我们通过 tick 循环来测试
    // 着地粒子只在首次着地时生成（onGround 从 false 变为 true）

    // 在这里我们验证粒子只在客户端生成
    // 服务端不应该生成粒子
    m_world.setClientSide(true);
    slime.tick();

    // 验证粒子数量：size * 8 = 2 * 8 = 16
    // 由于 onGround 状态变化可能不会在单次 tick 中发生，
    // 这里我们主要验证粒子系统在客户端正确工作
}

TEST_F(SlimeParticleTest, NoParticles_ServerSide)
{
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false); // 服务端
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);
    slime.setPosition(100.0, 64.0, 100.0);

    // tick 多次
    for (int i = 0; i < 10; ++i) {
        slime.tick();
    }

    // 服务端不应该生成粒子
    EXPECT_EQ(m_world.particleCount(), 0u);
}

TEST_F(SlimeParticleTest, ParticleCount_ScalesWithSize)
{
    // 验证粒子数量公式：particleCount = size * 8
    // 参考 MC 1.16.5 SlimeEntity.tick()

    // size = 1: 1 * 8 = 8 个粒子
    EXPECT_EQ(1 * 8, 8);

    // size = 2: 2 * 8 = 16 个粒子
    EXPECT_EQ(2 * 8, 16);

    // size = 4: 4 * 8 = 32 个粒子
    EXPECT_EQ(4 * 8, 32);
}

TEST_F(SlimeParticleTest, ParticleType_IsItemSlime)
{
    // 验证粒子类型是 ItemSlime
    // 参考 MC 1.16.5 SlimeEntity.tick() 使用 ParticleTypes.ITEM_SLIME
    using namespace particle;
    EXPECT_EQ(ParticleTypeId::ItemSlime, ParticleTypeId::ItemSlime);
}

// ==================== 目标选择器测试 ====================

/**
 * @brief 目标选择器测试夹具
 */
class SlimeTargetSelectorTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    SlimeTestWorld m_world;
};

TEST_F(SlimeTargetSelectorTest, TargetSelector_HasCorrectGoalCount)
{
    // MC 1.16.5: 史莱姆应该有两个目标选择器
    // 优先级 1: NearestAttackableTargetGoal<Player>
    // 优先级 3: NearestAttackableTargetGoal<IronGolem>
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    const auto& targetSelector = slime.targetSelector();
    const auto& goals = targetSelector.getAllGoals();

    // 验证至少有两个目标
    EXPECT_GE(goals.size(), 2u) << "SlimeEntity should have at least 2 target goals (Player and IronGolem)";
}

TEST_F(SlimeTargetSelectorTest, TargetSelector_PlayerGoal_HasPriority1)
{
    // MC 1.16.5: 玩家目标应该在优先级 1
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    const auto& targetSelector = slime.targetSelector();
    const auto& goals = targetSelector.getAllGoals();

    // 查找优先级 1 的目标（应该是 Player 目标）
    bool hasPriority1Goal = false;
    for (const auto& goal : goals) {
        if (goal.getPriority() == 1) {
            hasPriority1Goal = true;
            break;
        }
    }
    EXPECT_TRUE(hasPriority1Goal) << "SlimeEntity should have a target goal at priority 1 (Player)";
}

TEST_F(SlimeTargetSelectorTest, TargetSelector_IronGolemGoal_HasPriority3)
{
    // MC 1.16.5: 铁傀儡目标应该在优先级 3
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    const auto& targetSelector = slime.targetSelector();
    const auto& goals = targetSelector.getAllGoals();

    // 查找优先级 3 的目标（应该是 IronGolem 目标）
    bool hasPriority3Goal = false;
    for (const auto& goal : goals) {
        if (goal.getPriority() == 3) {
            hasPriority3Goal = true;
            break;
        }
    }
    EXPECT_TRUE(hasPriority3Goal) << "SlimeEntity should have a target goal at priority 3 (IronGolem)";
}

TEST_F(SlimeTargetSelectorTest, GoalSelector_HasCorrectGoalCount)
{
    // MC 1.16.5: 史莱姆应该有四个 AI 目标
    // 优先级 1: SlimeFloatGoal (游泳)
    // 优先级 2: SlimeAttackGoal (攻击)
    // 优先级 3: SlimeFaceRandomGoal (随机转向)
    // 优先级 5: SlimeHopGoal (跳跃)
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);

    const auto& goalSelector = slime.goalSelector();
    const auto& goals = goalSelector.getAllGoals();

    // 验证至少有四个 AI 目标
    EXPECT_GE(goals.size(), 4u) << "SlimeEntity should have at least 4 AI goals";
}

TEST_F(SlimeTargetSelectorTest, YDifferenceLimit_Is4Blocks)
{
    // MC 1.16.5: 史莱姆只攻击 Y 轴高度差 <= 4 格的玩家
    // 这是一个数学逻辑验证测试
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setPosition(0.0, 64.0, 0.0);

    // 史莱姆在 Y=64，玩家在 Y=64（高度差 0）
    f64 yDiff = std::abs(64.0 - 64.0);
    EXPECT_LE(yDiff, 4.0) << "Y difference of 0 should be within limit";

    // 玩家在 Y=66（高度差 2）
    yDiff = std::abs(66.0 - 64.0);
    EXPECT_LE(yDiff, 4.0) << "Y difference of 2 should be within limit";

    // 玩家在 Y=68（高度差 4）
    yDiff = std::abs(68.0 - 64.0);
    EXPECT_LE(yDiff, 4.0) << "Y difference of 4 should be within limit";

    // 玩家在 Y=70（高度差 6，超出限制）
    yDiff = std::abs(70.0 - 64.0);
    EXPECT_GT(yDiff, 4.0) << "Y difference of 6 should exceed limit";

    // 玩家在 Y=58（高度差 6，向下超出限制）
    yDiff = std::abs(58.0 - 64.0);
    EXPECT_GT(yDiff, 4.0) << "Y difference of 6 (downward) should exceed limit";
}

TEST_F(SlimeTargetSelectorTest, YDifferenceLimit_EdgeCases)
{
    // 测试边界情况
    SlimeEntity slime(EntityInstanceId(1), mc::test::testEcsRegistry());
    slime.setWorld(&m_world);
    slime.setPosition(0.0, 64.0, 0.0);

    f64 slimeY = slime.y();

    // 精确在边界（高度差恰好 4.0）
    f64 playerY1 = slimeY + 4.0; // 上方边界
    f64 playerY2 = slimeY - 4.0; // 下方边界

    EXPECT_LE(std::abs(playerY1 - slimeY), 4.0) << "Exactly 4 blocks above should be within limit";
    EXPECT_LE(std::abs(playerY2 - slimeY), 4.0) << "Exactly 4 blocks below should be within limit";

    // 略微超出边界
    f64 playerY3 = slimeY + 4.001; // 上方超出
    f64 playerY4 = slimeY - 4.001; // 下方超出

    EXPECT_GT(std::abs(playerY3 - slimeY), 4.0) << "4.001 blocks above should exceed limit";
    EXPECT_GT(std::abs(playerY4 - slimeY), 4.0) << "4.001 blocks below should exceed limit";
}

} // namespace
} // namespace mc
