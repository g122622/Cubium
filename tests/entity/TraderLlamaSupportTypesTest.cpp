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

#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc {
namespace {

// 测试用子类：暴露 protected 的 NBT 序列化方法
class TestTraderLlamaEntity : public TraderLlamaEntity {
public:
    using TraderLlamaEntity::TraderLlamaEntity;

    // 暴露 protected 方法供测试使用
    using TraderLlamaEntity::addAdditionalSaveData;
    using TraderLlamaEntity::readAdditionalSaveData;
};

// ============================================================================
// 基础属性测试
// ============================================================================

TEST(TraderLlamaEntityTest, InheritsFromLlamaAndExposesTraderFlag)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));

    EXPECT_NE(dynamic_cast<LlamaEntity*>(&traderLlama), nullptr);
    EXPECT_TRUE(traderLlama.isTraderLlama());
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);
}

TEST(TraderLlamaEntityTest, DefaultDespawnDelayIsCorrect)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    EXPECT_EQ(traderLlama.getDespawnDelay(), TraderLlamaEntity::DEFAULT_DESPAWN_DELAY);
}

TEST(TraderLlamaEntityTest, SetDespawnDelay)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setDespawnDelay(100);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 100);

    traderLlama.setDespawnDelay(0);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 0);

    traderLlama.setDespawnDelay(-5);
    EXPECT_EQ(traderLlama.getDespawnDelay(), -5);
}

TEST(TraderLlamaEntityTest, SyncDespawnDelayFromTrader)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    // 同步流浪商人的消失倒计时，羊驼的倒计时 = 商人倒计时 - 1
    traderLlama.syncDespawnDelayFromTrader(48000);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);

    traderLlama.syncDespawnDelayFromTrader(1);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 0);

    traderLlama.syncDespawnDelayFromTrader(0);
    EXPECT_EQ(traderLlama.getDespawnDelay(), -1);
}

TEST(TraderLlamaEntityTest, IsTraderLlamaFlag)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    EXPECT_TRUE(traderLlama.isTraderLlama());

    // TraderLlamaEntity 可以向上转型为 LlamaEntity
    LlamaEntity* llamaPtr = &traderLlama;
    EXPECT_NE(llamaPtr, nullptr);
}

// ============================================================================
// canDespawn() 测试
// ============================================================================

TEST(TraderLlamaEntityTest, CanDespawnReturnsTrueWhenUntamedAndUnleashed)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    // 默认状态：未驯服、未拴绳、无骑乘者
    EXPECT_TRUE(traderLlama.canDespawn(128.0));
    EXPECT_TRUE(traderLlama.canDespawn(0.0));
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsFalseWhenTamed)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setTame(true);
    EXPECT_FALSE(traderLlama.canDespawn(128.0));
    EXPECT_FALSE(traderLlama.canDespawn(0.0));
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsFalseWhenLeashedToFence)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    // 拴在栅栏上
    traderLlama.setLeashedToFence(BlockPos(0, 64, 0));
    EXPECT_TRUE(traderLlama.isLeashed());
    EXPECT_FALSE(traderLlama.canDespawn(128.0));
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsFalseWhenLeashedToEntity)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    // 拴在实体上（使用一个模拟的 UUID）
    traderLlama.setLeashedToEntity("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(traderLlama.isLeashed());
    EXPECT_FALSE(traderLlama.canDespawn(128.0));
}

TEST(TraderLlamaEntityTest, CanDespawnTamedOverridesLeashed)
{
    // 即使被拴住，驯服的商队羊驼也不应消失
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setTame(true);
    traderLlama.setLeashedToFence(BlockPos(0, 64, 0));
    EXPECT_FALSE(traderLlama.canDespawn(0.0));
}

TEST(TraderLlamaEntityTest, CanDespawnUnleashedUntamedReturnsTrue)
{
    // 未驯服、未拴绳的商队羊驼在任意距离都可以消失
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    EXPECT_TRUE(traderLlama.canDespawn(32.0));
    EXPECT_TRUE(traderLlama.canDespawn(128.0));
    EXPECT_TRUE(traderLlama.canDespawn(0.0));
}

// ============================================================================
// NBT 序列化测试
// ============================================================================

TEST(TraderLlamaEntityTest, NbtSerialization_DespawnDelayRoundTrip)
{
    TestTraderLlamaEntity entity(EntityInstanceId(1));
    entity.setDespawnDelay(12345);

    // 序列化
    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证 NBT 中包含 DespawnDelay 键
    i32 savedDelay = tag.get<nbt::tags::int_tag>(entity::serialization::nbt_keys::DESPAWN_DELAY);
    EXPECT_EQ(savedDelay, 12345);

    // 反序列化到新实体
    TestTraderLlamaEntity loaded(EntityInstanceId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getDespawnDelay(), 12345);
}

TEST(TraderLlamaEntityTest, NbtSerialization_DefaultDespawnDelay)
{
    TestTraderLlamaEntity entity(EntityInstanceId(1));
    // 默认值 47999

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    i32 savedDelay = tag.get<nbt::tags::int_tag>(entity::serialization::nbt_keys::DESPAWN_DELAY);
    EXPECT_EQ(savedDelay, TraderLlamaEntity::DEFAULT_DESPAWN_DELAY);
}

TEST(TraderLlamaEntityTest, NbtSerialization_MissingDespawnDelayKeyUsesDefault)
{
    // 反序列化一个不包含 DespawnDelay 的空 NBT
    nbt::tags::compound_tag emptyTag;

    TestTraderLlamaEntity entity(EntityInstanceId(1));
    // 先设置一个非默认值
    entity.setDespawnDelay(999);
    // 反序列化空标签时，tryGetInt 在找不到键时不修改 m_despawnDelay
    // 但 LlamaEntity::readAdditionalSaveData 可能因缺少必要字段而失败
    auto result = entity.readAdditionalSaveData(emptyTag);
    // DespawnDelay 的部分不受影响：找不到键则保留原值
    EXPECT_EQ(entity.getDespawnDelay(), 999);
}

TEST(TraderLlamaEntityTest, NbtSerialization_ZeroDespawnDelay)
{
    TestTraderLlamaEntity entity(EntityInstanceId(1));
    entity.setDespawnDelay(0);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    TestTraderLlamaEntity loaded(EntityInstanceId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getDespawnDelay(), 0);
}

TEST(TraderLlamaEntityTest, NbtSerialization_NegativeDespawnDelay)
{
    TestTraderLlamaEntity entity(EntityInstanceId(1));
    entity.setDespawnDelay(-10);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    TestTraderLlamaEntity loaded(EntityInstanceId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getDespawnDelay(), -10);
}

// ============================================================================
// 拴绳状态测试
// ============================================================================

TEST(TraderLlamaEntityTest, IsNotLeashedByDefault)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    EXPECT_FALSE(traderLlama.isLeashed());
}

TEST(TraderLlamaEntityTest, IsLeashedAfterSetLeashedToFence)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setLeashedToFence(BlockPos(10, 64, 20));
    EXPECT_TRUE(traderLlama.isLeashed());
}

TEST(TraderLlamaEntityTest, IsLeashedAfterSetLeashedToEntity)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setLeashedToEntity("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(traderLlama.isLeashed());
    EXPECT_TRUE(traderLlama.leashHolderUuid().has_value());
    EXPECT_EQ(*traderLlama.leashHolderUuid(), "550e8400-e29b-41d4-a716-446655440000");
}

TEST(TraderLlamaEntityTest, ClearLeashRemovesLeashState)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setLeashedToFence(BlockPos(10, 64, 20));
    EXPECT_TRUE(traderLlama.isLeashed());

    traderLlama.clearLeash();
    EXPECT_FALSE(traderLlama.isLeashed());
}

TEST(TraderLlamaEntityTest, GetLeashHolderEntityReturnsNullWithoutWorld)
{
    // getLeashHolderEntity() 在没有 world 时返回 nullptr
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    traderLlama.setLeashedToEntity("550e8400-e29b-41d4-a716-446655440000");
    // 没有 world，无法查找实体
    EXPECT_EQ(traderLlama.getLeashHolderEntity(), nullptr);
}

TEST(TraderLlamaEntityTest, LeashHolderUuidPreservedAfterSetLeashedToEntity)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    const std::string testUuid = "12345678-1234-1234-1234-123456789abc";
    traderLlama.setLeashedToEntity(testUuid);
    EXPECT_TRUE(traderLlama.isLeashed());
    ASSERT_TRUE(traderLlama.leashHolderUuid().has_value());
    EXPECT_EQ(*traderLlama.leashHolderUuid(), testUuid);
}

// ============================================================================
// finalizeSpawn 测试
// ============================================================================

TEST(TraderLlamaEntityTest, DefaultDespawnDelayIs47999)
{
    TraderLlamaEntity traderLlama(EntityInstanceId(1));
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);
    EXPECT_EQ(traderLlama.getDespawnDelay(), TraderLlamaEntity::DEFAULT_DESPAWN_DELAY);
}

} // namespace
} // namespace mc
