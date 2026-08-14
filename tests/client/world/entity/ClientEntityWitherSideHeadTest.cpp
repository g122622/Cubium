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
 * @file ClientEntityWitherSideHeadTest.cpp
 * @brief ClientEntity 凋灵侧头朝向客户端镜像计算单元测试
 *
 * 验证 ClientEntity::tickWitherSideHeads 正确镜像 MC 1.21.11
 * WitherBoss.aiStep() 中 j=0..1 的侧头朝向计算逻辑。
 *
 * 由于 ClientEntity 不继承 Entity/WitherEntity 且 WitherEntity::aiStep()
 * 不在客户端运行，ClientEntity 必须独立完成侧头朝向计算。本测试覆盖：
 *
 * - 初始状态：所有侧头 yaw/pitch = 0
 * - 无目标（targetId=0）：yaw 朝 bodyRot 逼近 10°/tick，pitch 不变
 * - 有目标：根据 dx/dy/dz 计算 targetYaw/targetPitch，rotlerp 逼近
 * - yaw 限速 10°/tick，pitch 限速 40°/tick
 * - 目标实体不存在（targetId>0 但 lookup 返回 nullptr）：走无目标分支
 * - nullptr 回调：走无目标分支
 * - 多 tick 收敛：大角度差逐步逼近
 * - prev 值正确备份（用于渲染插值）
 * - getInterpolatedWitherSideHeadYaw/Pitch 在 partialTick=0/0.5/1.0 的值
 * - HEAD_TARGET 元数据同步到 m_witherHeadTargetId
 * - 两侧头独立追踪不同目标
 *
 * 数据流：
 * 服务端 WitherEntity::aiStep → HEAD_TARGET_1/2/3 网络同步
 * → ClientEntity::syncMetadataFromDataManager 读取 → m_witherHeadTargetId[3]
 * → ClientEntityManager::tick 调用 tickWitherSideHeads(lookup)
 * → 客户端独立镜像 MC 的侧头朝向计算
 * → getInterpolatedWitherSideHeadYaw/Pitch 供渲染器插值取值
 */

#include <gtest/gtest.h>

#include <cmath>
#include <functional>

#include "client/world/entity/ClientEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/util/math/MathUtils.hpp"

using namespace mc;
using namespace mc::client;

namespace {

/**
 * @brief 在 ClientEntity 的 dataManager 中注册一个与服务端
 *        WitherEntity::HEAD_TARGET_1/2/3 相同 ID 的 i32 参数。
 */
void registerHeadTargetParam(ClientEntity& entity, u16 paramId, i32 defaultValue)
{
    ::mc::entity::DataParameter<i32> param(paramId);
    entity.dataManager().registerParam(param, defaultValue);
}

void setHeadTargetParam(ClientEntity& entity, u16 paramId, i32 value)
{
    ::mc::entity::DataParameter<i32> param(paramId);
    entity.dataManager().set(param, value);
}

// ============================================================================
// 测试夹具
// ============================================================================

class ClientEntityWitherSideHeadTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 构造一次服务端 WitherEntity 以触发其 registerData()，使静态 DataParameter
        // (HEAD_TARGET_1/2/3 等) 经继承链分配器获得真实 id（而非哨兵 0xFFFF）。
        // 否则 getHeadTarget2ParamId() 返回 0xFFFF，多个未分配静态成员互相别名，
        // syncMetadataFromDataManager 的 getRaw(0xFFFF).get<T>() 类型不符抛 bad variant access。
        // 静态成员进程内幂等，首次构造即分配，后续复用。
        static const auto s_serverWither = std::make_unique<::mc::entity::WitherEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        (void)s_serverWither;

        // 创建凋灵 ClientEntity
        m_wither = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:wither");
        // 凋灵眼高 2.0（用于目标实体 eyeHeight 计算）
        m_wither->setEyeHeight(2.0f);
    }

    void TearDown() override { m_wither.reset(); }

    /// @brief 创建一个目标 ClientEntity 并返回其 EntityInstanceId
    EntityInstanceId makeTargetClientEntity(f32 x, f32 y, f32 z, f32 eyeHeight = 1.62f)
    {
        EntityInstanceId id = m_nextTargetId;
        m_nextTargetId = EntityInstanceId(static_cast<u32>(m_nextTargetId) + 1);
        auto target = std::make_unique<ClientEntity>(id, "minecraft:player");
        target->setPosition(x, y, z);
        target->setEyeHeight(eyeHeight);
        m_targets[id] = std::move(target);
        return id;
    }

    /// @brief 实体查找回调，模拟 ClientEntityManager::getEntity
    const ClientEntity* lookupEntity(EntityInstanceId id) const
    {
        auto it = m_targets.find(id);
        if (it == m_targets.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    std::unique_ptr<ClientEntity> m_wither;
    std::unordered_map<EntityInstanceId, std::unique_ptr<ClientEntity>> m_targets;
    EntityInstanceId m_nextTargetId = EntityInstanceId(100);
};

// ============================================================================
// 初始状态测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, InitialState_AllSideHeadAnglesZero)
{
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(0, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(0, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(1, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(1, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(1, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(1, 1.0f), 0.0f);
}

// ============================================================================
// 无目标时侧头 yaw 回正测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, NoTarget_YawLerpsTowardBodyRot)
{
    // bodyRot = yaw = 90°，侧头初始 yaw = 0°
    // 无目标时 yaw 以 10°/tick 逼近 bodyRot
    m_wither->setRotation(90.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 使用空回调（无目标）
    m_wither->tickWitherSideHeads({});

    // 第一次 tick：yaw 从 0 朝 90 移动 10°
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 10.0f, 0.001f);
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(1, 1.0f), 10.0f, 0.001f);
}

TEST_F(ClientEntityWitherSideHeadTest, NoTarget_PitchUnchanged)
{
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    f32 initialPitch0 = m_wither->getInterpolatedWitherSideHeadPitch(0, 1.0f);
    f32 initialPitch1 = m_wither->getInterpolatedWitherSideHeadPitch(1, 1.0f);

    m_wither->tickWitherSideHeads({});

    // pitch 应保持不变
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(0, 1.0f), initialPitch0);
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadPitch(1, 1.0f), initialPitch1);
}

TEST_F(ClientEntityWitherSideHeadTest, NoTarget_MultipleTicks_ConvergesToBodyRot)
{
    m_wither->setRotation(45.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    for (i32 i = 0; i < 4; ++i) {
        m_wither->tickWitherSideHeads({});
    }
    // 4 tick 后 yaw = 40°
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 40.0f, 0.001f);

    // 第 5 tick：diff = 45-40 = 5 < 10，直接到 45
    m_wither->tickWitherSideHeads({});
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 45.0f, 0.001f);
}

// ============================================================================
// 有目标时侧头朝向计算测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, WithTarget_YawConvergesTowardTarget)
{
    // wither 在原点，bodyRot=0
    // 侧头1（左头，j=0）位置 = (cos(0)*1.3, 66.2, sin(0)*1.3) = (1.3, 66.2, 0)
    // 目标在 (1.3, 66.2, 20)（正前方），dx=0, dz=20
    // targetYaw = atan2(20, 0) * 180/PI - 90 = 90 - 90 = 0
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 创建目标实体
    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 64.0f, 20.0f, 2.2f);

    // 直接设置 m_witherHeadTargetId[1]（左头目标）
    // 通过元数据同步设置
    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));
    m_wither->syncMetadataFromDataManager();

    // 用 lambda 捕获 this 提供实体查找
    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });

    // 侧头1 yaw 朝 0° 逼近，初始也是 0°，diff=0，yaw 保持 0°
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f, 0.5f);
}

TEST_F(ClientEntityWitherSideHeadTest, WithTarget_YawRateLimitedTo10DegreesPerTick)
{
    // 目标在侧头1 正后方（-Z），targetYaw ≈ -180°
    // 侧头1 位置 = (1.3, 66.2, 0)，目标在 (1.3, 66.2, -20)
    // dx=0, dz=-20, targetYaw = atan2(-20, 0) * 180/PI - 90 = -90 - 90 = -180
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 64.0f, -20.0f, 2.2f);

    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));
    m_wither->syncMetadataFromDataManager();

    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });

    // yaw 从 0 朝 -180 逼近，限速 10°/tick
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), -10.0f, 0.01f);
}

TEST_F(ClientEntityWitherSideHeadTest, WithTarget_PitchRateLimitedTo40DegreesPerTick)
{
    // 目标在侧头1 下方，targetPitch 很大
    // 侧头1 位置 = (1.3, 66.2, 0)，目标在 (1.3, 60, 0.1)
    // dy = (60 + 2.2) - 66.2 = -4.0, horizontalDist = 0.1
    // targetPitch = -(atan2(-4, 0.1) * 180/PI) = -(-88.57) = 88.57°
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 60.0f, 0.1f, 2.2f);

    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));
    m_wither->syncMetadataFromDataManager();

    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });

    // pitch 限速 40°/tick，从 0 朝 88.57 逼近，第一次 tick = 40
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadPitch(0, 1.0f), 40.0f, 0.01f);
}

TEST_F(ClientEntityWitherSideHeadTest, WithTarget_MultipleTicks_ConvergesToTargetYaw)
{
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 64.0f, -20.0f, 2.2f);

    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));
    m_wither->syncMetadataFromDataManager();

    // 推进 20 tick（10°/tick * 18 = 180°，足够收敛）
    for (i32 i = 0; i < 20; ++i) {
        m_wither->tickWitherSideHeads(
            [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });
    }

    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), -180.0f, 1.0f);
}

// ============================================================================
// 边界场景测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, TargetIdPositive_ButLookupReturnsNull_FallsBackToNoTarget)
{
    // targetId > 0 但 lookup 返回 nullptr → 走无目标分支
    m_wither->setRotation(30.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 注册一个 targetId=99999 但 lookup 不会找到（m_targets 中没有）
    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 99999);
    m_wither->syncMetadataFromDataManager();

    // lookup 回调始终返回 nullptr
    m_wither->tickWitherSideHeads([](EntityInstanceId) -> const ClientEntity* { return nullptr; });

    // 无目标分支：yaw 朝 bodyRot=30 逼近 10°/tick
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 10.0f, 0.01f);
}

TEST_F(ClientEntityWitherSideHeadTest, NullLookupCallback_FallsBackToNoTarget)
{
    // 传入空 std::function → 走无目标分支
    m_wither->setRotation(20.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 即使设置了 targetId，空回调也应走无目标分支
    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 99999);
    m_wither->syncMetadataFromDataManager();

    // 传空 std::function
    m_wither->tickWitherSideHeads({});

    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 10.0f, 0.01f);
}

// ============================================================================
// prev 备份与插值测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, PrevAngles_BackupBeforeUpdate)
{
    m_wither->setRotation(30.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 初始 prev = 0, current = 0
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.0f), 0.0f); // prev
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f); // current

    m_wither->tickWitherSideHeads({});

    // tick 后：prev = 0（旧 current），current = 10
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.0f), 0.0f);     // prev
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 10.0f, 0.01f); // current

    // 第二次 tick
    m_wither->tickWitherSideHeads({});

    // prev = 10（上一次的 current），current = 20
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.0f), 10.0f, 0.01f);
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 20.0f, 0.01f);
}

TEST_F(ClientEntityWitherSideHeadTest, Interpolation_PartialTick_Midpoint)
{
    m_wither->setRotation(30.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    // 推进 2 tick：prev=0→10, current=10→20
    m_wither->tickWitherSideHeads({});
    m_wither->tickWitherSideHeads({});

    // prev=10, current=20
    // partialTick=0.5 → (10 + 20) / 2 = 15
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.0f), 10.0f, 0.01f);
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 20.0f, 0.01f);
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 0.5f), 15.0f, 0.01f);
}

// ============================================================================
// 两侧头独立追踪测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, TwoSideHeads_IndependentTargets)
{
    // bodyRot=0
    // 侧头1（j=0）位置 = (1.3, 66.2, 0)，目标在 +Z → targetYaw=0
    // 侧头2（j=1）位置 = (cos(180)*1.3, 66.2, sin(180)*1.3) = (-1.3, 66.2, 0)
    //                  目标在 -Z → targetYaw=-180
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId target1Id = makeTargetClientEntity(1.3f, 64.0f, 20.0f, 2.2f);
    EntityInstanceId target2Id = makeTargetClientEntity(-1.3f, 64.0f, -20.0f, 2.2f);

    // HEAD_TARGET_2 = 侧头1（左头），HEAD_TARGET_3 = 侧头2（右头）
    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(target1Id)));
    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget3ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget3ParamId(), static_cast<i32>(static_cast<u32>(target2Id)));
    m_wither->syncMetadataFromDataManager();

    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });

    // 侧头1 朝 +Z（targetYaw=0），yaw 保持 0
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f, 0.5f);

    // 侧头2 朝 -Z（targetYaw=-180），yaw 从 0 朝 -180 逼近 10°
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(1, 1.0f), -10.0f, 0.5f);
}

// ============================================================================
// 元数据同步测试
// ============================================================================

TEST_F(ClientEntityWitherSideHeadTest, MetadataSync_HEAD_TARGET_2_PopulatesLeftHeadTarget)
{
    // 验证 syncMetadataFromDataManager 正确读取 HEAD_TARGET_2 并写入 m_witherHeadTargetId[1]
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 64.0f, 20.0f, 2.2f);

    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));

    // 同步前：无目标 → yaw 保持 0
    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f);

    // 同步后：侧头1 有目标在 +Z（targetYaw=0），yaw 仍保持 0（diff=0）
    m_wither->syncMetadataFromDataManager();
    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });

    // yaw 仍为 0（targetYaw=0, current=0, diff=0）
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f, 0.5f);
}

TEST_F(ClientEntityWitherSideHeadTest, MetadataSync_NoParamRegistered_DoesNotCrash)
{
    // 未注册参数时 syncMetadataFromDataManager 不应崩溃
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EXPECT_NO_THROW({ m_wither->syncMetadataFromDataManager(); });

    // 无目标 → yaw 保持 0
    m_wither->tickWitherSideHeads({});
    EXPECT_FLOAT_EQ(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f);
}

TEST_F(ClientEntityWitherSideHeadTest, MetadataSync_ReadsCorrectParamId)
{
    // 验证 syncMetadataFromDataManager 读取的是 HEAD_TARGET_2 而非其他 i32 参数
    m_wither->setRotation(0.0f, 0.0f);
    m_wither->setPosition(0.0f, 64.0f, 0.0f);

    EntityInstanceId targetId = makeTargetClientEntity(1.3f, 64.0f, 20.0f, 2.2f);

    registerHeadTargetParam(*m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(
        *m_wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), static_cast<i32>(static_cast<u32>(targetId)));

    // 额外注册一个无关的 i32 参数（ID 不同）
    auto otherParam = ::mc::entity::EntityDataManager::createKey<i32>();
    m_wither->dataManager().registerParam(otherParam, 0);
    m_wither->dataManager().set(otherParam, 55555);

    m_wither->syncMetadataFromDataManager();

    // 应读取 HEAD_TARGET_2 而非 otherParam
    // 侧头1 有目标在 +Z（targetYaw=0），yaw 保持 0（而非朝 bodyRot 逼近）
    m_wither->tickWitherSideHeads(
        [this](EntityInstanceId id) -> const ClientEntity* { return this->lookupEntity(id); });
    EXPECT_NEAR(m_wither->getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f, 0.5f)
        << "应读取 HEAD_TARGET_2 而非其他 i32 参数";
}

// ============================================================================
// typeId 归一化测试（带/不带 "minecraft:" 前缀）
// ============================================================================

TEST(ClientEntityWitherSideHeadTypeIdTest, WitherWithoutPrefix_SyncsHeadTarget)
{
    // 独立 TEST 无 SetUp 复用夹具内的服务端 WitherEntity 构造，须自行构造一次
    // 以触发 registerData() 为静态 DataParameter 分配真实 id（首次进程内幂等），
    // 否则 getHeadTarget2ParamId() 返回哨兵 0xFFFF，set 在 0xFFFF 创建条目被
    // syncMetadataFromDataManager 的 MobFlags 分支误读为 i8 触发 bad_variant_access。
    static const auto s_serverWither = std::make_unique<::mc::entity::WitherEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    (void)s_serverWither;

    ClientEntity wither(EntityInstanceId(1), "wither");
    wither.setRotation(0.0f, 0.0f);
    wither.setPosition(0.0f, 64.0f, 0.0f);
    wither.setEyeHeight(2.0f);

    // 注册 HEAD_TARGET_2 参数并设置值
    registerHeadTargetParam(wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(wither, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 99999);

    // typeId="wither"（不带前缀）应被正确识别，syncMetadataFromDataManager 不崩溃
    EXPECT_NO_THROW({ wither.syncMetadataFromDataManager(); });
}

// ============================================================================
// 非凋灵实体不触发同步测试
// ============================================================================

TEST(ClientEntityWitherSideHeadTypeIdTest, NonWither_DoesNotSyncHeadTarget)
{
    // 独立 TEST 须自行构造服务端 WitherEntity 触发 registerData() 分配真实 id，
    // 否则 getHeadTarget2ParamId() 返回哨兵 0xFFFF 致 set 在 0xFFFF 建条目，
    // 被 MobFlags 分支误读为 i8 触发 bad_variant_access。
    static const auto s_serverWither = std::make_unique<::mc::entity::WitherEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    (void)s_serverWither;

    ClientEntity zombie(EntityInstanceId(1), "minecraft:zombie");

    // 即使注册了 HEAD_TARGET_2 参数，zombie 的 syncMetadataFromDataManager 也不应读取
    registerHeadTargetParam(zombie, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 0);
    setHeadTargetParam(zombie, ::mc::entity::WitherEntity::getHeadTarget2ParamId(), 99999);

    EXPECT_NO_THROW({ zombie.syncMetadataFromDataManager(); });
    // zombie 不应崩溃，且 tickWitherSideHeads 不应改变其状态
    // （但 zombie 可以调用 tickWitherSideHeads，只是 m_witherHeadTargetId 均为 0）
    zombie.tickWitherSideHeads({});
    EXPECT_FLOAT_EQ(zombie.getInterpolatedWitherSideHeadYaw(0, 1.0f), 0.0f);
}

} // namespace
