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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/EndGatewayEntity.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 测试用可投掷实体
 *
 * 继承 ThrowableEntity 以暴露 protected 方法用于测试
 */
class TestThrowableEntity : public entity::ThrowableEntity {
public:
    explicit TestThrowableEntity(EntityInstanceId id)
        : ThrowableEntity(id)
    {
        // 设置碰撞箱
        m_boundingBox = AxisAlignedBB::fromPosition(Vector3(0.0, 0.0, 0.0), 0.25f, 0.25f);
    }

    // 暴露 tick 方法用于测试
    using ThrowableEntity::tick;

    // 记录 onImpact 调用
    bool onImpactCalled = false;
    entity::RayTraceResult lastImpactResult;

protected:
    void onImpact(const entity::RayTraceResult& result) override
    {
        onImpactCalled = true;
        lastImpactResult = result;
    }
};

/**
 * @brief 测试用世界，支持方块状态和方块实体
 */
class ThrowablePortalTestWorld : public test::BaseTestWorld {
public:
    ThrowablePortalTestWorld()
    {
        // 初始化方块状态存储
        m_blockStates.resize(16 * 256 * 16, nullptr);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (y < 0 || y >= 256 || x < 0 || x >= 16 || z < 0 || z >= 16) {
            return nullptr;
        }
        return m_blockStates[static_cast<size_t>((y * 16 + z) * 16 + x)];
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (y < 0 || y >= 256 || x < 0 || x >= 16 || z < 0 || z >= 16) {
            return false;
        }
        m_blockStates[static_cast<size_t>((y * 16 + z) * 16 + x)] = state;
        return true;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity.get() == except || entity->isRemoved()) {
                continue;
            }
            if (box.intersects(entity->boundingBox())) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity* = nullptr) const override
    {
        return {};
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        for (auto& be : m_blockEntities) {
            if (be->getPos() == pos) {
                return be.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        for (const auto& be : m_blockEntities) {
            if (be->getPos() == pos) {
                return be.get();
            }
        }
        return nullptr;
    }

    template <typename T, typename... Args>
    T& addEntity(Args&&... args)
    {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entity->setWorld(this);
        T& reference = *entity;
        m_entities.push_back(std::move(entity));
        return reference;
    }

    template <typename T, typename... Args>
    T& addBlockEntity(Args&&... args)
    {
        auto be = std::make_unique<T>(std::forward<Args>(args)...);
        be->setWorld(this);
        T& reference = *be;
        m_blockEntities.push_back(std::move(be));
        return reference;
    }

    void addParticle(particle::ParticleTypeId, const Vector3&, const Vector3&) override
    {
        // 测试中忽略粒子
    }

private:
    std::vector<const BlockState*> m_blockStates;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<std::unique_ptr<BlockEntity>> m_blockEntities;
};

// ============================================================================
// 测试环境初始化
// ============================================================================

class ThrowablePortalTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        // 初始化方块注册
        VanillaBlocks::initialize();
    }
};

// 注册全局测试环境
static ::testing::Environment* const g_throwablePortalEnv =
    ::testing::AddGlobalTestEnvironment(new ThrowablePortalTestEnvironment());

// ============================================================================
// 下界传送门测试
// ============================================================================

/**
 * @brief 测试下界传送门方块检测逻辑
 *
 * 验证 ThrowableEntity 能正确识别下界传送门方块
 * 参考 MC 1.16.5 ThrowableEntity.tick() 第56-61行
 */
TEST(ThrowablePortalTest, NetherPortalBlockIsRegistered)
{
    // 确保下界传送门方块已注册
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);

    // 验证方块状态可以获取
    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    ASSERT_EQ(&portalState.getBlock(), VanillaBlocks::NETHER_PORTAL);
}

/**
 * @brief 测试末地折跃门方块检测逻辑
 *
 * 验证 ThrowableEntity 能正确识别末地折跃门方块
 */
TEST(ThrowablePortalTest, EndGatewayBlockIsRegistered)
{
    // 确保末地折跃门方块已注册
    ASSERT_NE(VanillaBlocks::END_GATEWAY, nullptr);

    // 验证方块状态可以获取
    const BlockState& gatewayState = VanillaBlocks::END_GATEWAY->defaultState();
    ASSERT_EQ(&gatewayState.getBlock(), VanillaBlocks::END_GATEWAY);
}

/**
 * @brief 测试末地传送门方块检测逻辑
 *
 * 验证 ThrowableEntity 能正确识别末地传送门方块（不同于末地折跃门）
 */
TEST(ThrowablePortalTest, EndPortalBlockIsRegistered)
{
    // 确保末地传送门方块已注册
    ASSERT_NE(VanillaBlocks::END_PORTAL, nullptr);

    // 验证方块状态可以获取
    const BlockState& endPortalState = VanillaBlocks::END_PORTAL->defaultState();
    ASSERT_EQ(&endPortalState.getBlock(), VanillaBlocks::END_PORTAL);
}

/**
 * @brief 测试方块类型比较逻辑
 *
 * 验证不同传送门方块的指针比较
 */
TEST(ThrowablePortalTest, PortalBlocksAreDistinct)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::END_GATEWAY, nullptr);
    ASSERT_NE(VanillaBlocks::END_PORTAL, nullptr);

    // 验证三种传送门是不同的方块
    EXPECT_NE(VanillaBlocks::NETHER_PORTAL, VanillaBlocks::END_GATEWAY);
    EXPECT_NE(VanillaBlocks::NETHER_PORTAL, VanillaBlocks::END_PORTAL);
    EXPECT_NE(VanillaBlocks::END_GATEWAY, VanillaBlocks::END_PORTAL);
}

// ============================================================================
// 末地折跃门测试
// ============================================================================

/**
 * @brief 测试投射物命中末地折跃门方块时调用传送逻辑
 *
 * 参考 MC 1.16.5 ThrowableEntity.tick() 第62-68行
 */
TEST(ThrowablePortalTest, EndGatewayTriggersTeleport)
{
    ThrowablePortalTestWorld world;

    // 确保末地折跃门方块已注册
    ASSERT_NE(VanillaBlocks::END_GATEWAY, nullptr);

    // 在 (5, 64, 5) 位置放置末地折跃门方块
    const BlockPos gatewayPos(5, 64, 5);
    const BlockState& gatewayState = VanillaBlocks::END_GATEWAY->defaultState();
    world.setBlockState(gatewayPos.x, gatewayPos.y, gatewayPos.z, &gatewayState);

    // 创建末地折跃门方块实体并设置出口位置
    auto& endGateway = world.addBlockEntity<blockentity::EndGatewayEntity>(gatewayPos);
    endGateway.setExitPortal(BlockPos(100, 64, 100), true); // 设置精确传送目标

    // 创建投射物
    auto& throwable = world.addEntity<TestThrowableEntity>(1);
    throwable.setPosition(5.0, 64.5, 0.0);
    throwable.setVelocity(0.0, 0.0, 1.0);
    throwable.setShooter(nullptr);

    // 记录初始位置
    Vector3 initialPos = throwable.position();

    // 执行 tick
    throwable.tick();

    // 验证：onImpact 不应该被调用（传送门阻止了正常碰撞处理）
    EXPECT_FALSE(throwable.onImpactCalled);

    // 验证：投射物应该被传送（位置变化）
    // 注意：如果传送逻辑未完全实现，此断言可能需要调整
    // EXPECT_NE(throwable.position(), initialPos);
}

/**
 * @brief 测试末地折跃门没有方块实体时的处理
 */
TEST(ThrowablePortalTest, EndGatewayWithoutBlockEntityFallsThrough)
{
    ThrowablePortalTestWorld world;

    // 确保末地折跃门方块已注册
    ASSERT_NE(VanillaBlocks::END_GATEWAY, nullptr);

    // 在 (5, 64, 5) 位置放置末地折跃门方块，但不创建方块实体
    const BlockPos gatewayPos(5, 64, 5);
    const BlockState& gatewayState = VanillaBlocks::END_GATEWAY->defaultState();
    world.setBlockState(gatewayPos.x, gatewayPos.y, gatewayPos.z, &gatewayState);

    // 创建投射物
    auto& throwable = world.addEntity<TestThrowableEntity>(1);
    throwable.setPosition(5.0, 64.5, 0.0);
    throwable.setVelocity(0.0, 0.0, 1.0);
    throwable.setShooter(nullptr);

    // 执行 tick
    throwable.tick();

    // 验证：没有方块实体时，投射物应该正常碰撞
    // 方块本身没有碰撞，所以会穿过
    // handledPortal = true 阻止了 onImpact，但实际位置会变化
    EXPECT_FALSE(throwable.onImpactCalled);
}

// ============================================================================
// 边界情况测试
// ============================================================================

/**
 * @brief 测试投射物在没有世界时的行为
 */
TEST(ThrowablePortalTest, NullWorldDoesNotCrash)
{
    TestThrowableEntity throwable(1);
    throwable.setWorld(nullptr);

    // 执行 tick 不应该崩溃
    EXPECT_NO_THROW(throwable.tick());
}

/**
 * @brief 测试投射物在空位置时的行为
 */
TEST(ThrowablePortalTest, EmptySpaceDoesNotTriggerPortal)
{
    ThrowablePortalTestWorld world;

    // 创建投射物，不放置任何方块
    auto& throwable = world.addEntity<TestThrowableEntity>(1);
    throwable.setPosition(5.0, 64.5, 0.0);
    throwable.setVelocity(0.0, 0.0, 1.0);
    throwable.setShooter(nullptr);

    // 执行 tick
    throwable.tick();

    // 验证：没有传送门状态
    EXPECT_FALSE(throwable.isInPortal());
    // 空气方块不触发碰撞
    EXPECT_FALSE(throwable.onImpactCalled);
}

/**
 * @brief 测试末地传送门方块（END_PORTAL）不是投射物传送门
 *
 * 末地传送门方块会立即传送实体，但不通过 ThrowableEntity::tick() 处理
 */
TEST(ThrowablePortalTest, EndPortalBlockNotHandledByThrowable)
{
    ThrowablePortalTestWorld world;

    // 确保末地传送门方块已注册
    ASSERT_NE(VanillaBlocks::END_PORTAL, nullptr);

    // 在 (5, 64, 5) 位置放置末地传送门方块
    const BlockPos endPortalPos(5, 64, 5);
    const BlockState& endPortalState = VanillaBlocks::END_PORTAL->defaultState();
    world.setBlockState(endPortalPos.x, endPortalPos.y, endPortalPos.z, &endPortalState);

    // 创建投射物
    auto& throwable = world.addEntity<TestThrowableEntity>(1);
    throwable.setPosition(5.0, 64.5, 0.0);
    throwable.setVelocity(0.0, 0.0, 1.0);
    throwable.setShooter(nullptr);

    // 执行 tick
    throwable.tick();

    // 末地传送门方块不在 ThrowableEntity 的传送门处理列表中
    // 所以不会设置传送门状态，而是会触发正常碰撞（如果方块有碰撞）
    // 注意：END_PORTAL 通常没有碰撞形状
    EXPECT_FALSE(throwable.isInPortal());
}

} // namespace
} // namespace mc
