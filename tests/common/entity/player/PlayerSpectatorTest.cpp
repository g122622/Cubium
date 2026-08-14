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

/**
 * @file PlayerSpectatorTest.cpp
 * @brief Player 旁观者模式跟踪状态单元测试
 *
 * 测试 Player 基类中旁观者摄像机跟踪的相关方法：
 * - m_cameraEntityId 字段的读写（getCameraEntityId/setCameraEntityId）
 * - isSpectating() 状态查询
 * - onCameraEntityChanged() 虚方法调度和相等性守卫
 * - 旁观者模式下 noclip 设置
 * - 离开旁观者模式时 camera 清除及 onCameraEntityChanged 通知
 * - 旁观者模式下 attack() 设置旁观目标及 onCameraEntityChanged 通知
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ==================== 测试辅助：记录 onCameraEntityChanged 调用的 TestPlayer ====================

/**
 * @brief 重写 onCameraEntityChanged() 的测试用 Player 子类
 *
 * 记录每次 onCameraEntityChanged() 调用的参数，用于验证虚方法调度和相等性守卫逻辑。
 * 类似于 CauseExtraKnockbackTest 中 TestPlayer 重写 sendVelocityPacket() 的模式。
 */
class CameraTrackingTestPlayer : public Player {
public:
    explicit CameraTrackingTestPlayer(EntityInstanceId id)
        : Player(id, "CameraTestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0);
        attributes().setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
        setHealth(20.0f);
    }

    /**
     * @brief 重写 onCameraEntityChanged，记录调用参数
     */
    void onCameraEntityChanged(
        std::optional<EntityInstanceId> oldCameraId, std::optional<EntityInstanceId> newCameraId) override
    {
        m_cameraChangeCalls.push_back({oldCameraId, newCameraId});
    }

    // ========== 查询接口 ==========

    /// onCameraEntityChanged 被调用的次数
    [[nodiscard]] size_t cameraChangeCallCount() const { return m_cameraChangeCalls.size(); }

    /// 是否被调用过
    [[nodiscard]] bool wasCameraChangeCalled() const { return !m_cameraChangeCalls.empty(); }

    /// 获取第 N 次调用的参数（从 0 开始）
    [[nodiscard]] std::optional<EntityInstanceId> oldCameraIdAt(size_t index) const
    {
        if (index >= m_cameraChangeCalls.size()) {
            return std::nullopt;
        }
        return m_cameraChangeCalls[index].oldCameraId;
    }

    [[nodiscard]] std::optional<EntityInstanceId> newCameraIdAt(size_t index) const
    {
        if (index >= m_cameraChangeCalls.size()) {
            return std::nullopt;
        }
        return m_cameraChangeCalls[index].newCameraId;
    }

    /// 清除所有记录的调用
    void resetCameraChangeCalls() { m_cameraChangeCalls.clear(); }

private:
    struct CameraChangeCall {
        std::optional<EntityInstanceId> oldCameraId;
        std::optional<EntityInstanceId> newCameraId;
    };
    std::vector<CameraChangeCall> m_cameraChangeCalls;
};

// ==================== Player 旁观者摄像机跟踪测试 ====================

class PlayerSpectatorTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry()); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ---------- 基础状态测试 ----------

TEST_F(PlayerSpectatorTest, DefaultNotSpectating)
{
    // 默认情况下玩家没有旁观目标
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityId)
{
    // 设置旁观目标
    player->setCameraEntityId(EntityInstanceId(42));
    EXPECT_TRUE(player->isSpectating());
    EXPECT_TRUE(player->getCameraEntityId().has_value());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(42));
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdToNullopt)
{
    // 设置旁观目标后清除
    player->setCameraEntityId(EntityInstanceId(42));
    EXPECT_TRUE(player->isSpectating());

    player->setCameraEntityId(std::nullopt);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdOverwrite)
{
    // 多次设置旁观目标，后设置的覆盖前一个
    player->setCameraEntityId(EntityInstanceId(10));
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(10));

    player->setCameraEntityId(EntityInstanceId(20));
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(20));
    EXPECT_TRUE(player->isSpectating());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdZeroIsValid)
{
    // 实体 ID 为 0 也应该合法（虽然实际中不太可能）
    player->setCameraEntityId(EntityInstanceId(0));
    EXPECT_TRUE(player->isSpectating());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(0));
}

// ---------- 旁观者模式与 noclip 测试 ----------

TEST_F(PlayerSpectatorTest, SpectatorModeEnablesNoclip)
{
    // 切换到旁观者模式应该启用 noclip
    EXPECT_FALSE(player->noClip());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->noClip());
    EXPECT_TRUE(player->isSpectator());
}

TEST_F(PlayerSpectatorTest, LeaveSpectatorModeDisablesNoclip)
{
    // 切换到旁观者模式后离开，应该关闭 noclip
    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->noClip());

    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->noClip());
    EXPECT_FALSE(player->isSpectator());
}

TEST_F(PlayerSpectatorTest, LeaveSpectatorModeClearsCamera)
{
    // 切换到旁观者模式并设置旁观目标，离开旁观模式时应该清除
    player->setGameMode(GameMode::Spectator);
    player->setCameraEntityId(EntityInstanceId(100));
    EXPECT_TRUE(player->isSpectating());

    // 离开旁观者模式
    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SwitchBetweenSpectatorAndCreative)
{
    // 旁观者 → 创造 → 旁观者
    player->setGameMode(GameMode::Spectator);
    player->setCameraEntityId(EntityInstanceId(50));
    EXPECT_TRUE(player->isSpectating());

    player->setGameMode(GameMode::Creative);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->isSpectator());
    EXPECT_TRUE(player->noClip());
}

TEST_F(PlayerSpectatorTest, SwitchFromSurvivalToSpectatorNoCamera)
{
    // 生存模式切换到旁观者模式，不应该自动设置旁观目标
    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->isSpectating());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->isSpectator());   // 是旁观者模式
    EXPECT_FALSE(player->isSpectating()); // 但没有旁观目标
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

// ---------- 旁观者模式 attack() 测试 ----------

TEST_F(PlayerSpectatorTest, SpectatorAttackSetsCameraTarget)
{
    // 旁观者模式下攻击实体应该设置旁观目标而非造成伤害
    player->setGameMode(GameMode::Spectator);

    // 创建一个目标实体用于 attack 测试
    Entity target(EntityInstanceId(99), nullptr, mc::test::testEcsRegistry());

    // 旁观者模式下 attack 不应造成伤害，但会设置旁观目标
    player->attack(target);

    // 在 Player 基类中，旁观者 attack 会设置 cameraEntityId
    EXPECT_TRUE(player->isSpectating());
    EXPECT_TRUE(player->getCameraEntityId().has_value());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(99));
}

TEST_F(PlayerSpectatorTest, NonSpectatorAttackDoesNotSetCamera)
{
    // 非旁观者模式下攻击不应该设置旁观目标
    player->setGameMode(GameMode::Survival);

    Entity target(EntityInstanceId(99), nullptr, mc::test::testEcsRegistry());
    // 注意：非旁观者的 attack 会正常执行攻击逻辑，
    // 但对于没有世界/没有 LivingEntity 目标的情况，attack 会提前返回
    player->attack(target);

    // 不应该设置旁观目标
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

// ---------- isInputSneaking 测试 ----------

TEST_F(PlayerSpectatorTest, IsInputSneakingDefaultFalse)
{
    // 默认不潜行
    EXPECT_FALSE(player->isInputSneaking());
}

// ==================== onCameraEntityChanged 虚方法调度和相等性守卫测试 ====================

class CameraChangedTrackingTest : public ::testing::Test {
protected:
    void SetUp() override { trackingPlayer = std::make_unique<CameraTrackingTestPlayer>(EntityInstanceId(1)); }

    void TearDown() override { trackingPlayer.reset(); }

    std::unique_ptr<CameraTrackingTestPlayer> trackingPlayer;
};

// ---------- 值变化时 onCameraEntityChanged 被调用 ----------

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedCalledWhenValueChanges)
{
    // 从 nullopt 变为有值时，应该触发 onCameraEntityChanged
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(0).has_value()); // 旧值是 nullopt
    EXPECT_TRUE(trackingPlayer->newCameraIdAt(0).has_value());
    EXPECT_EQ(trackingPlayer->newCameraIdAt(0).value(), EntityInstanceId(42));
}

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedCalledWhenValueChangesToDifferentEntity)
{
    // 从一个实体 ID 变为另一个实体 ID 时，应该触发
    trackingPlayer->setCameraEntityId(EntityInstanceId(10));
    trackingPlayer->setCameraEntityId(EntityInstanceId(20));
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 2u);

    // 第一次：nullopt -> 10
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(0).has_value());
    EXPECT_EQ(trackingPlayer->newCameraIdAt(0).value(), EntityInstanceId(10));

    // 第二次：10 -> 20
    EXPECT_TRUE(trackingPlayer->oldCameraIdAt(1).has_value());
    EXPECT_EQ(trackingPlayer->oldCameraIdAt(1).value(), EntityInstanceId(10));
    EXPECT_EQ(trackingPlayer->newCameraIdAt(1).value(), EntityInstanceId(20));
}

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedCalledWhenValueClearedToNullopt)
{
    // 从有值变为 nullopt 时，应该触发
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    trackingPlayer->resetCameraChangeCalls(); // 清除记录

    trackingPlayer->setCameraEntityId(std::nullopt);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);
    EXPECT_TRUE(trackingPlayer->oldCameraIdAt(0).has_value());
    EXPECT_EQ(trackingPlayer->oldCameraIdAt(0).value(), EntityInstanceId(42));
    EXPECT_FALSE(trackingPlayer->newCameraIdAt(0).has_value()); // 新值是 nullopt
}

// ---------- 值未变化时 onCameraEntityChanged 不被调用（相等性守卫） ----------

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedNotCalledWhenSameValueSet)
{
    // 设置相同的实体 ID，不应该触发回调
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);

    // 重复设置相同的值
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u); // 仍然只调用了一次
}

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedNotCalledWhenNulloptToNullopt)
{
    // 初始值是 nullopt，再次设置 nullopt 不应该触发回调
    EXPECT_FALSE(trackingPlayer->getCameraEntityId().has_value()); // 默认就是 nullopt

    trackingPlayer->setCameraEntityId(std::nullopt);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 0u); // 不应该被调用
}

TEST_F(CameraChangedTrackingTest, OnCameraEntityChangedNotCalledAfterClearToNullopt)
{
    // 先设置值，再清除，再清除一次
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    trackingPlayer->setCameraEntityId(std::nullopt);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 2u);

    // 再次清除（已经是 nullopt）
    trackingPlayer->setCameraEntityId(std::nullopt);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 2u); // 不应该再被调用
}

// ---------- setGameMode 触发 onCameraEntityChanged ----------

TEST_F(CameraChangedTrackingTest, SetGameModeLeavingSpectatorTriggersCameraChange)
{
    // 进入旁观者模式并设置旁观目标
    trackingPlayer->setGameMode(GameMode::Spectator);
    trackingPlayer->setCameraEntityId(EntityInstanceId(100));
    trackingPlayer->resetCameraChangeCalls(); // 清除之前调用的记录

    // 离开旁观者模式，setGameMode 应该通过 setCameraEntityId(nullopt) 触发回调
    trackingPlayer->setGameMode(GameMode::Survival);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);
    EXPECT_TRUE(trackingPlayer->oldCameraIdAt(0).has_value());
    EXPECT_EQ(trackingPlayer->oldCameraIdAt(0).value(), EntityInstanceId(100));
    EXPECT_FALSE(trackingPlayer->newCameraIdAt(0).has_value()); // 清除为 nullopt
}

TEST_F(CameraChangedTrackingTest, SetGameModeLeavingSpectatorWithoutCameraDoesNotTrigger)
{
    // 进入旁观者模式但没有设置旁观目标
    trackingPlayer->setGameMode(GameMode::Spectator);
    EXPECT_FALSE(trackingPlayer->isSpectating()); // 没有旁观目标
    trackingPlayer->resetCameraChangeCalls();

    // 离开旁观者模式，因为 cameraEntityId 已经是 nullopt，
    // setGameMode 中 setCameraEntityId(nullopt) 不会触发回调
    trackingPlayer->setGameMode(GameMode::Survival);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 0u);
}

// ---------- attack 旁观者路径触发 onCameraEntityChanged ----------

TEST_F(CameraChangedTrackingTest, SpectatorAttackTriggersCameraChange)
{
    trackingPlayer->setGameMode(GameMode::Spectator);
    trackingPlayer->resetCameraChangeCalls();

    Entity target(EntityInstanceId(99), nullptr, mc::test::testEcsRegistry());
    trackingPlayer->attack(target);

    // attack 旁观者路径应该通过 setCameraEntityId 触发 onCameraEntityChanged
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(0).has_value()); // 之前没有旁观目标
    EXPECT_EQ(trackingPlayer->newCameraIdAt(0).value(), EntityInstanceId(99));
}

TEST_F(CameraChangedTrackingTest, SpectatorAttackToDifferentEntityTriggersChange)
{
    trackingPlayer->setGameMode(GameMode::Spectator);

    // 第一次攻击实体 99
    Entity target1(EntityInstanceId(99), nullptr, mc::test::testEcsRegistry());
    trackingPlayer->attack(target1);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);

    // 第二次攻击实体 100（切换目标）
    Entity target2(EntityInstanceId(100), nullptr, mc::test::testEcsRegistry());
    trackingPlayer->attack(target2);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 2u);

    // 验证第二次调用的参数
    EXPECT_EQ(trackingPlayer->oldCameraIdAt(1).value(), EntityInstanceId(99));
    EXPECT_EQ(trackingPlayer->newCameraIdAt(1).value(), EntityInstanceId(100));
}

TEST_F(CameraChangedTrackingTest, SpectatorAttackSameEntityDoesNotTriggerChange)
{
    trackingPlayer->setGameMode(GameMode::Spectator);

    // 第一次攻击实体 99
    Entity target(EntityInstanceId(99), nullptr, mc::test::testEcsRegistry());
    trackingPlayer->attack(target);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);

    // 再次攻击同一实体（值相同，不应该触发回调）
    trackingPlayer->attack(target);
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u); // 仍然只调用了一次
}

// ---------- 基类 Player 的 onCameraEntityChanged 是空操作 ----------

TEST_F(PlayerSpectatorTest, BaseClassOnCameraEntityChangedIsNoOp)
{
    // 使用基类 Player 设置 setCameraEntityId，不应该崩溃或产生副作用
    // 基类的 onCameraEntityChanged() 是空操作
    player->setCameraEntityId(EntityInstanceId(42));
    EXPECT_TRUE(player->isSpectating());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(42));

    // 多次设置不同值也不应该有问题
    player->setCameraEntityId(EntityInstanceId(100));
    EXPECT_EQ(player->getCameraEntityId().value(), EntityInstanceId(100));

    player->setCameraEntityId(std::nullopt);
    EXPECT_FALSE(player->isSpectating());
}

// ---------- 连续切换和边界场景 ----------

TEST_F(CameraChangedTrackingTest, RapidCameraSwitchingRecordsAllChanges)
{
    // 快速切换多个旁观目标
    trackingPlayer->setCameraEntityId(EntityInstanceId(1));
    trackingPlayer->setCameraEntityId(EntityInstanceId(2));
    trackingPlayer->setCameraEntityId(EntityInstanceId(3));
    trackingPlayer->setCameraEntityId(std::nullopt);

    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 4u);

    // 验证每次调用的参数
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(0).has_value());
    EXPECT_EQ(trackingPlayer->newCameraIdAt(0).value(), EntityInstanceId(1));

    EXPECT_EQ(trackingPlayer->oldCameraIdAt(1).value(), EntityInstanceId(1));
    EXPECT_EQ(trackingPlayer->newCameraIdAt(1).value(), EntityInstanceId(2));

    EXPECT_EQ(trackingPlayer->oldCameraIdAt(2).value(), EntityInstanceId(2));
    EXPECT_EQ(trackingPlayer->newCameraIdAt(2).value(), EntityInstanceId(3));

    EXPECT_EQ(trackingPlayer->oldCameraIdAt(3).value(), EntityInstanceId(3));
    EXPECT_FALSE(trackingPlayer->newCameraIdAt(3).has_value());
}

TEST_F(CameraChangedTrackingTest, EntityIdZeroIsNotNullopt)
{
    // EntityInstanceId(0) 不是 nullopt，设置 EntityInstanceId(0) 应该触发从 nullopt 的变更
    trackingPlayer->setCameraEntityId(EntityInstanceId(0));
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 1u);
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(0).has_value()); // 旧值是 nullopt
    EXPECT_TRUE(trackingPlayer->newCameraIdAt(0).has_value());  // 新值是 EntityInstanceId(0)
    EXPECT_EQ(trackingPlayer->newCameraIdAt(0).value(), EntityInstanceId(0));
    EXPECT_TRUE(trackingPlayer->isSpectating()); // EntityInstanceId(0) 也是有效的旁观目标
}

TEST_F(CameraChangedTrackingTest, ClearingToNulloptAndSettingAgain)
{
    // 设置 -> 清除 -> 再设置
    trackingPlayer->setCameraEntityId(EntityInstanceId(42));
    trackingPlayer->setCameraEntityId(std::nullopt);
    trackingPlayer->setCameraEntityId(EntityInstanceId(42)); // 再设置回同一个 ID
    EXPECT_EQ(trackingPlayer->cameraChangeCallCount(), 3u);

    // 最后一次：nullopt -> 42
    EXPECT_FALSE(trackingPlayer->oldCameraIdAt(2).has_value());
    EXPECT_EQ(trackingPlayer->newCameraIdAt(2).value(), EntityInstanceId(42));
}
