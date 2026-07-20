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
 * @file ClientEntityAggressiveSyncTest.cpp
 * @brief ClientEntity Mob 激怒状态元数据同步单元测试
 *
 * 验证 ClientEntity::syncMetadataFromDataManager 正确读取 MobEntity::DATA_MOB_FLAGS_PARAM
 * 的位 2 (MOB_FLAG_AGGRESSIVE) 并写入 m_isAggressive 镜像字段。
 *
 * 与骷髅拉弓同步（ClientEntitySkeletonSyncTest）不同，Mob 激怒状态同步
 * 不按 typeId 分发，而是通过 m_dataManager.hasParam(MobEntity::getMobFlagsParamId())
 * 检测参数是否存在（所有 MobEntity 子类都注册了 DATA_MOB_FLAGS_PARAM）。
 *
 * 数据流：
 * 服务端 MobEntity::setAggressive → DATA_MOB_FLAGS_PARAM 位 2 置位
 * → EntityTracker 广播 EntityMetadataPacket
 * → 客户端 ClientEntity::setMetadata → EntityMetadataSerializer::deserialize
 * → 写入 ClientEntity::m_dataManager
 * → syncMetadataFromDataManager 读取 DATA_MOB_FLAGS_PARAM & 0x04
 * → setIsAggressive(aggressive)
 * → EntityRendererManager::_applyZombieState 读取 isAggressive()
 *   推送到 ZombieModel::setAggressive，驱动 animateZombieArms 攻击抬臂动画
 */

#include <gtest/gtest.h>

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/item/Items.hpp"

using namespace mc;
using namespace mc::client;

namespace {

/**
 * @brief 在 ClientEntity 的 dataManager 中注册一个与服务端
 *        MobEntity::DATA_MOB_FLAGS_PARAM 相同 ID 的 i8 参数。
 *
 * 服务端 DATA_MOB_FLAGS_PARAM 通过 EntityDataManager::createKey<i8>() 分配 ID，
 * 客户端 ClientEntity 拥有独立的 EntityDataManager 实例。syncMetadataFromDataManager
 * 通过 MobEntity::getMobFlagsParamId() 查找参数，因此客户端必须注册相同 ID 的参数才能被读取。
 *
 * EntityMetadataSerializer::deserialize 会自动注册参数（如果服务端发送了），
 * 此处直接构造相同 ID 的 DataParameter 模拟反序列化后的状态。
 */
void registerMobFlagsParam(ClientEntity& entity, i8 defaultValue)
{
    const u16 paramId = ::mc::MobEntity::getMobFlagsParamId();
    const ::mc::entity::DataParameter<i8> param(paramId);
    entity.dataManager().registerParam(param, defaultValue);
}

void setMobFlagsParam(ClientEntity& entity, i8 value)
{
    const u16 paramId = ::mc::MobEntity::getMobFlagsParamId();
    const ::mc::entity::DataParameter<i8> param(paramId);
    entity.dataManager().set(param, value);
}

} // namespace

// ============================================================================
// 僵尸（zombie）同步测试
// ============================================================================

class ClientEntityZombieAggressiveSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:zombie"); }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityZombieAggressiveSyncTest, Aggressive_DefaultFalse)
{
    EXPECT_FALSE(entity->isAggressive()) << "新建 ClientEntity 默认 isAggressive=false";
}

TEST_F(ClientEntityZombieAggressiveSyncTest, Aggressive_SetAndGet)
{
    entity->setIsAggressive(true);
    EXPECT_TRUE(entity->isAggressive());

    entity->setIsAggressive(false);
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    // 模拟服务端广播 aggressive=true（位 2 置位）
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask())); // 0x04

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "zombie typeId + 位 2 置位 应同步到 isAggressive 镜像字段";
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_FlagCleared_KeepsAggressiveFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0)); // 位 2 未置位

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_ToggleTrueThenFalse)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));

    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive());

    setMobFlagsParam(*entity, static_cast<i8>(0));
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_NoParamRegistered_DoesNotCrash)
{
    // 未注册参数时 syncMetadataFromDataManager 不应崩溃
    // hasParam 返回 false，分支自然跳过
    EXPECT_FALSE(entity->isAggressive());
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive());
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_ReadsCorrectParamId)
{
    // 验证 syncMetadataFromDataManager 读取的是 DATA_MOB_FLAGS_PARAM 的 ID，
    // 而非其他 i8 参数。注册一个不同 ID 的 i8 参数，确保不被误读。
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    // 额外注册一个无关的 i8 参数（ID 不同），设置为 0xFF（所有位置位）
    auto otherParam = ::mc::entity::EntityDataManager::createKey<i8>();
    entity->dataManager().registerParam(otherParam, static_cast<i8>(0));
    entity->dataManager().set(otherParam, static_cast<i8>(0xFF));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "应读取 DATA_MOB_FLAGS_PARAM 而非其他 i8 参数";
}

// ============================================================================
// 位隔离测试：syncMetadataFromDataManager 只读取位 2
// ============================================================================

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_OtherFlagsSet_Bit2Clear_NotAggressive)
{
    // 设置位 0 (NO_AI) 和位 1 (LEFTHANDED)，但不设置位 2 → isAggressive 应为 false
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0x03)); // 位 0 + 位 1

    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->isAggressive()) << "仅位 0/1 置位（位 2 未置位）时 isAggressive 应为 false";
}

TEST_F(ClientEntityZombieAggressiveSyncTest, SyncFromDataManager_AllFlagsSet_Bit2Set_Aggressive)
{
    // 设置所有位（0x07 = NO_AI | LEFTHANDED | AGGRESSIVE）→ isAggressive 应为 true
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(0x07));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "位 2 置位时 isAggressive 应为 true（即使其他位也置位）";
}

// ============================================================================
// 僵尸变体（husk/drowned/zombie_villager）同步测试
// ============================================================================

class ClientEntityHuskAggressiveSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:husk"); }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityHuskAggressiveSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "husk typeId + 位 2 置位 应同步到 isAggressive";
}

class ClientEntityDrownedAggressiveSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:drowned"); }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityDrownedAggressiveSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "drowned typeId + 位 2 置位 应同步到 isAggressive";
}

class ClientEntityZombieVillagerAggressiveSyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:zombie_villager"); }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityZombieVillagerAggressiveSyncTest, SyncFromDataManager_FlagSet_SetsIsAggressive)
{
    registerMobFlagsParam(*entity, static_cast<i8>(0));
    setMobFlagsParam(*entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));

    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->isAggressive()) << "zombie_villager typeId + 位 2 置位 应同步到 isAggressive";
}

// ============================================================================
// typeId 归一化测试（带/不带 "minecraft:" 前缀）
// ============================================================================

TEST(ClientEntityAggressiveTypeIdNormalizeTest, Zombie_WithoutPrefix_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "zombie");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "typeId='zombie'（不带前缀）应被正确识别";
}

// ============================================================================
// 通用 Mob 同步测试（非僵尸系 Mob 也通过 DATA_MOB_FLAGS_PARAM 同步）
// ============================================================================

TEST(ClientEntityGenericMobAggressiveSyncTest, Skeleton_SyncsAggressive)
{
    // 骷髅也是 MobEntity 子类，DATA_MOB_FLAGS_PARAM 通用同步
    ClientEntity entity(EntityInstanceId(1), "minecraft:skeleton");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "skeleton（任意 MobEntity 子类）应通过 DATA_MOB_FLAGS_PARAM 同步激怒状态";
}

TEST(ClientEntityGenericMobAggressiveSyncTest, Creeper_SyncsAggressive)
{
    ClientEntity entity(EntityInstanceId(1), "minecraft:creeper");
    EXPECT_FALSE(entity.isAggressive());

    registerMobFlagsParam(entity, static_cast<i8>(0));
    setMobFlagsParam(entity, static_cast<i8>(::mc::MobEntity::getAggressiveFlagMask()));
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.isAggressive()) << "creeper（任意 MobEntity 子类）应通过 DATA_MOB_FLAGS_PARAM 同步激怒状态";
}
