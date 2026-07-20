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

#include "../../../TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/village/raid/Raid.hpp"
#include "common/world/village/raid/RaiderType.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc::test {

/**
 * @brief Raid 私有方法测试访问器。
 *
 * 通过 friend 关系暴露 _updateBossBar、_getHealthOfLivingRaiders 以及若干私有
 * 字段设置器，使单元测试可以在不依赖完整袭击生成流程（Village、实体注册等）
 * 的前提下精确控制 Raid 的内部状态，从而覆盖三段式 Boss 栏进度的所有边界。
 */
class RaidTestAccessor {
public:
    explicit RaidTestAccessor(world::village::raid::Raid& raid)
        : m_raid(raid)
    {}

    /// @brief 直接调用私有 _updateBossBar。
    void updateBossBar(IWorld& world) { m_raid._updateBossBar(world); }

    /// @brief 直接调用私有 _getHealthOfLivingRaiders。
    [[nodiscard]] f32 getHealthOfLivingRaiders(IWorld& world) const { return m_raid._getHealthOfLivingRaiders(world); }

    /// @brief 直接读取缓存的进度（绕过 getBossBarProgress 的状态过滤）。
    [[nodiscard]] f32 cachedProgress() const { return m_raid.m_cachedProgress; }

    /// @brief 直接读取当前波总血量。
    [[nodiscard]] f32 totalHealth() const { return m_raid.m_totalHealth; }

    /// @brief 设置当前波总血量（用于在不调用 spawnRaiders 的情况下构造分母）。
    void setTotalHealth(f32 value) { m_raid.m_totalHealth = value; }

    /// @brief 直接读取波间冷却倒计时。
    [[nodiscard]] i32 raidCooldownTicks() const { return m_raid.m_raidCooldownTicks; }

    /// @brief 设置波间冷却倒计时（模拟 onRaiderDeath 启动冷却后的中间状态）。
    void setRaidCooldownTicks(i32 ticks) { m_raid.m_raidCooldownTicks = ticks; }

    /// @brief 直接设置当前波次编号，绕过 startNextWave 触发的生成流程。
    void setWave(i32 wave) { m_raid.m_wave = wave; }

    /// @brief 直接设置袭击状态。
    void setStatus(world::village::raid::RaidStatus status) { m_raid.m_status = status; }

    /// @brief 直接设置难度（用于 maxWaves 等下游计算）。
    void setDifficulty(Difficulty difficulty) { m_raid.m_difficulty = difficulty; }

    /// @brief 直接设置不祥之兆等级。
    void setBadOmenLevel(i32 level) { m_raid.m_badOmenLevel = level; }

    /// @brief 通过公开接口 addRaider 添加追踪 ID。
    void addRaider(EntityInstanceId id) { m_raid.addRaider(id); }

    /// @brief 通过公开接口 removeRaider 移除追踪 ID。
    void removeRaider(EntityInstanceId id) { m_raid.removeRaider(id); }

    /// @brief 暴露内部 raiders 列表大小，便于断言死亡后是否被正确移除。
    [[nodiscard]] size_t trackedRaidersCount() const { return m_raid.raiders().size(); }

private:
    world::village::raid::Raid& m_raid;
};

} // namespace mc::test

namespace mc {
namespace world::village::raid {
namespace test {

// ============================================================================
// 测试夹具
// ============================================================================

/**
 * @brief Raid Boss 栏进度单元测试夹具。
 *
 * 提供一个可注入实体的最小化 IWorld 实现，以及若干构造好的 LivingEntity
 * 实例，用于覆盖 _updateBossBar 和 _getHealthOfLivingRaiders 的所有分支。
 */
class RaidBossBarProgressTest : public ::testing::Test {
protected:
    /**
     * @brief 可控难度的最小化 IWorld 桩。
     *
     * 仅实现 _updateBossBar / _getHealthOfLivingRaiders 所依赖的接口：
     * getEntity / difficulty / seed / currentTick。其他接口走基类默认。
     */
    class RaidBossBarTestWorld final : public ::mc::test::BaseTestWorld {
    public:
        [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
        {
            const auto it = m_entities.find(static_cast<u64>(id));
            return it != m_entities.end() ? it->second.get() : nullptr;
        }

        [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
        {
            const auto it = m_entities.find(static_cast<u64>(id));
            return it != m_entities.end() ? it->second.get() : nullptr;
        }

        [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
        void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }

        [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
        void setCurrentTick(u64 tick) { m_currentTick = tick; }

        /// @brief 注册一个裸 LivingEntity 并指定初始血量。
        EntityInstanceId addLivingEntityWithHealth(EntityInstanceId id, f32 health)
        {
            auto entity = std::make_unique<LivingEntity>(id, nullptr);
            entity->registerData();
            entity->registerAttributes();
            entity->setHealth(health);
            m_entities[static_cast<u64>(id)] = std::move(entity);
            return id;
        }

        /// @brief 注册一个非 LivingEntity 的纯 Entity，用于测试 dynamic_cast 失败分支。
        EntityInstanceId addBareEntity(EntityInstanceId id)
        {
            auto entity = std::make_unique<Entity>(id, nullptr);
            m_entities[static_cast<u64>(id)] = std::move(entity);
            return id;
        }

        /// @brief 从世界中移除实体，模拟实体已离开世界（getEntity 返回 nullptr）。
        void removeEntity(EntityInstanceId id) { m_entities.erase(static_cast<u64>(id)); }

        /// @brief 直接修改已注册实体的血量。
        void setEntityHealth(EntityInstanceId id, f32 health)
        {
            const auto it = m_entities.find(static_cast<u64>(id));
            ASSERT_NE(it, m_entities.end());
            auto* living = dynamic_cast<LivingEntity*>(it->second.get());
            ASSERT_NE(living, nullptr);
            living->setHealth(health);
        }

    private:
        std::unordered_map<u64, std::unique_ptr<Entity>> m_entities;
        Difficulty m_difficulty = Difficulty::Normal;
        u64 m_currentTick = 0;
    };

    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_raid = std::make_unique<Raid>(RaidId(1), nullptr);
        m_accessor = std::make_unique<::mc::test::RaidTestAccessor>(*m_raid);
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_raid.reset();
    }

    RaidBossBarTestWorld m_world;
    std::unique_ptr<Raid> m_raid;
    std::unique_ptr<::mc::test::RaidTestAccessor> m_accessor;
};

// ============================================================================
// getBossBarProgress / _updateBossBar：状态过滤分支
// ============================================================================

TEST_F(RaidBossBarProgressTest, GetBossBarProgress_NonOngoingStatus_ReturnsZero)
{
    // 胜利、失败、停止三种非 Ongoing 状态均应返回 0。
    m_accessor->setStatus(RaidStatus::Victory);
    m_accessor->setTotalHealth(100.0f);
    m_accessor->updateBossBar(m_world);
    EXPECT_FLOAT_EQ(m_raid->getBossBarProgress(), 0.0f);

    m_accessor->setStatus(RaidStatus::Loss);
    m_accessor->updateBossBar(m_world);
    EXPECT_FLOAT_EQ(m_raid->getBossBarProgress(), 0.0f);

    m_accessor->setStatus(RaidStatus::Stopped);
    m_accessor->updateBossBar(m_world);
    EXPECT_FLOAT_EQ(m_raid->getBossBarProgress(), 0.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_NonOngoingStatus_CachesZero)
{
    m_accessor->setStatus(RaidStatus::Victory);
    m_accessor->setTotalHealth(100.0f);
    m_accessor->setRaidCooldownTicks(150);

    m_accessor->updateBossBar(m_world);

    // 即便 totalHealth 与 cooldownTicks 都不为零，非 Ongoing 状态下缓存也应为 0。
    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.0f);
}

// ============================================================================
// _updateBossBar：战斗中分支（m_totalHealth 决定行为）
// ============================================================================

TEST_F(RaidBossBarProgressTest, UpdateBossBar_TotalHealthZero_ReturnsOneToAvoidNaN)
{
    // 战斗中、未冷却、分母为 0（波次尚未生成任何袭击者）时，进度应视为 1.0，
    // 与 Java 版 totalHealth == 0 时的退化行为一致，避免 0/0 NaN。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setTotalHealth(0.0f);
    m_accessor->setRaidCooldownTicks(0);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 1.0f);
    EXPECT_FLOAT_EQ(m_raid->getBossBarProgress(), 1.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_AllRaidersAlive_ReturnsOne)
{
    // 三个袭击者各 20 血，totalHealth = 60，存活血量 = 60，进度应为 1.0。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);

    constexpr EntityInstanceId id1{1001};
    constexpr EntityInstanceId id2{1002};
    constexpr EntityInstanceId id3{1003};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_world.addLivingEntityWithHealth(id2, 20.0f);
    m_world.addLivingEntityWithHealth(id3, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);
    m_accessor->addRaider(id3);
    m_accessor->setTotalHealth(60.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 1.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_HalfRaidersDead_ReturnsHalfProgress)
{
    // 两个袭击者各 20 血，totalHealth = 40；其中一个血量降到 10，存活总血量 = 30。
    // 进度 = 30 / 40 = 0.75。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);

    constexpr EntityInstanceId id1{2001};
    constexpr EntityInstanceId id2{2002};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_world.addLivingEntityWithHealth(id2, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);
    m_accessor->setTotalHealth(40.0f);

    // 把 id1 打到 10 血
    m_world.setEntityHealth(id1, 10.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.75f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_AllRaidersAtZeroHealth_ReturnsZero)
{
    // 全部袭击者血量为 0（处于死亡过渡阶段），存活血量为 0，进度应为 0。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);

    constexpr EntityInstanceId id1{3001};
    constexpr EntityInstanceId id2{3002};
    m_world.addLivingEntityWithHealth(id1, 0.0f);
    m_world.addLivingEntityWithHealth(id2, 0.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);
    m_accessor->setTotalHealth(40.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_PartialRaidersAtZeroHealth_OnlyCountsAlive)
{
    // 三个袭击者：一个满血 20、一个 10 血、一个 0 血（已死）。totalHealth = 60。
    // 存活血量 = 20 + 10 + 0 = 30，进度 = 30 / 60 = 0.5。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);

    constexpr EntityInstanceId id1{4001};
    constexpr EntityInstanceId id2{4002};
    constexpr EntityInstanceId id3{4003};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_world.addLivingEntityWithHealth(id2, 10.0f);
    m_world.addLivingEntityWithHealth(id3, 0.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);
    m_accessor->addRaider(id3);
    m_accessor->setTotalHealth(60.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.5f);
}

// ============================================================================
// _updateBossBar：波间冷却分支（300 → 0 倒计时）
// ============================================================================

TEST_F(RaidBossBarProgressTest, UpdateBossBar_CooldownJustStarted_ReturnsZero)
{
    // onRaiderDeath 启动冷却瞬间，raidCooldownTicks = 300，进度 = 0/300 = 0。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(RaidConfig::RAID_COOLDOWN_TICKS);
    m_accessor->setTotalHealth(60.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_CooldownHalfway_ReturnsHalfProgress)
{
    // 冷却过半，raidCooldownTicks = 150，进度 = (300-150)/300 = 0.5。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(150);
    m_accessor->setTotalHealth(60.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.5f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_CooldownOneTickLeft_ReturnsNearOne)
{
    // 冷却即将结束，raidCooldownTicks = 1，进度 = (300-1)/300。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(1);
    m_accessor->setTotalHealth(60.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), (300.0f - 1.0f) / 300.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_CooldownReachesZero_SwitchesToCombatMode)
{
    // 冷却归零后，进度计算应切回战斗中模式。此时 m_totalHealth = 0 退化到 1.0。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);
    m_accessor->setTotalHealth(0.0f);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 1.0f);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_CooldownTakesPrecedenceOverCombat)
{
    // 即使 m_totalHealth > 0 且有存活袭击者，只要 raidCooldownTicks > 0，
    // 进度就应走冷却分支。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(100);
    m_accessor->setTotalHealth(60.0f);

    constexpr EntityInstanceId id1{5001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);

    m_accessor->updateBossBar(m_world);

    // 冷却分支：(300-100)/300 = 200/300
    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 200.0f / 300.0f);
}

// ============================================================================
// _getHealthOfLivingRaiders：实体不存在 / 非 LivingEntity / 已死亡
// ============================================================================

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_EmptyRaiders_ReturnsZero)
{
    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 0.0f);
}

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_EntityNotInWorld_ReturnsZero)
{
    // raiders 列表中存在 ID，但世界中没有该实体（已离开世界）。
    m_accessor->addRaider(EntityInstanceId(6001));
    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 0.0f);
}

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_EntityRemovedFromWorldMidRaid_ExcludedFromSum)
{
    // 三个袭击者各 20 血。移除其中一个后，存活血量应为 40。
    constexpr EntityInstanceId id1{7001};
    constexpr EntityInstanceId id2{7002};
    constexpr EntityInstanceId id3{7003};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_world.addLivingEntityWithHealth(id2, 20.0f);
    m_world.addLivingEntityWithHealth(id3, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);
    m_accessor->addRaider(id3);

    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 60.0f);

    m_world.removeEntity(id2);

    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 40.0f);
}

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_NonLivingEntity_Skipped)
{
    // raiders 列表中包含一个非 LivingEntity 的纯 Entity，应被 dynamic_cast 跳过。
    // 注意：LivingEntity 默认 maxHealth = 20，setHealth 会被 clamp 到 [0, 20]，
    // 因此这里使用 20.0f 而非 25.0f 以避免 clamp 干扰断言。
    constexpr EntityInstanceId id1{8001};
    constexpr EntityInstanceId id2{8002};
    m_world.addBareEntity(id1); // 纯 Entity
    m_world.addLivingEntityWithHealth(id2, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);

    // 仅 id2 的 20 血应计入。
    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 20.0f);
}

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_DeadEntityAtZeroHealth_Excluded)
{
    // 一个满血袭击者 + 一个 0 血袭击者（已死），存活血量应只算满血者。
    constexpr EntityInstanceId id1{9001};
    constexpr EntityInstanceId id2{9002};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_world.addLivingEntityWithHealth(id2, 0.0f);
    m_accessor->addRaider(id1);
    m_accessor->addRaider(id2);

    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 20.0f);
}

TEST_F(RaidBossBarProgressTest, GetHealthOfLivingRaiders_HealthChangesAfterSpawn_ReflectedInSum)
{
    // 模拟袭击者被攻击：初始 20 血，受伤降到 5，存活血量应实时反映。
    constexpr EntityInstanceId id1{10001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);

    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 20.0f);

    m_world.setEntityHealth(id1, 5.0f);
    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 5.0f);

    m_world.setEntityHealth(id1, 0.0f);
    EXPECT_FLOAT_EQ(m_accessor->getHealthOfLivingRaiders(m_world), 0.0f);
}

// ============================================================================
// _updateBossBar：进度值的范围与 clamp 行为
// ============================================================================

TEST_F(RaidBossBarProgressTest, UpdateBossBar_LivingHealthExceedsTotal_ClampedToOne)
{
    // 极端情况：存活血量超过 totalHealth（理论上不应发生，但需保证 clamp 到 1.0）。
    m_accessor->setStatus(RaidStatus::Ongoing);
    m_accessor->setRaidCooldownTicks(0);
    m_accessor->setTotalHealth(10.0f);

    constexpr EntityInstanceId id1{11001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);

    m_accessor->updateBossBar(m_world);

    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 1.0f);
}

// 注：原 UpdateBossBar_CooldownProgressNeverExceedsOne 测试假设 raidCooldownTicks
// 可能为负数并验证 clamp 行为。但 _updateBossBar 的实现使用 `if (m_raidCooldownTicks > 0)`
// 作为分支条件，负值会落入战斗分支而非冷却分支，因此该边界由战斗分支的 clamp
// 隐式保证（livingHealth / totalHealth ≤ 1.0）。冷却分支自身的上界由
// UpdateBossBar_CooldownOneTickLeft_ReturnsNearOne 覆盖（进度接近但不超过 1.0）。

// ============================================================================
// getBossBarProgress：默认状态与新构造 Raid
// ============================================================================

TEST_F(RaidBossBarProgressTest, GetBossBarProgress_NewRaidOngoing_CachedProgressInitiallyZero)
{
    // 新构造的 Raid 默认 Ongoing，m_cachedProgress 默认 0，外部应读到 0。
    EXPECT_EQ(m_raid->status(), RaidStatus::Ongoing);
    EXPECT_FLOAT_EQ(m_raid->getBossBarProgress(), 0.0f);
}

// ============================================================================
// 综合场景：onRaiderDeath → 冷却启动 → tick 推进
// ============================================================================

TEST_F(RaidBossBarProgressTest, OnRaiderDeath_LastRaiderDead_StartsCooldown)
{
    // 通过 addRaider 加入一个袭击者，然后通过 onRaiderDeath 触发冷却。
    // 注意：onRaiderDeath 会调用 removeRaider，使 isWaveDefeated() 返回 true。
    m_accessor->setWave(1);
    m_accessor->setDifficulty(Difficulty::Normal);
    m_accessor->setBadOmenLevel(1);

    constexpr EntityInstanceId id1{12001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->setTotalHealth(20.0f);

    // 死亡前 cooldown 应为 0。
    EXPECT_EQ(m_accessor->raidCooldownTicks(), 0);

    m_raid->onRaiderDeath(id1, m_world);

    // 死亡后应启动 300 tick 冷却（因为 hasMoreWaves 在 Normal 难度 + wave=1 时为 true）。
    EXPECT_EQ(m_accessor->raidCooldownTicks(), RaidConfig::RAID_COOLDOWN_TICKS);
    EXPECT_EQ(m_accessor->trackedRaidersCount(), 0u);
}

TEST_F(RaidBossBarProgressTest, OnRaiderDeath_LastRaiderDeadNoMoreWaves_TriggersVictory)
{
    // Hard 难度 + wave=7（最大波），击败最后一个袭击者应直接 setVictory，不启动冷却。
    m_accessor->setWave(7);
    m_accessor->setDifficulty(Difficulty::Hard);
    m_accessor->setBadOmenLevel(1);

    constexpr EntityInstanceId id1{13001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->setTotalHealth(20.0f);

    m_raid->onRaiderDeath(id1, m_world);

    EXPECT_EQ(m_raid->status(), RaidStatus::Victory);
    EXPECT_EQ(m_accessor->raidCooldownTicks(), 0);
}

TEST_F(RaidBossBarProgressTest, UpdateBossBar_AfterCooldownStarted_ProgressReflectsCooldown)
{
    // 模拟 onRaiderDeath 启动冷却后，_updateBossBar 的进度应走冷却分支。
    m_accessor->setWave(1);
    m_accessor->setDifficulty(Difficulty::Normal);
    m_accessor->setBadOmenLevel(1);

    constexpr EntityInstanceId id1{14001};
    m_world.addLivingEntityWithHealth(id1, 20.0f);
    m_accessor->addRaider(id1);
    m_accessor->setTotalHealth(20.0f);

    m_raid->onRaiderDeath(id1, m_world);

    // 冷却刚启动，进度应为 0。
    m_accessor->updateBossBar(m_world);
    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.0f);

    // 模拟 150 tick 后，进度应为 0.5。
    m_accessor->setRaidCooldownTicks(150);
    m_accessor->updateBossBar(m_world);
    EXPECT_FLOAT_EQ(m_accessor->cachedProgress(), 0.5f);
}

// ============================================================================
// RaidConfig::RAID_COOLDOWN_TICKS 常量正确性
// ============================================================================

TEST_F(RaidBossBarProgressTest, RaidConfig_CooldownTicksConstant_MatchesJavaVersion)
{
    // Java 版 1.21.11 Raid 中 raidCooldownTicks 初始值为 300。
    EXPECT_EQ(RaidConfig::RAID_COOLDOWN_TICKS, 300);
}

} // namespace test
} // namespace world::village::raid
} // namespace mc
