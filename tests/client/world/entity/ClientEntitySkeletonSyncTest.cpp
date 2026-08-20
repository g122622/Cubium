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
 * @file ClientEntitySkeletonSyncTest.cpp
 * @brief ClientEntity 骷髅激怒状态元数据同步单元测试
 *
 * 验证 ClientEntity::syncMetadataFromDataManager 对 skeleton/stray/bogged/
 * wither_skeleton 四个骷髅 typeId 正确读取 MobEntity::DATA_MOB_FLAGS_PARAM
 * 位 2（MOB_FLAG_AGGRESSIVE）并写入 m_isAggressive 镜像字段。
 *
 * 拉弓渲染状态不再有独立同步字段——对齐 vanilla 1.21.11
 * AbstractSkeletonRenderer.getArmPose：客户端据 isAggressive + 主手持弓判定
 * 渲染 BowAndArrow。原 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM(id16)
 * 同步已移除（该 id16 致 vanilla Stray/WitherSkeleton 客户端 set_entity_data
 * 越界崩溃 "Index 16 out of bounds for length 16"）。
 *
 * 数据流：
 * 服务端 RangedBowAttackGoal::startExecuting → setAggroed(true)
 *   → MobEntity::setAggressive → DATA_MOB_FLAGS_PARAM 置位 2
 *   → EntityTracker 广播 ir::play::SetEntityData
 *   → 客户端 ClientEntity::setMetadata → EntityMetadataSerializer::deserialize
 *   → 写入 ClientEntity::m_dataManager
 *   → syncMetadataFromDataManager（不按 typeId 过滤，仅 hasParam 判断）
 *   → 读取 DATA_MOB_FLAGS_PARAM 位 2 → setIsAggressive(aggressive)
 *   → EntityRendererManager::_applySkeletonArmPose 据 isAggressive + 持弓
 *     设置 SkeletonModel 右臂 BowAndArrow 姿态
 *
 * 注：DATA_MOB_FLAGS_PARAM 由 MobEntity::registerData 注册，所有 MobEntity
 * 子类（含全部骷髅变体）都拥有此参数，syncMetadataFromDataManager 不按 typeId
 * 分发，仅以 hasParam 判断——故 wither_skeleton（走近战， MeleeAttackGoal 也用
 * aggressive）同样同步。
 */

#include <gtest/gtest.h>

#include "client/world/entity/ClientEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/item/Items.hpp"

using namespace mc;
using namespace mc::client;

namespace {

/**
 * @brief 构造一次服务端 MobEntity 触发其 registerData()，使静态
 *        DataParameter(DATA_MOB_FLAGS_PARAM) 经继承链分配器获得真实 id
 *        （而非哨兵 0xFFFF）。
 *
 * 否则 getMobFlagsParamId() 返回 0xFFFF，测试 helper 的 set 在 0xFFFF 创建条目，
 * 断言语义错乱。静态成员进程内幂等，首次构造即分配真实 id(15)，后续复用。
 * MobEntity 构造仅 registerData()，可独立实例化。
 */
void ensureMobFlagsParamAllocated()
{
    static const auto s_serverMob = std::make_unique<::mc::MobEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    (void)s_serverMob;
}

/**
 * @brief 在 ClientEntity 的 dataManager 中注册一个与服务端
 *        MobEntity::DATA_MOB_FLAGS_PARAM 相同 ID 的 i8 参数。
 *
 * 客户端 ClientEntity 拥有独立的 EntityDataManager 实例。syncMetadataFromDataManager
 * 通过 MobEntity::getMobFlagsParamId() 查找参数，因此客户端必须注册相同 ID 的参数
 * 才能被读取。此处直接构造相同 ID 的 DataParameter 模拟反序列化后的状态。
 */
void registerMobFlagsParam(ClientEntity& entity, i8 defaultValue)
{
    const u16 paramId = ::mc::MobEntity::getMobFlagsParamId();
    ::mc::entity::DataParameter<i8> param(paramId);
    entity.dataManager().registerParam(param, defaultValue);
}

void setMobFlagsParam(ClientEntity& entity, i8 value)
{
    const u16 paramId = ::mc::MobEntity::getMobFlagsParamId();
    const ::mc::entity::DataParameter<i8> param(paramId);
    entity.dataManager().set(param, value);
}

// ============================================================================
// 普通骷髅（skeleton）同步测试
// ============================================================================

class ClientEntitySkeletonSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureMobFlagsParamAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:skeleton");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntitySkeletonSyncTest, Aggressive_DefaultFalse)
{
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntitySkeletonSyncTest, Aggressive_SetAndGet)
{
    entity->setIsAggressive(true);
    EXPECT_TRUE(entity->isAggressive());

    entity->setIsAggressive(false);
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    // 模拟服务端广播 aggressive=true（位 2 置位，RangedBowAttackGoal::startExecuting）
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask())); // 0x04

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "skeleton typeId + 位 2 置位 应同步到 isAggressive 镜像字段";
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_FlagCleared_KeepsAggressiveFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0)); // 位 2 未置位

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_ToggleTrueThenFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));

    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive());

    setMobFlagsParam(*entity, static_cast<i8>(0));
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_NoParamRegistered_DoesNotCrash)
{
    // 未注册参数时 syncMetadataFromDataManager 不应崩溃，hasParam 返回 false 自然跳过
    EXPECT_FALSE(entity->isAggressive());
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_ReadsCorrectParamId)
{
    // 验证 syncMetadataFromDataManager 读取的是 DATA_MOB_FLAGS_PARAM 的 ID，
    // 而非其他 i8 参数。注册一个不同 ID 的 i8 参数，确保不被误读。
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    // 额外注册一个无关的 i8 参数（ID 不同），设置为 0
    auto otherParam = ::mc::entity::EntityDataManager::createKey<i8>();
    entity->dataManager().registerParam(otherParam, static_cast<i8>(0));
    entity->dataManager().set(otherParam, static_cast<i8>(0));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "应读取 DATA_MOB_FLAGS_PARAM 而非其他 i8 参数";
}

// ============================================================================
// 流浪者（stray）同步测试
// ============================================================================

class ClientEntityStraySyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureMobFlagsParamAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:stray");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityStraySyncTest, Aggressive_DefaultFalse)
{
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityStraySyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "stray typeId + 位 2 置位 应同步到 isAggressive 镜像字段";
}

TEST_F(ClientEntityStraySyncTest, SyncFromDataManager_FlagCleared_KeepsAggressiveFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0));

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

// ============================================================================
// 沼骸骨（bogged）同步测试
// ============================================================================

class ClientEntityBoggedSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureMobFlagsParamAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:bogged");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityBoggedSyncTest, Aggressive_DefaultFalse)
{
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityBoggedSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "bogged typeId + 位 2 置位 应同步到 isAggressive 镜像字段";
}

TEST_F(ClientEntityBoggedSyncTest, SyncFromDataManager_FlagCleared_KeepsAggressiveFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0));

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

// ============================================================================
// 凋灵骷髅（wither_skeleton）同步测试
//
// 凋灵骷髅走 MeleeAttackGoal（不持弓拉弓），但 MeleeAttackGoal::startExecuting
// 同样置 aggressive（驱动近战抬臂动画）。DATA_MOB_FLAGS_PARAM 由 MobEntity 注册，
// syncMetadataFromDataManager 不按 typeId 过滤，故 wither_skeleton 同样同步 aggressive。
// （此与旧 chargingBow 机制不同——旧机制 wither_skeleton 不在 skeleton/stray/bogged
// 分支故不同步 chargingBow；新机制统一走 MobFlags，wither_skeleton 也同步。）
// ============================================================================

class ClientEntityWitherSkeletonSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureMobFlagsParamAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:wither_skeleton");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityWitherSkeletonSyncTest, Aggressive_DefaultFalse)
{
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityWitherSkeletonSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    // 凋灵骷髅虽不持弓，但 MeleeAttackGoal 也用 aggressive，经 MobFlags 同步
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive())
        << "wither_skeleton typeId + 位 2 置位 应同步到 isAggressive（统一走 MobFlags）";
}

TEST_F(ClientEntityWitherSkeletonSyncTest, SyncFromDataManager_FlagCleared_KeepsAggressiveFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0));

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

// ============================================================================
// typeId 归一化测试（带/不带 "minecraft:" 前缀）
// ============================================================================

class ClientEntitySkeletonTypeIdNormalizeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { ensureMobFlagsParamAllocated(); }
};

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Skeleton_WithoutPrefix_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "skeleton");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "typeId='skeleton'（不带前缀）应被正确识别";
}

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Stray_WithoutPrefix_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "stray");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "typeId='stray'（不带前缀）应被正确识别";
}

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Bogged_WithoutPrefix_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "bogged");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "typeId='bogged'（不带前缀）应被正确识别";
}

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, WitherSkeleton_WithoutPrefix_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "wither_skeleton");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "typeId='wither_skeleton'（不带前缀）应被正确识别";
}

// ============================================================================
// 位隔离测试：aggressive 仅受位 2 控制，不受其他位影响
// ============================================================================

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_OnlyBit2ControlsAggressive)
{
    // 位 0+1（noAi + leftHanded）置位，位 2 清零 → aggressive=false
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0x03)); // 位 0+1，非位 2

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive()) << "仅位 0+1 置位（位 2 清零）应 aggressive=false";

    // 全位置位（含位 2）→ aggressive=true
    setMobFlagsParam(*entity, static_cast<i8>(0x07)); // 位 0+1+2
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "位 2 置位（含其他位）应 aggressive=true";
}

} // namespace
