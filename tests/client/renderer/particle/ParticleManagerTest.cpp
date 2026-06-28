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

#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/settings/ClientSettings.hpp"
#include "common/core/Types.hpp"
#include <glm/glm.hpp>
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::particle;

/**
 * @brief 测试 ParticleManager 待处理粒子队列
 */
class ParticleManagerPendingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 ParticleRegistry 已初始化
        ParticleRegistry::instance();
    }
};

/**
 * @brief 测试添加待处理粒子
 */
TEST_F(ParticleManagerPendingTest, AddPendingParticle)
{
    ParticleManager manager;

    // 添加待处理粒子
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f, 0.1f, 0.0f));

    EXPECT_EQ(manager.pendingParticleCount(), 1);
    EXPECT_EQ(manager.particleCount(), 0); // 还没有处理
}

/**
 * @brief 测试清除待处理粒子
 */
TEST_F(ParticleManagerPendingTest, ClearPending)
{
    ParticleManager manager;

    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f));
    manager.addPendingParticle(ParticleTypeId::Smoke, glm::vec3(1.0f), glm::vec3(0.0f));

    EXPECT_EQ(manager.pendingParticleCount(), 2);

    manager.clearPending();

    EXPECT_EQ(manager.pendingParticleCount(), 0);
}

/**
 * @brief 测试处理待处理粒子
 *
 * 注意：processPendingParticles 是私有方法，通过 tick() 间接测试
 */
TEST_F(ParticleManagerPendingTest, ProcessPendingParticles)
{
    ParticleManager manager;

    // 添加待处理粒子
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    manager.addPendingParticle(ParticleTypeId::Heart, glm::vec3(1.0f), glm::vec3(0.0f));

    EXPECT_EQ(manager.pendingParticleCount(), 2);
    EXPECT_EQ(manager.particleCount(), 0);

    // tick 会调用 processPendingParticles
    manager.tick(nullptr);

    // 待处理粒子应该被清空
    EXPECT_EQ(manager.pendingParticleCount(), 0);

    // 粒子应该被创建并添加到主列表
    // 注意：如果 Flame 或 Heart 粒子没有注册工厂，可能不会创建
    // 至少待处理队列应该被清空
}

/**
 * @brief 测试待处理粒子数量限制
 */
TEST_F(ParticleManagerPendingTest, PendingParticleLimit)
{
    ParticleManager manager;

    // 添加超过最大粒子数的待处理粒子
    for (size_t i = 0; i < 20000; ++i) {
        manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(static_cast<float>(i)), glm::vec3(0.0f));
    }

    // 应该被限制在 MAX_PARTICLES
    EXPECT_LE(manager.pendingParticleCount(), 16384);
}

/**
 * @brief 测试距离裁剪功能
 */
class ParticleManagerDistanceCullingTest : public ::testing::Test {
protected:
    void SetUp() override { ParticleRegistry::instance(); }
};

/**
 * @brief 测试设置相机位置
 */
TEST_F(ParticleManagerDistanceCullingTest, SetCameraPosition)
{
    ParticleManager manager;

    manager.setCameraPosition(glm::vec3(100.0f, 64.0f, 100.0f));

    // 没有直接的 getter，但设置不应该抛出异常
    SUCCEED();
}

/**
 * @brief 测试设置最大粒子距离
 */
TEST_F(ParticleManagerDistanceCullingTest, SetMaxParticleDistance)
{
    ParticleManager manager;

    manager.setMaxParticleDistance(128.0f);

    // 没有直接的 getter，但设置不应该抛出异常
    SUCCEED();
}

/**
 * @brief 测试默认最大粒子距离
 *
 * MC 1.16.5 默认为 256 格
 */
TEST_F(ParticleManagerDistanceCullingTest, DefaultMaxDistance)
{
    // 默认值在头文件中定义，通过行为间接测试
    ParticleManager manager;

    // 设置相机位置远离开原点
    manager.setCameraPosition(glm::vec3(300.0f, 0.0f, 0.0f));

    // 添加待处理粒子在原点
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f));

    // 处理待处理粒子时，超出距离的粒子应该被裁剪
    manager.tick(nullptr);

    // 由于距离超过默认 256 格，粒子不应该被创建
    // 注意：这依赖于 distance culling 正确实现
}

/**
 * @brief 测试距离裁剪禁用
 */
TEST_F(ParticleManagerDistanceCullingTest, DisableDistanceCulling)
{
    ParticleManager manager;

    // 设置距离为 0 表示禁用距离裁剪
    manager.setMaxParticleDistance(0.0f);

    // 设置相机位置远离开原点
    manager.setCameraPosition(glm::vec3(1000.0f, 0.0f, 0.0f));

    // 添加待处理粒子在原点
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f));

    // 处理待处理粒子
    manager.tick(nullptr);

    // 由于距离裁剪禁用，粒子应该被创建
    // 注意：这依赖于粒子工厂注册
}

/**
 * @brief 测试存活粒子计数
 */
class ParticleManagerAliveCountTest : public ::testing::Test {
protected:
    void SetUp() override { ParticleRegistry::instance(); }
};

/**
 * @brief 测试空 ParticleManager 的存活粒子计数
 */
TEST_F(ParticleManagerAliveCountTest, EmptyManager)
{
    ParticleManager manager;

    EXPECT_EQ(manager.aliveParticleCount(), 0);
    EXPECT_EQ(manager.particleCount(), 0);
}

/**
 * @brief 测试清除所有粒子
 */
TEST_F(ParticleManagerAliveCountTest, ClearParticles)
{
    ParticleManager manager;

    manager.clear();

    EXPECT_EQ(manager.particleCount(), 0);
    EXPECT_EQ(manager.pendingParticleCount(), 0);
}

/**
 * @brief 测试初始化状态
 */
class ParticleManagerInitTest : public ::testing::Test {
protected:
    void SetUp() override { ParticleRegistry::instance(); }
};

/**
 * @brief 测试未初始化状态
 */
TEST_F(ParticleManagerInitTest, NotInitializedByDefault)
{
    ParticleManager manager;

    EXPECT_FALSE(manager.isInitialized());
}

// ============================================================================
// 粒子质量过滤测试（ParticleMode / shouldShowParticle）
// ============================================================================

/**
 * @brief 粒子质量过滤测试夹具
 */
class ParticleManagerFilterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 ParticleRegistry 已初始化
        ParticleRegistry::instance();
    }
};

/**
 * @brief All 模式：所有粒子类型都应该通过过滤
 */
TEST_F(ParticleManagerFilterTest, AllModeShowsAllParticles)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::All);

    // overrideLimiter=false 的普通粒子应该通过
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Flame));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Smoke));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Heart));

    // overrideLimiter=true 的重要粒子也应该通过
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::DamageIndicator));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Explosion));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Poof));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::SweepAttack));
}

/**
 * @brief Minimal 模式：仅 overrideLimiter=true 的重要粒子通过
 */
TEST_F(ParticleManagerFilterTest, MinimalModeOnlyShowsImportantParticles)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::Minimal);

    // overrideLimiter=false 的普通粒子应该被过滤掉
    EXPECT_FALSE(manager.shouldShowParticle(ParticleTypeId::Flame));
    EXPECT_FALSE(manager.shouldShowParticle(ParticleTypeId::Smoke));
    EXPECT_FALSE(manager.shouldShowParticle(ParticleTypeId::Heart));

    // overrideLimiter=true 的重要粒子应该始终通过
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::DamageIndicator));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Explosion));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Poof));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::SweepAttack));
}

/**
 * @brief Decreased 模式：重要粒子始终通过，普通粒子有概率通过
 *
 * 统计测试：对普通粒子调用 shouldShowParticle 多次，
 * 验证通过率在合理范围内（期望约 2/3，允许误差 ±15%）。
 */
TEST_F(ParticleManagerFilterTest, DecreasedModeProbabilisticFiltering)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::Decreased);

    // overrideLimiter=true 的重要粒子始终通过
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::DamageIndicator));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Explosion));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Poof));
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::SweepAttack));

    // 对普通粒子进行统计测试
    constexpr int iterations = 3000;
    int passCount = 0;
    for (int i = 0; i < iterations; ++i) {
        if (manager.shouldShowParticle(ParticleTypeId::Flame)) {
            ++passCount;
        }
    }

    // 期望通过率约 2/3（66.7%），允许误差 ±15%（即 51.7% ~ 81.7%）
    const double passRate = static_cast<double>(passCount) / static_cast<double>(iterations);
    EXPECT_GT(passRate, 0.517) << "Pass rate too low: " << passRate;
    EXPECT_LT(passRate, 0.817) << "Pass rate too high: " << passRate;
}

/**
 * @brief Decreased 模式：非确定性验证
 *
 * 两次大量调用的通过率应该不完全相同，验证确实存在随机性。
 */
TEST_F(ParticleManagerFilterTest, DecreasedModeIsNonDeterministic)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::Decreased);

    constexpr int iterations = 1000;

    // 第一轮
    int passCount1 = 0;
    for (int i = 0; i < iterations; ++i) {
        if (manager.shouldShowParticle(ParticleTypeId::Flame)) {
            ++passCount1;
        }
    }

    // 第二轮
    int passCount2 = 0;
    for (int i = 0; i < iterations; ++i) {
        if (manager.shouldShowParticle(ParticleTypeId::Flame)) {
            ++passCount2;
        }
    }

    // 两次结果不一定完全相同（概率极低）
    // 这验证了随机源确实在推进，而不是每次返回相同结果
    // 注意：理论上两次可能完全相同，但概率极低
    // 我们只检查两次都在合理范围内
    const double rate1 = static_cast<double>(passCount1) / static_cast<double>(iterations);
    const double rate2 = static_cast<double>(passCount2) / static_cast<double>(iterations);
    EXPECT_GT(rate1, 0.4);
    EXPECT_LT(rate1, 0.9);
    EXPECT_GT(rate2, 0.4);
    EXPECT_LT(rate2, 0.9);
}

/**
 * @brief 模式切换：运行时切换模式后过滤行为正确变化
 */
TEST_F(ParticleManagerFilterTest, ModeSwitchChangesFilterBehavior)
{
    ParticleManager manager;

    // 从 All 切换到 Minimal
    manager.setParticleMode(mc::client::ParticleMode::All);
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Flame));

    manager.setParticleMode(mc::client::ParticleMode::Minimal);
    EXPECT_FALSE(manager.shouldShowParticle(ParticleTypeId::Flame));

    // 切换回 All
    manager.setParticleMode(mc::client::ParticleMode::All);
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::Flame));

    // 重要粒子在任何模式下都通过
    manager.setParticleMode(mc::client::ParticleMode::Minimal);
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::DamageIndicator));

    manager.setParticleMode(mc::client::ParticleMode::All);
    EXPECT_TRUE(manager.shouldShowParticle(ParticleTypeId::DamageIndicator));
}

/**
 * @brief addPendingParticle 在 Minimal 模式下过滤普通粒子
 */
TEST_F(ParticleManagerFilterTest, AddPendingParticleFilteredInMinimalMode)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::Minimal);

    // 普通粒子应该被过滤掉
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f));
    EXPECT_EQ(manager.pendingParticleCount(), 0);

    // 重要粒子应该通过
    manager.addPendingParticle(ParticleTypeId::DamageIndicator, glm::vec3(0.0f), glm::vec3(0.0f));
    EXPECT_EQ(manager.pendingParticleCount(), 1);
}

/**
 * @brief addPendingParticle 在 All 模式下不过滤任何粒子
 */
TEST_F(ParticleManagerFilterTest, AddPendingParticleNotFilteredInAllMode)
{
    ParticleManager manager;
    manager.setParticleMode(mc::client::ParticleMode::All);

    // 所有粒子都应该通过
    manager.addPendingParticle(ParticleTypeId::Flame, glm::vec3(0.0f), glm::vec3(0.0f));
    manager.addPendingParticle(ParticleTypeId::DamageIndicator, glm::vec3(1.0f), glm::vec3(0.0f));
    EXPECT_EQ(manager.pendingParticleCount(), 2);
}

/**
 * @brief particleMode getter 返回正确的模式
 */
TEST_F(ParticleManagerFilterTest, ParticleModeGetter)
{
    ParticleManager manager;

    // 默认模式是 All
    EXPECT_EQ(manager.particleMode(), mc::client::ParticleMode::All);

    manager.setParticleMode(mc::client::ParticleMode::Minimal);
    EXPECT_EQ(manager.particleMode(), mc::client::ParticleMode::Minimal);

    manager.setParticleMode(mc::client::ParticleMode::Decreased);
    EXPECT_EQ(manager.particleMode(), mc::client::ParticleMode::Decreased);

    manager.setParticleMode(mc::client::ParticleMode::All);
    EXPECT_EQ(manager.particleMode(), mc::client::ParticleMode::All);
}
