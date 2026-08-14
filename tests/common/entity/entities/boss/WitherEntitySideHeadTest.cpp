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
 * @file WitherEntitySideHeadTest.cpp
 * @brief WitherEntity 侧头朝向计算单元测试
 *
 * 验证 WitherEntity::_updateSideHeadRotations 与 _rotLerp 的行为，
 * 覆盖 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 循环的侧头朝向计算：
 *
 * - rotlerp 限速：yaw 最大 10°/tick，pitch 最大 40°/tick
 * - 无目标时 yaw 逐步回正到 renderYawOffset（bodyRot），pitch 不变
 * - 有目标时根据 dx/dy/dz 计算 targetYaw/targetPitch 并 rotlerp 逼近
 * - 目标实体不存在（targetId > 0 但 world 中找不到）走无目标分支
 * - targetId <= 0（无目标）走无目标分支
 * - 多 tick 收敛：大角度差需要多 tick 逐步逼近
 * - prev 侧头角度在 aiStep() 中正确备份
 *
 * 对应 MC 1.21.11 WitherBoss.aiStep()：
 *   super.aiStep();
 *   for (i in 0..1) { yRotOHeads[i] = yRotHeads[i]; xRotOHeads[i] = xRotHeads[i]; }
 *   for (j in 0..1) {
 *       k = getAlternativeTarget(j+1);
 *       entity1 = (k > 0) ? level.getEntity(k) : null;
 *       if (entity1 != null) {
 *           // 计算 targetYaw/targetPitch，rotlerp 逼近
 *           xRotHeads[j] = rotlerp(xRotHeads[j], targetPitch, 40);
 *           yRotHeads[j] = rotlerp(yRotHeads[j], targetYaw, 10);
 *       } else {
 *           yRotHeads[j] = rotlerp(yRotHeads[j], yBodyRot, 10);
 *       }
 *   }
 */

#include <gtest/gtest.h>

#include <cmath>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/FlyingMovementController.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World 类（与 WitherEntityTest.cpp 中相同结构）
 *
 * 支持 spawnEntity / getEntity，用于侧头目标追踪测试。
 * tickManager() 抛出异常——WitherEntity::aiStep() 不会触发 tickManager。
 */
class WitherSideHeadTestWorld final : public mc::test::BaseTestWorld {
public:
    WitherSideHeadTestWorld() { VanillaBlocks::initialize(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        BlockPos pos(x, y, z);
        auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        BlockPos pos(x, y, z);
        if (state) {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        } else {
            m_blocks.erase(pos);
        }
        return true;
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const override
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& aabb, const Entity* exclude) const override
    {
        std::vector<Entity*> result;
        for (auto& entity : m_entities) {
            if (entity.get() != exclude && entity->boundingBox().intersects(aabb)) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    Entity* getEntity(EntityInstanceId id) override
    {
        for (auto& entity : m_entities) {
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

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) return EntityInstanceId(0);
        EntityInstanceId id = m_nextEntityId;
        m_nextEntityId = EntityInstanceId(static_cast<u32>(m_nextEntityId) + 1);
        entity->setId(id);
        entity->setWorld(this);
        m_entities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] i64 dayTime() const override { return 6000; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WitherSideHeadTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WitherSideHeadTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_entities;
    EntityInstanceId m_nextEntityId = EntityInstanceId(1);
    u64 m_currentTick = 0;
};

/**
 * @brief 测试夹具
 */
class WitherEntitySideHeadTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_world = std::make_unique<WitherSideHeadTestWorld>();
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<WitherSideHeadTestWorld> m_world;
};

// ============================================================================
// 侧头角度初始状态测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, InitialState_SideHeadAnglesAreZero)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());

    // 初始状态：两侧头 pitch/yaw 均为 0
    EXPECT_FLOAT_EQ(wither.sideHeadPitch(0), 0.0f);
    EXPECT_FLOAT_EQ(wither.sideHeadYaw(0), 0.0f);
    EXPECT_FLOAT_EQ(wither.sideHeadPitch(1), 0.0f);
    EXPECT_FLOAT_EQ(wither.sideHeadYaw(1), 0.0f);

    // prev 也为 0
    EXPECT_FLOAT_EQ(wither.prevSideHeadPitch(0), 0.0f);
    EXPECT_FLOAT_EQ(wither.prevSideHeadYaw(0), 0.0f);
    EXPECT_FLOAT_EQ(wither.prevSideHeadPitch(1), 0.0f);
    EXPECT_FLOAT_EQ(wither.prevSideHeadYaw(1), 0.0f);
}

// ============================================================================
// 无目标时侧头 yaw 回正到 bodyRot 测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, NoTarget_YawLerpsTowardBodyRot)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));

    // 设置 bodyRot = 90°，侧头初始 yaw = 0°
    // 无目标时 yaw 以 10°/tick 逼近 bodyRot
    wither.setRenderYawOffset(90.0f);

    wither.aiStep();

    // 第一次 tick：yaw 应该从 0 朝 90 移动 10°，结果为 10°
    EXPECT_NEAR(wither.sideHeadYaw(0), 10.0f, 0.001f);
    EXPECT_NEAR(wither.sideHeadYaw(1), 10.0f, 0.001f);
}

TEST_F(WitherEntitySideHeadTest, NoTarget_PitchUnchanged)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));

    // 无目标时 pitch 不变化（MC 行为）
    wither.setRenderYawOffset(0.0f);

    // 记录初始 pitch
    f32 initialPitch0 = wither.sideHeadPitch(0);
    f32 initialPitch1 = wither.sideHeadPitch(1);

    wither.aiStep();

    // pitch 应保持不变
    EXPECT_FLOAT_EQ(wither.sideHeadPitch(0), initialPitch0);
    EXPECT_FLOAT_EQ(wither.sideHeadPitch(1), initialPitch1);
}

TEST_F(WitherEntitySideHeadTest, NoTarget_MultipleTicks_ConvergesToBodyRot)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));

    // bodyRot = 45°，初始 yaw = 0°
    // 10°/tick 需要 5 tick 才能到达 45°（0→10→20→30→40→45）
    wither.setRenderYawOffset(45.0f);

    for (i32 i = 0; i < 4; ++i) {
        wither.aiStep();
    }
    // 4 tick 后 yaw = 40°
    EXPECT_NEAR(wither.sideHeadYaw(0), 40.0f, 0.001f);

    // 第 5 tick：diff = 45-40 = 5 < 10，直接到 45
    wither.aiStep();
    EXPECT_NEAR(wither.sideHeadYaw(0), 45.0f, 0.001f);
    EXPECT_NEAR(wither.sideHeadYaw(1), 45.0f, 0.001f);
}

// ============================================================================
// 有目标时侧头朝向计算测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, WithTarget_YawConvergesTowardTarget)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // 生成一个目标实体放在 wither 正前方（+Z 方向）20 格处
    // bodyRot=0，侧头1（左头）位置 = (0+cos(0)*1.3, 64+2.2, 0+sin(0)*1.3) = (1.3, 66.2, 0)
    // 目标 eyeY = targetY + eyeHeight(WitherEntity=2.0)
    // 让 eyeY == headY → targetY = 66.2 - 2.0 = 64.2
    // 目标在 (1.3, 64.2, 20) 时，dx=0, dz=20
    // targetYaw = atan2(20, 0) * 180/PI - 90 = 90 - 90 = 0
    auto target = std::make_unique<entity::WitherEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    target->setPosition(Vector3(1.3, 64.2, 20.0));
    EntityInstanceId targetId = m_world->spawnEntity(std::move(target));

    // 设置侧头1（左头，index 0）的目标
    wither.updateWatchedTargetId(1, static_cast<i32>(static_cast<u32>(targetId)));

    wither.aiStep();

    // 侧头1（index 0）yaw 应该朝向 0°（正前方）
    // 由于初始 yaw=0，targetYaw=0，diff=0，yaw 保持 0
    EXPECT_NEAR(wither.sideHeadYaw(0), 0.0f, 0.01f);
}

TEST_F(WitherEntitySideHeadTest, WithTarget_PitchConvergesTowardTarget)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // 目标在侧头1正下方，让 pitch 收敛到正值（向下看，但 atan2 取反为正）
    // 侧头1 位置 = (1.3, 66.2, 0)，目标在 (1.3, 60.0, 0.1)
    // WitherEntity eyeHeight = 2.0，eyeY = 60 + 2.0 = 62.0
    // dx=0, dy = 62.0 - 66.2 = -4.2, dz=0.1
    // horizontalDist = sqrt(0 + 0.01) = 0.1
    // targetPitch = -(atan2(-4.2, 0.1) * 180/PI) = -(-88.6) = 88.6°
    auto target = std::make_unique<entity::WitherEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    target->setPosition(Vector3(1.3, 60.0, 0.1));
    EntityInstanceId targetId = m_world->spawnEntity(std::move(target));

    wither.updateWatchedTargetId(1, static_cast<i32>(static_cast<u32>(targetId)));

    wither.aiStep();

    // pitch 应该是负值（向下看），且朝目标逼近
    // targetPitch = -(atan2(dy, horizontalDist) * 180/PI)
    // dy = (60 + 2.2) - 66.2 = -4.0, horizontalDist = sqrt(0 + 0.01) = 0.1
    // targetPitch = -(atan2(-4, 0.1) * 180/PI) = -(-88.57) = 88.57°
    // 但 pitch 限速 40°/tick，所以第一次 tick 后 pitch = 0 + 40 = 40°（朝 88.57 逼近）
    EXPECT_GT(wither.sideHeadPitch(0), 0.0f) << "pitch 应向正值方向移动（目标在下，但 atan2(dy, hd) 为负，取反为正）";
    EXPECT_NEAR(wither.sideHeadPitch(0), 40.0f, 0.01f) << "pitch 限速 40°/tick";
}

TEST_F(WitherEntitySideHeadTest, WithTarget_YawRateLimitedTo10DegreesPerTick)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // 目标在侧头1的正后方（-Z 方向），targetYaw ≈ -180°
    // 侧头1 位置 = (1.3, 66.2, 0)
    // 目标在 (1.3, 64.2, -20) 时（eyeY=64.2+2.0=66.2==headY），dx=0, dz=-20
    // targetYaw = atan2(-20, 0) * 180/PI - 90 = -90 - 90 = -180°
    auto target = std::make_unique<entity::WitherEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    target->setPosition(Vector3(1.3, 64.2, -20.0));
    EntityInstanceId targetId = m_world->spawnEntity(std::move(target));

    wither.updateWatchedTargetId(1, static_cast<i32>(static_cast<u32>(targetId)));

    wither.aiStep();

    // yaw 从 0 朝 -180 逼近，限速 10°/tick
    // 第一次 tick：yaw = 0 + (-10) = -10°
    EXPECT_NEAR(wither.sideHeadYaw(0), -10.0f, 0.01f) << "yaw 限速 10°/tick";
}

TEST_F(WitherEntitySideHeadTest, WithTarget_MultipleTicks_ConvergesToTargetYaw)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // 目标在 -Z 方向，targetYaw = -180°
    auto target = std::make_unique<entity::WitherEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    target->setPosition(Vector3(1.3, 64.2, -20.0));
    EntityInstanceId targetId = m_world->spawnEntity(std::move(target));

    wither.updateWatchedTargetId(1, static_cast<i32>(static_cast<u32>(targetId)));

    // 推进 20 tick，应该足以收敛到 -180°（10°/tick * 18 = 180°）
    for (i32 i = 0; i < 20; ++i) {
        wither.aiStep();
    }

    // yaw 应该接近 -180°
    EXPECT_NEAR(wither.sideHeadYaw(0), -180.0f, 1.0f);
}

// ============================================================================
// 目标实体不存在的边界场景
// ============================================================================

TEST_F(WitherEntitySideHeadTest, TargetIdPositive_ButEntityNotFound_FallsBackToNoTarget)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(30.0f);

    // 设置一个不存在的 targetId（世界中没有此实体）
    // MC: k > 0 但 level.getEntity(k) 返回 null → 走 else 分支
    wither.updateWatchedTargetId(1, 99999);

    wither.aiStep();

    // 无目标分支：yaw 朝 bodyRot=30° 逼近，限速 10°/tick
    EXPECT_NEAR(wither.sideHeadYaw(0), 10.0f, 0.01f) << "目标不存在时应走无目标分支，yaw 朝 bodyRot 逼近";
}

TEST_F(WitherEntitySideHeadTest, TargetIdZero_FallsBackToNoTarget)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(20.0f);

    // targetId = 0（默认无目标）
    EXPECT_EQ(wither.getWatchedTargetId(1), 0);

    wither.aiStep();

    // 无目标分支：yaw 朝 bodyRot=20° 逼近
    EXPECT_NEAR(wither.sideHeadYaw(0), 10.0f, 0.01f);
}

TEST_F(WitherEntitySideHeadTest, NoWorld_DoesNotCrash)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 故意不设置 world
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // world() == nullptr，不应崩溃
    EXPECT_NO_THROW({ wither.aiStep(); });

    // 无 world 时 targetEntity == nullptr，走无目标分支
    // yaw 保持 0（bodyRot=0，diff=0）
    EXPECT_FLOAT_EQ(wither.sideHeadYaw(0), 0.0f);
}

// ============================================================================
// prev 侧头角度备份测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, PrevAngles_BackupBeforeUpdate)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(30.0f);

    // 初始 prev = 0
    EXPECT_FLOAT_EQ(wither.prevSideHeadYaw(0), 0.0f);

    wither.aiStep();

    // 第一次 tick 后：
    // prev = 0（更新前的值）
    // current = 10（朝 bodyRot=30 逼近 10°）
    EXPECT_FLOAT_EQ(wither.prevSideHeadYaw(0), 0.0f) << "prev 应保存更新前的值";
    EXPECT_NEAR(wither.sideHeadYaw(0), 10.0f, 0.01f);

    // 第二次 tick
    wither.aiStep();

    // prev = 10（上一次的 current）
    EXPECT_NEAR(wither.prevSideHeadYaw(0), 10.0f, 0.01f);
    EXPECT_NEAR(wither.sideHeadYaw(0), 20.0f, 0.01f);
}

// ============================================================================
// 两侧头独立测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, TwoSideHeads_IndependentTargets)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));
    wither.setRenderYawOffset(0.0f);

    // 侧头1（左头，j=0）目标在 +Z（targetYaw≈0）
    auto target1 = std::make_unique<entity::WitherEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    target1->setPosition(Vector3(1.3, 64.2, 20.0));
    EntityInstanceId target1Id = m_world->spawnEntity(std::move(target1));
    wither.updateWatchedTargetId(1, static_cast<i32>(static_cast<u32>(target1Id)));

    // 侧头2（右头，j=1）目标在 -Z（targetYaw≈-180）
    // 侧头2 位置 = bodyRot + 180 = 180°，(0+cos(180)*1.3, 66.2, 0+sin(180)*1.3) = (-1.3, 66.2, 0)
    auto target2 = std::make_unique<entity::WitherEntity>(EntityInstanceId(3), mc::test::testEcsRegistry());
    target2->setPosition(Vector3(-1.3, 64.2, -20.0));
    EntityInstanceId target2Id = m_world->spawnEntity(std::move(target2));
    wither.updateWatchedTargetId(2, static_cast<i32>(static_cast<u32>(target2Id)));

    wither.aiStep();

    // 侧头1 朝 +Z 方向，targetYaw=0，yaw 保持 0
    EXPECT_NEAR(wither.sideHeadYaw(0), 0.0f, 0.5f) << "侧头1 朝 +Z 目标，yaw≈0";

    // 侧头2 朝 -Z 方向，targetYaw=-180，yaw 从 0 朝 -180 逼近 10°
    EXPECT_NEAR(wither.sideHeadYaw(1), -10.0f, 0.5f) << "侧头2 朝 -Z 目标，yaw 从 0 朝 -180 逼近 10°";
}

// ============================================================================
// Wrap-around（角度环绕）测试
// ============================================================================

TEST_F(WitherEntitySideHeadTest, NoTarget_YawWrapAround_ShortestPath)
{
    entity::WitherEntity wither(EntityInstanceId(1), mc::test::testEcsRegistry());
    wither.setWorld(m_world.get());
    wither.setPosition(Vector3(0.0, 64.0, 0.0));

    // 初始 yaw = 170°，bodyRot = -170°
    // diff = wrapDegrees(-170 - 170) = wrapDegrees(-340) = 20
    // yaw = 170 + min(20, 10) = 180
    // 但 180 在 wrapDegrees 中会变成 -180？不，rotlerp 返回 from + clamped，不 wrap
    // 所以 yaw = 170 + 10 = 180
    // 注意：MC 的 rotlerp 也不 wrap 结果，所以可能超过 180

    // 由于无法直接设置 sideHeadYaw（私有字段），我们通过 bodyRot 的变化来测试 wrap
    // 先让 yaw 收敛到 bodyRot=170
    wither.setRenderYawOffset(170.0f);
    for (i32 i = 0; i < 20; ++i) {
        wither.aiStep();
    }
    // 现在 yaw 应该接近 170°
    EXPECT_NEAR(wither.sideHeadYaw(0), 170.0f, 1.0f);

    // 切换 bodyRot 到 -170°，diff = wrapDegrees(-170 - 170) = wrapDegrees(-340) = 20
    // yaw = 170 + min(20, 10) = 180
    wither.setRenderYawOffset(-170.0f);
    wither.aiStep();

    // yaw 应该朝 -170 方向（经过 +180/-180 边界），移动 10°
    // 170 + 10 = 180（不走 -340 的长路径）
    EXPECT_NEAR(wither.sideHeadYaw(0), 180.0f, 0.5f) << "wrap-around 应走最短路径（+10 而非 -350）";
}

} // namespace
} // namespace mc
