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
 * @brief ClientEntity 骷髅拉弓状态元数据同步单元测试
 *
 * 验证 ClientEntity::syncMetadataFromDataManager 对 skeleton/stray/bogged
 * typeId 正确读取 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 并写入
 * m_chargingBow 镜像字段，且对 wither_skeleton 不触发（凋灵骷髅不持弓，
 * 但其仍注册参数——本测试验证 typeId 分支的正确性）。
 *
 * 数据流：
 * 服务端 AbstractSkeletonEntity::setChargingBow → DataParameter::set
 * → EntityTracker 广播 ir::play::SetEntityData
 * → 客户端 ClientEntity::setMetadata → EntityMetadataSerializer::deserialize
 * → 写入 ClientEntity::m_dataManager
 * → syncMetadataFromDataManager 按 typeId 分发
 * → 读取 DATA_CHARGING_BOW_PARAM → setChargingBow(charging)
 * → EntityRendererManager::_applySkeletonArmPose 读取 isChargingBow()
 *   设置 SkeletonModel 右臂 BowAndArrow 姿态
 */

#include <gtest/gtest.h>

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/item/Items.hpp"

using namespace mc;
using namespace mc::client;

namespace {

/**
 * @brief 构造一次服务端 SkeletonEntity 触发其 registerData()，使静态
 *        DataParameter(DATA_CHARGING_BOW_PARAM) 经继承链分配器获得真实 id
 *        （而非哨兵 0xFFFF）。
 *
 * 否则 getChargingBowParamId() 返回 0xFFFF，测试 helper 的 set 在 0xFFFF 创建条目，
 * 被 syncMetadataFromDataManager 的 MobFlags 分支(getRaw(0xFFFF).get<i8>())误读
 * 触发 bad_variant_access。静态成员进程内幂等，首次构造即分配，后续复用。
 *
 * 用具体子类 SkeletonEntity 而非 AbstractSkeletonEntity：后者构造函数为 protected
 * （设计为抽象基类），无法在测试中直接 std::make_unique。SkeletonEntity::create(IWorld*)
 * 忽略 world 参数直接构造实体，证明其构造不依赖 IWorld；registerGoals/Attributes/
 * setCombatTask 在无 world 下可安全执行。
 */
void ensureSkeletonParamsAllocated()
{
    static const auto s_serverSkeleton = std::make_unique<::mc::SkeletonEntity>(EntityInstanceId(1));
    (void)s_serverSkeleton;
}

/**
 * @brief 在 ClientEntity 的 dataManager 中注册一个与服务端
 *        AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 相同 ID 的 bool 参数。
 *
 * 服务端 DATA_CHARGING_BOW_PARAM 通过 EntityDataManager::createKey<bool>() 分配 ID，
 * 客户端 ClientEntity 拥有独立的 EntityDataManager 实例。syncMetadataFromDataManager
 * 通过 AbstractSkeletonEntity::getChargingBowParamId() 查找参数，因此客户端必须
 * 注册相同 ID 的参数才能被读取。
 *
 * EntityMetadataSerializer::deserialize 会自动注册参数（如果服务端发送了），
 * 此处直接构造相同 ID 的 DataParameter 模拟反序列化后的状态。
 */
void registerChargingBowParam(ClientEntity& entity, bool defaultValue)
{
    const u16 paramId = ::mc::AbstractSkeletonEntity::getChargingBowParamId();
    ::mc::entity::DataParameter<bool> param(paramId);
    entity.dataManager().registerParam(param, defaultValue);
}

void setChargingBowParam(ClientEntity& entity, bool value)
{
    const u16 paramId = ::mc::AbstractSkeletonEntity::getChargingBowParamId();
    ::mc::entity::DataParameter<bool> param(paramId);
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
        ensureSkeletonParamsAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:skeleton");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntitySkeletonSyncTest, ChargingBow_DefaultFalse)
{
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntitySkeletonSyncTest, ChargingBow_SetAndGet)
{
    entity->setChargingBow(true);
    EXPECT_TRUE(entity->isChargingBow());

    entity->setChargingBow(false);
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_True_SetsChargingBow)
{
    // 模拟服务端广播 chargingBow=true
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, true);

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isChargingBow()) << "skeleton typeId + chargingBow=true 应同步到镜像字段";
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_False_KeepsChargingBowFalse)
{
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, false);

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_ToggleTrueThenFalse)
{
    registerChargingBowParam(*entity, false);

    setChargingBowParam(*entity, true);
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isChargingBow());

    setChargingBowParam(*entity, false);
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_NoParamRegistered_DoesNotCrash)
{
    // 未注册参数时 syncMetadataFromDataManager 不应崩溃
    // hasParam 返回 false，分支自然跳过
    EXPECT_FALSE(entity->isChargingBow());
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntitySkeletonSyncTest, SyncFromDataManager_ReadsCorrectParamId)
{
    // 验证 syncMetadataFromDataManager 读取的是 DATA_CHARGING_BOW_PARAM 的 ID，
    // 而非其他 bool 参数。注册一个不同 ID 的 bool 参数，确保不被误读。
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, true);

    // 额外注册一个无关的 bool 参数（ID 不同），设置为 false
    auto otherParam = ::mc::entity::EntityDataManager::createKey<bool>();
    entity->dataManager().registerParam(otherParam, false);
    entity->dataManager().set(otherParam, false);

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isChargingBow()) << "应读取 DATA_CHARGING_BOW_PARAM 而非其他 bool 参数";
}

// ============================================================================
// 流浪者（stray）同步测试
// ============================================================================

class ClientEntityStraySyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureSkeletonParamsAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:stray");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityStraySyncTest, ChargingBow_DefaultFalse)
{
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntityStraySyncTest, SyncFromDataManager_True_SetsChargingBow)
{
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, true);

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isChargingBow()) << "stray typeId + chargingBow=true 应同步到镜像字段";
}

TEST_F(ClientEntityStraySyncTest, SyncFromDataManager_False_KeepsChargingBowFalse)
{
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, false);

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow());
}

// ============================================================================
// 沼骸骨（bogged）同步测试
// ============================================================================

class ClientEntityBoggedSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureSkeletonParamsAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:bogged");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityBoggedSyncTest, ChargingBow_DefaultFalse)
{
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntityBoggedSyncTest, SyncFromDataManager_True_SetsChargingBow)
{
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, true);

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isChargingBow()) << "bogged typeId + chargingBow=true 应同步到镜像字段";
}

TEST_F(ClientEntityBoggedSyncTest, SyncFromDataManager_False_KeepsChargingBowFalse)
{
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, false);

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow());
}

// ============================================================================
// typeId 归一化测试（带/不带 "minecraft:" 前缀）
// ============================================================================

class ClientEntitySkeletonTypeIdNormalizeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { ensureSkeletonParamsAllocated(); }
};

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Skeleton_WithoutPrefix_SyncsChargingBow)
{
    ClientEntity entity(EntityInstanceId(1), "skeleton");
    EXPECT_FALSE(entity.isChargingBow());

    registerChargingBowParam(entity, false);
    setChargingBowParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isChargingBow()) << "typeId='skeleton'（不带前缀）应被正确识别";
}

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Stray_WithoutPrefix_SyncsChargingBow)
{
    ClientEntity entity(EntityInstanceId(1), "stray");
    EXPECT_FALSE(entity.isChargingBow());

    registerChargingBowParam(entity, false);
    setChargingBowParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isChargingBow()) << "typeId='stray'（不带前缀）应被正确识别";
}

TEST_F(ClientEntitySkeletonTypeIdNormalizeTest, Bogged_WithoutPrefix_SyncsChargingBow)
{
    ClientEntity entity(EntityInstanceId(1), "bogged");
    EXPECT_FALSE(entity.isChargingBow());

    registerChargingBowParam(entity, false);
    setChargingBowParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isChargingBow()) << "typeId='bogged'（不带前缀）应被正确识别";
}

// ============================================================================
// 凋灵骷髅（wither_skeleton）不触发同步测试
//
// 凋灵骷髅不持弓，走 MeleeAttackGoal。syncMetadataFromDataManager 的
// skeleton/stray/bogged 分支不包含 wither_skeleton，因此即使
// DATA_CHARGING_BOW_PARAM 被注册（继承自 AbstractSkeletonEntity），
// syncMetadataFromDataManager 也不会读取它。
// 注意：凋灵骷髅的 isChargingBow() 镜像字段永远保持 false。
// ============================================================================

class ClientEntityWitherSkeletonSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override
    {
        ensureSkeletonParamsAllocated();
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:wither_skeleton");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityWitherSkeletonSyncTest, ChargingBow_DefaultFalse)
{
    EXPECT_FALSE(entity->isChargingBow());
}

TEST_F(ClientEntityWitherSkeletonSyncTest, SyncFromDataManager_DoesNotReadChargingBowParam)
{
    // 凋灵骷髅 typeId 不在 skeleton/stray/bogged 分支中，
    // 即使注册了 DATA_CHARGING_BOW_PARAM 并设置为 true，
    // syncMetadataFromDataManager 也不会写入 isChargingBow。
    registerChargingBowParam(*entity, false);
    setChargingBowParam(*entity, true);

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isChargingBow()) << "wither_skeleton typeId 不应触发 chargingBow 同步（凋灵骷髅不持弓）";
}

// ============================================================================
// 非骷髅实体不触发同步测试
// ============================================================================

TEST(ClientEntityNonSkeletonSyncTest, Zombie_DoesNotSyncChargingBow)
{
    // 独立 TEST 须自行触发 registerData() 分配真实 id，否则 getChargingBowParamId()
    // 返回哨兵 0xFFFF 致 set 在 0xFFFF 建条目，被 MobFlags 分支误读为 i8 触发 bad_variant_access。
    ensureSkeletonParamsAllocated();

    ClientEntity entity(EntityInstanceId(1), "minecraft:zombie");
    EXPECT_FALSE(entity.isChargingBow());

    registerChargingBowParam(entity, false);
    setChargingBowParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_FALSE(entity.isChargingBow()) << "zombie typeId 不应触发 chargingBow 同步";
}

TEST(ClientEntityNonSkeletonSyncTest, Player_DoesNotSyncChargingBow)
{
    ensureSkeletonParamsAllocated();

    ClientEntity entity(EntityInstanceId(1), "minecraft:player");
    EXPECT_FALSE(entity.isChargingBow());

    registerChargingBowParam(entity, false);
    setChargingBowParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_FALSE(entity.isChargingBow()) << "player typeId 不应触发 chargingBow 同步";
}

} // namespace
