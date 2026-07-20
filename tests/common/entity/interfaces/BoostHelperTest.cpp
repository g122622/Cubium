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
 * @file BoostHelperTest.cpp
 * @brief BoostHelper NBT 序列化单元测试
 *
 * 测试 BoostHelper 类的 NBT 读写功能，验证：
 * - writeToNbt() 正确保存鞍状态
 * - readFromNbt() 正确读取鞍状态
 * - NBT 数据格式符合 MC 1.16.5 标准（使用 Byte 标签存储布尔值）
 */

#include <gtest/gtest.h>

#include "common/entity/interfaces/BoostHelper.hpp"
#include "entity/core/EntityDataManager.hpp"
#include "util/math/random/Random.hpp"
#include "util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::nbt;

// ============================================================================
// BoostHelper NBT 测试
// ============================================================================

/**
 * @brief 创建测试用的 BoostHelper
 * @param dataManager 数据管理器
 * @return 初始化好的 BoostHelper
 */
static BoostHelper createTestBoostHelper(entity::EntityDataManager& dataManager)
{
    auto boostTimeParam = entity::EntityDataManager::createKey<i32>();
    auto saddledParam = entity::EntityDataManager::createKey<bool>();
    dataManager.registerParam(boostTimeParam, static_cast<i32>(0));
    dataManager.registerParam(saddledParam, false);

    BoostHelper helper;
    helper.init(dataManager, boostTimeParam, saddledParam);
    return helper;
}

// ============================================================================
// 初始化测试
// ============================================================================

TEST(BoostHelper, Initialization)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    EXPECT_TRUE(helper.isInitialized());
    EXPECT_FALSE(helper.getSaddled());
    EXPECT_EQ(helper.getBoostTime(), 0);
    EXPECT_FALSE(helper.isBoosting());
}

// ============================================================================
// 鞍状态测试
// ============================================================================

TEST(BoostHelper, SetSaddledFromBoolean)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 初始状态：无鞍
    EXPECT_FALSE(helper.getSaddled());

    // 设置鞍
    helper.setSaddledFromBoolean(true);
    EXPECT_TRUE(helper.getSaddled());

    // 移除鞍
    helper.setSaddledFromBoolean(false);
    EXPECT_FALSE(helper.getSaddled());
}

// ============================================================================
// 加速测试
// ============================================================================

TEST(BoostHelper, Boost)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);
    math::Random rng(12345);

    // 初始状态：未加速
    EXPECT_FALSE(helper.isBoosting());

    // 触发加速
    bool boosted = helper.boost(rng);
    EXPECT_TRUE(boosted);
    EXPECT_TRUE(helper.isBoosting());
    EXPECT_TRUE(helper.saddledRaw);
    EXPECT_EQ(helper.field_233611_b_, 0);

    // 验证加速时间在有效范围内 [140, 980]
    EXPECT_GE(helper.boostTimeRaw, 140);
    EXPECT_LE(helper.boostTimeRaw, 980);

    // 再次加速应该失败（已经在加速中）
    boosted = helper.boost(rng);
    EXPECT_FALSE(boosted);
}

TEST(BoostHelper, Tick)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);
    math::Random rng(12345);

    // 触发加速
    helper.boost(rng);
    i32 boostTime = helper.boostTimeRaw;
    EXPECT_GT(boostTime, 0);

    // Tick 更新
    for (i32 i = 0; i < boostTime; ++i) {
        EXPECT_TRUE(helper.tick());
        EXPECT_TRUE(helper.isBoosting());
    }

    // 加速结束
    EXPECT_FALSE(helper.tick());
    EXPECT_FALSE(helper.isBoosting());
    EXPECT_FALSE(helper.saddledRaw);
}

// ============================================================================
// NBT 写入测试
// ============================================================================

TEST(BoostHelper, WriteToNbt_WithoutSaddle)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    tags::compound_tag tag;
    helper.writeToNbt(tag);

    // 验证写入的标签
    auto it = tag.value.find("Saddle");
    ASSERT_NE(it, tag.value.end());
    EXPECT_EQ(it->second->id(), TagId::Byte);

    // 无鞍时应该写入 0
    i8 value = dynamic_cast<const tags::byte_tag&>(*it->second).value;
    EXPECT_EQ(value, 0);
}

TEST(BoostHelper, WriteToNbt_WithSaddle)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 设置鞍状态
    helper.setSaddledFromBoolean(true);

    tags::compound_tag tag;
    helper.writeToNbt(tag);

    // 验证写入的标签
    auto it = tag.value.find("Saddle");
    ASSERT_NE(it, tag.value.end());
    EXPECT_EQ(it->second->id(), TagId::Byte);

    // 有鞍时应该写入 1
    i8 value = dynamic_cast<const tags::byte_tag&>(*it->second).value;
    EXPECT_EQ(value, 1);
}

// ============================================================================
// NBT 读取测试
// ============================================================================

TEST(BoostHelper, ReadFromNbt_NoSaddle)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 创建包含无鞍状态的 NBT
    tags::compound_tag tag;
    tag.put("Saddle", static_cast<i8>(0));

    // 读取 NBT
    helper.readFromNbt(tag);
    EXPECT_FALSE(helper.getSaddled());
}

TEST(BoostHelper, ReadFromNbt_WithSaddle)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 创建包含有鞍状态的 NBT
    tags::compound_tag tag;
    tag.put("Saddle", static_cast<i8>(1));

    // 读取 NBT
    helper.readFromNbt(tag);
    EXPECT_TRUE(helper.getSaddled());
}

TEST(BoostHelper, ReadFromNbt_MissingKey)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 创建空 NBT（没有 Saddle 键）
    tags::compound_tag tag;

    // 读取 NBT 不应该崩溃，鞍状态应保持默认
    helper.readFromNbt(tag);
    EXPECT_FALSE(helper.getSaddled());
}

TEST(BoostHelper, ReadFromNbt_NonzeroValue)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);

    // 创建包含非零值的 NBT（任何非零值都应该被视为 true）
    tags::compound_tag tag;
    tag.put("Saddle", static_cast<i8>(42));

    // 读取 NBT
    helper.readFromNbt(tag);
    EXPECT_TRUE(helper.getSaddled());
}

// ============================================================================
// NBT 往返测试
// ============================================================================

TEST(BoostHelper, NbtRoundTrip_NoSaddle)
{
    entity::EntityDataManager dataManager1;
    auto helper1 = createTestBoostHelper(dataManager1);

    // 无鞍状态
    helper1.setSaddledFromBoolean(false);

    // 写入 NBT
    tags::compound_tag writeTag;
    helper1.writeToNbt(writeTag);

    // 创建新的 BoostHelper 并读取 NBT
    entity::EntityDataManager dataManager2;
    auto helper2 = createTestBoostHelper(dataManager2);
    helper2.readFromNbt(writeTag);

    // 验证状态一致
    EXPECT_EQ(helper2.getSaddled(), helper1.getSaddled());
    EXPECT_FALSE(helper2.getSaddled());
}

TEST(BoostHelper, NbtRoundTrip_WithSaddle)
{
    entity::EntityDataManager dataManager1;
    auto helper1 = createTestBoostHelper(dataManager1);

    // 有鞍状态
    helper1.setSaddledFromBoolean(true);

    // 写入 NBT
    tags::compound_tag writeTag;
    helper1.writeToNbt(writeTag);

    // 创建新的 BoostHelper 并读取 NBT
    entity::EntityDataManager dataManager2;
    auto helper2 = createTestBoostHelper(dataManager2);
    helper2.readFromNbt(writeTag);

    // 验证状态一致
    EXPECT_EQ(helper2.getSaddled(), helper1.getSaddled());
    EXPECT_TRUE(helper2.getSaddled());
}

// ============================================================================
// 加速状态不被持久化测试
// ============================================================================

TEST(BoostHelper, BoostStateNotPersisted)
{
    entity::EntityDataManager dataManager1;
    auto helper1 = createTestBoostHelper(dataManager1);
    math::Random rng(12345);

    // 触发加速
    helper1.boost(rng);
    EXPECT_TRUE(helper1.isBoosting());

    // 写入 NBT
    tags::compound_tag writeTag;
    helper1.writeToNbt(writeTag);

    // 创建新的 BoostHelper 并读取 NBT
    entity::EntityDataManager dataManager2;
    auto helper2 = createTestBoostHelper(dataManager2);
    helper2.readFromNbt(writeTag);

    // 加速状态不应该被持久化（新加载的实体不处于加速状态）
    EXPECT_FALSE(helper2.isBoosting());
    EXPECT_EQ(helper2.field_233611_b_, 0);
    EXPECT_EQ(helper2.boostTimeRaw, 0);
}

// ============================================================================
// syncFromDataManager 测试
// ============================================================================

TEST(BoostHelper, SyncFromDataManager)
{
    entity::EntityDataManager dataManager;
    auto helper = createTestBoostHelper(dataManager);
    math::Random rng(12345);

    // 触发加速（这会设置 boostTimeParam）
    helper.boost(rng);
    i32 boostTime = helper.boostTimeRaw;

    // 重置本地状态
    helper.saddledRaw = false;
    helper.field_233611_b_ = 100;
    helper.boostTimeRaw = 0;

    // 从数据管理器同步
    helper.syncFromDataManager();

    // 验证同步后的状态
    EXPECT_TRUE(helper.saddledRaw);
    EXPECT_EQ(helper.field_233611_b_, 0);
    EXPECT_EQ(helper.boostTimeRaw, boostTime);
}
