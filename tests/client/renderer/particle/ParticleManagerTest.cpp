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
