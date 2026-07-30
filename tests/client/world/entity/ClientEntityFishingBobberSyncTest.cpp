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
 * @file ClientEntityFishingBobberSyncTest.cpp
 * @brief ClientEntity 钓鱼浮标元数据同步端到端单元测试
 *
 * 验证 FishingBobberEntity 的 DATA_HOOKED_ENTITY_PARAM / DATA_BITING_PARAM
 * 通过 EntityMetadataSerializer 序列化 → 反序列化 → syncMetadataFromDataManager
 * 完整链路正确同步到 ClientEntity 的镜像字段 m_fishingHookedEntityId / m_fishingBiting。
 *
 * 端到端数据流：
 * 服务端 FishingBobberEntity::_syncCaughtEntityId() / _catchingFish() / tick()
 *   → m_dataManager.set(DATA_HOOKED_ENTITY_PARAM / DATA_BITING_PARAM, ...)
 *   → EntityMetadataSerializer::serialize(dirtyOnly=true)
 *   → [网络传输字节流]
 *   → EntityMetadataSerializer::deserialize → ClientEntity::m_dataManager
 *   → ClientEntity::syncMetadataFromDataManager (fishing_bobber 分支)
 *   → m_fishingHookedEntityId / m_fishingBiting 镜像字段
 *   → fishingHookedEntityId() / fishingBiting() 访问器
 *
 * 对应 MC 1.21.11 FishingHook.onSyncedDataUpdated(DATA_HOOKED_ENTITY/DATA_BITING)。
 */

#include <gtest/gtest.h>

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/network/codec/EntityMetadataSerializer.hpp"

using namespace mc;
using namespace mc::client;

namespace {

/**
 * @brief 在 ClientEntity 的 dataManager 中注册与服务端
 *        FishingBobberEntity::DATA_HOOKED_ENTITY_PARAM / DATA_BITING_PARAM
 *        相同 ID 的参数，模拟反序列化后的状态。
 *
 * 服务端静态参数通过 EntityDataManager::createKey<T>() 分配全局唯一 ID，
 * 客户端 ClientEntity 拥有独立的 EntityDataManager 实例。
 * syncMetadataFromDataManager 通过 FishingBobberEntity::getHookedEntityParamId() /
 * getBitingParamId() 查找参数，因此客户端必须注册相同 ID 的参数才能被读取。
 */
void registerFishingParams(ClientEntity& entity)
{
    const u16 hookedId = ::mc::entity::FishingBobberEntity::getHookedEntityParamId();
    const u16 bitingId = ::mc::entity::FishingBobberEntity::getBitingParamId();
    ::mc::entity::DataParameter<i32> hookedParam(hookedId);
    ::mc::entity::DataParameter<bool> bitingParam(bitingId);
    entity.dataManager().registerParam(hookedParam, static_cast<i32>(0));
    entity.dataManager().registerParam(bitingParam, false);
}

void setHookedEntityParam(ClientEntity& entity, i32 value)
{
    const u16 paramId = ::mc::entity::FishingBobberEntity::getHookedEntityParamId();
    ::mc::entity::DataParameter<i32> param(paramId);
    entity.dataManager().set(param, value);
}

void setBitingParam(ClientEntity& entity, bool value)
{
    const u16 paramId = ::mc::entity::FishingBobberEntity::getBitingParamId();
    ::mc::entity::DataParameter<bool> param(paramId);
    entity.dataManager().set(param, value);
}

// ============================================================================
// 镜像字段默认值测试
// ============================================================================

class ClientEntityFishingBobberSyncTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 构造一次服务端 FishingBobberEntity 触发其 registerData()，使静态
        // DataParameter(DATA_HOOKED_ENTITY_PARAM/DATA_BITING_PARAM) 经继承链分配器
        // 获得真实 id（而非哨兵 0xFFFF）。否则 getHookedEntityParamId()/
        // getBitingParamId() 返回 0xFFFF，set 在 0xFFFF 建条目，被
        // syncMetadataFromDataManager 的 MobFlags 分支误读为 i8 触发 bad_variant_access。
        // 静态成员进程内幂等，首次构造即分配，后续复用。
        static const auto s_serverFishingBobber =
            std::make_unique<::mc::entity::FishingBobberEntity>(EntityInstanceId(1));
        (void)s_serverFishingBobber;

        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:fishing_bobber");
    }
    void TearDown() override { entity.reset(); }
    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityFishingBobberSyncTest, DefaultFishingHookedEntityIdIsZero)
{
    EXPECT_EQ(entity->fishingHookedEntityId(), 0);
}

TEST_F(ClientEntityFishingBobberSyncTest, DefaultFishingBitingIsFalse)
{
    EXPECT_FALSE(entity->fishingBiting());
}

// ============================================================================
// DATA_HOOKED_ENTITY 同步测试
// ============================================================================

TEST_F(ClientEntityFishingBobberSyncTest, SyncHookedEntity_Zero_KeepsDefault)
{
    registerFishingParams(*entity);
    setHookedEntityParam(*entity, 0);
    entity->syncMetadataFromDataManager();
    EXPECT_EQ(entity->fishingHookedEntityId(), 0);
}

TEST_F(ClientEntityFishingBobberSyncTest, SyncHookedEntity_NonZero_UpdatesMirror)
{
    registerFishingParams(*entity);
    // 模拟服务端钩住 entityId=42 的实体：存储 42+1=43
    setHookedEntityParam(*entity, 43);
    entity->syncMetadataFromDataManager();
    EXPECT_EQ(entity->fishingHookedEntityId(), 43);
}

TEST_F(ClientEntityFishingBobberSyncTest, SyncHookedEntity_TransitionFromHookedToNull)
{
    registerFishingParams(*entity);
    // 先钩住实体
    setHookedEntityParam(*entity, 43);
    entity->syncMetadataFromDataManager();
    EXPECT_EQ(entity->fishingHookedEntityId(), 43);

    // 然后清除（被钩实体失效）
    setHookedEntityParam(*entity, 0);
    entity->syncMetadataFromDataManager();
    EXPECT_EQ(entity->fishingHookedEntityId(), 0);
}

// ============================================================================
// DATA_BITING 同步测试
// ============================================================================

TEST_F(ClientEntityFishingBobberSyncTest, SyncBiting_True_UpdatesMirror)
{
    registerFishingParams(*entity);
    setBitingParam(*entity, true);
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->fishingBiting());
}

TEST_F(ClientEntityFishingBobberSyncTest, SyncBiting_False_KeepsMirrorFalse)
{
    registerFishingParams(*entity);
    setBitingParam(*entity, false);
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->fishingBiting());
}

TEST_F(ClientEntityFishingBobberSyncTest, SyncBiting_TransitionTrueThenFalse)
{
    registerFishingParams(*entity);

    // 鱼咬钩
    setBitingParam(*entity, true);
    entity->syncMetadataFromDataManager();
    EXPECT_TRUE(entity->fishingBiting());

    // 咬钩超时
    setBitingParam(*entity, false);
    entity->syncMetadataFromDataManager();
    EXPECT_FALSE(entity->fishingBiting());
}

// ============================================================================
// 未注册参数时不崩溃
// ============================================================================

TEST_F(ClientEntityFishingBobberSyncTest, SyncWithoutRegisteredParams_DoesNotCrash)
{
    // 未注册参数时 syncMetadataFromDataManager 不应崩溃
    // hasParam 返回 false，分支自然跳过
    EXPECT_EQ(entity->fishingHookedEntityId(), 0);
    EXPECT_FALSE(entity->fishingBiting());
    entity->syncMetadataFromDataManager();
    EXPECT_EQ(entity->fishingHookedEntityId(), 0);
    EXPECT_FALSE(entity->fishingBiting());
}

// ============================================================================
// typeId 归一化测试（带/不带 "minecraft:" 前缀）
// ============================================================================

TEST(ClientEntityFishingBobberTypeIdNormalizeTest, WithoutPrefix_SyncsHookedEntity)
{
    // 独立 TEST 须自行构造服务端 FishingBobberEntity 触发 registerData() 分配真实 id，
    // 否则 getHookedEntityParamId() 返回哨兵 0xFFFF 致 set 在 0xFFFF 建条目，
    // 被 syncMetadataFromDataManager 的 MobFlags 分支误读为 i8 触发 bad_variant_access。
    static const auto s_serverFishingBobber = std::make_unique<::mc::entity::FishingBobberEntity>(EntityInstanceId(1));
    (void)s_serverFishingBobber;

    ClientEntity entity(EntityInstanceId(1), "fishing_bobber");
    registerFishingParams(entity);
    setHookedEntityParam(entity, 43);
    entity.syncMetadataFromDataManager();
    EXPECT_EQ(entity.fishingHookedEntityId(), 43) << "typeId='fishing_bobber'（不带前缀）应被正确识别";
}

TEST(ClientEntityFishingBobberTypeIdNormalizeTest, WithoutPrefix_SyncsBiting)
{
    static const auto s_serverFishingBobber = std::make_unique<::mc::entity::FishingBobberEntity>(EntityInstanceId(1));
    (void)s_serverFishingBobber;

    ClientEntity entity(EntityInstanceId(1), "fishing_bobber");
    registerFishingParams(entity);
    setBitingParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_TRUE(entity.fishingBiting()) << "typeId='fishing_bobber'（不带前缀）应被正确识别";
}

// ============================================================================
// 非钓鱼浮标实体不触发同步测试
// ============================================================================

TEST(ClientEntityNonFishingBobberSyncTest, Zombie_DoesNotSyncFishingParams)
{
    // 独立 TEST 须自行构造服务端 FishingBobberEntity 触发 registerData() 分配真实 id，
    // 否则 getHookedEntityParamId()/getBitingParamId() 返回哨兵 0xFFFF 致 set 在 0xFFFF
    // 建条目，被 syncMetadataFromDataManager 的 MobFlags 分支误读为 i8 触发 bad_variant_access。
    static const auto s_serverFishingBobber = std::make_unique<::mc::entity::FishingBobberEntity>(EntityInstanceId(1));
    (void)s_serverFishingBobber;

    ClientEntity entity(EntityInstanceId(1), "minecraft:zombie");
    registerFishingParams(entity);
    setHookedEntityParam(entity, 43);
    setBitingParam(entity, true);
    entity.syncMetadataFromDataManager();
    EXPECT_EQ(entity.fishingHookedEntityId(), 0) << "zombie typeId 不应触发钓鱼浮标同步";
    EXPECT_FALSE(entity.fishingBiting()) << "zombie typeId 不应触发钓鱼浮标同步";
}

// ============================================================================
// 端到端序列化/反序列化测试
//
// 验证完整链路：
//   服务端 EntityDataManager (dirty) → EntityMetadataSerializer::serialize
//   → 字节流 → EntityMetadataSerializer::deserialize → 客户端 EntityDataManager
//   → syncMetadataFromDataManager → 镜像字段
// 对应 EntityTracker::tick() 中 serialize(manager, true) + 广播 +
// ClientEntity::setMetadata 中 deserialize 的真实路径。
// ============================================================================

class FishingBobberEndToEndSyncTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 先构造一次服务端 FishingBobberEntity 触发 registerData()，使静态
        // DataParameter 获得真实 id（而非哨兵 0xFFFF）。注：FishingBobberEntity
        // 构造仅需 EntityInstanceId（不依赖 IWorld），此前注释"构造需要 IWorld"
        // 为旧分配器时期的误判。本夹具仍用独立 EntityDataManager 验证序列化链路，
        // 但参数 id 须与生产一致，故须先触发分配。
        static const auto s_serverFishingBobber =
            std::make_unique<::mc::entity::FishingBobberEntity>(EntityInstanceId(1));
        (void)s_serverFishingBobber;

        // 服务端数据管理器（模拟 FishingBobberEntity 的 m_dataManager）
        // 此处直接使用 EntityDataManager 并注册相同 ID 的参数，验证序列化/反序列化链路。
        serverManager = std::make_unique<::mc::entity::EntityDataManager>();

        const u16 hookedId = ::mc::entity::FishingBobberEntity::getHookedEntityParamId();
        const u16 bitingId = ::mc::entity::FishingBobberEntity::getBitingParamId();
        ::mc::entity::DataParameter<i32> hookedParam(hookedId);
        ::mc::entity::DataParameter<bool> bitingParam(bitingId);
        serverManager->registerParam(hookedParam, static_cast<i32>(0));
        serverManager->registerParam(bitingParam, false);

        // 客户端实体
        clientEntity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:fishing_bobber");
    }

    void TearDown() override
    {
        serverManager.reset();
        clientEntity.reset();
    }

    /// 模拟 EntityTracker::tick() 的元数据同步路径：
    /// serialize(dirtyOnly=true) → 广播 → 客户端 setMetadata → syncMetadataFromDataManager
    void syncToClient()
    {
        std::vector<u8> metadata = ::mc::network::EntityMetadataSerializer::serialize(*serverManager, true);
        // serialize 在无脏数据时可能返回空或仅结束标记，deserialize 需要安全处理
        if (!metadata.empty()) {
            ::mc::network::EntityMetadataSerializer::deserialize(metadata, clientEntity->dataManager());
        }
        clientEntity->syncMetadataFromDataManager();
        serverManager->clearDirty();
    }

    void setServerHookedEntity(i32 value)
    {
        const u16 paramId = ::mc::entity::FishingBobberEntity::getHookedEntityParamId();
        ::mc::entity::DataParameter<i32> param(paramId);
        serverManager->set(param, value);
    }

    void setServerBiting(bool value)
    {
        const u16 paramId = ::mc::entity::FishingBobberEntity::getBitingParamId();
        ::mc::entity::DataParameter<bool> param(paramId);
        serverManager->set(param, value);
    }

    std::unique_ptr<::mc::entity::EntityDataManager> serverManager;
    std::unique_ptr<ClientEntity> clientEntity;
};

TEST_F(FishingBobberEndToEndSyncTest, HookedEntity_SerializesAndAppliesToClient)
{
    // 服务端钩住 entityId=42 的实体：存储 42+1=43
    setServerHookedEntity(43);
    syncToClient();

    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);
}

TEST_F(FishingBobberEndToEndSyncTest, Biting_SerializesAndAppliesToClient)
{
    // 服务端鱼咬钩
    setServerBiting(true);
    syncToClient();

    EXPECT_TRUE(clientEntity->fishingBiting());
}

TEST_F(FishingBobberEndToEndSyncTest, BothParams_SerializeAndApplyToClient)
{
    // 同时设置两个参数
    setServerHookedEntity(43);
    setServerBiting(true);
    syncToClient();

    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);
    EXPECT_TRUE(clientEntity->fishingBiting());
}

TEST_F(FishingBobberEndToEndSyncTest, Transition_HookedThenCleared)
{
    // 先钩住实体
    setServerHookedEntity(43);
    syncToClient();
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);

    // 被钩实体失效，服务端清除
    setServerHookedEntity(0);
    syncToClient();
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 0);
}

TEST_F(FishingBobberEndToEndSyncTest, Transition_BitingTrueThenFalse)
{
    // 鱼咬钩
    setServerBiting(true);
    syncToClient();
    EXPECT_TRUE(clientEntity->fishingBiting());

    // 咬钩超时
    setServerBiting(false);
    syncToClient();
    EXPECT_FALSE(clientEntity->fishingBiting());
}

TEST_F(FishingBobberEndToEndSyncTest, NoDirtyData_DoesNotChangeClient)
{
    // 首次同步设置值
    setServerHookedEntity(43);
    setServerBiting(true);
    syncToClient();
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);
    EXPECT_TRUE(clientEntity->fishingBiting());

    // 再次同步但无脏数据（值未变化）——客户端镜像应保持不变
    syncToClient();
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);
    EXPECT_TRUE(clientEntity->fishingBiting());
}

TEST_F(FishingBobberEndToEndSyncTest, DirtyOnly_SkipsUnchangedParam)
{
    // 同时设置两个参数并同步
    setServerHookedEntity(43);
    setServerBiting(true);
    syncToClient();
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 43);
    EXPECT_TRUE(clientEntity->fishingBiting());

    // 仅修改 hookedEntity，biting 保持不变
    setServerHookedEntity(100);
    syncToClient();

    // hookedEntity 应更新
    EXPECT_EQ(clientEntity->fishingHookedEntityId(), 100);
    // biting 应保持上次的 true（未变化未序列化）
    EXPECT_TRUE(clientEntity->fishingBiting());
}

} // namespace
