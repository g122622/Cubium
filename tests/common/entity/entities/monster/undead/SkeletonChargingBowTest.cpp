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
 * @file SkeletonChargingBowTest.cpp
 * @brief AbstractSkeletonEntity 拉弓状态（DATA_CHARGING_BOW_PARAM）单元测试
 *
 * 验证内容：
 * - registerData() 正确注册 DATA_CHARGING_BOW_PARAM（通过 hasParam 验证）
 * - isChargingBow/setChargingBow 通过 DataParameter 读写
 * - setChargingBow 标记脏数据（EntityTracker 自动广播）
 * - getChargingBowParamId 返回与服务端一致的参数 ID
 * - tick() 中 isUsingItem + 持弓状态正确驱动 chargingBow
 * - attackEntityWithRangedAttack 重置 chargingBow 为 false
 * - 子类（SkeletonEntity/StrayEntity/BoggedEntity）继承注册
 * - 凋灵骷髅（WitherSkeletonEntity）同样继承注册但不持弓
 *
 * 对应 MC 1.21.11 AbstractSkeletonRenderer.getArmPose：
 *   isAggressive && mainHandItem.is(Items.BOW) → BOW_AND_ARROW
 * 本项目用 chargingBow 布尔字段替代，由 tick 根据 isUsingItem + 持弓设置。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/BoggedEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/StrayEntity.hpp"
#include "common/entity/entities/monster/undead/WitherSkeletonEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试世界 - 支持骷髅拉弓状态测试所需的最小 IWorld 接口
// ============================================================================

class SkeletonChargingBowTestWorld final : public test::BaseTestWorld {
public:
    SkeletonChargingBowTestWorld() = default;

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

private:
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_clientSide = false;
};

// ============================================================================
// 测试夹具
// ============================================================================

class SkeletonChargingBowTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<SkeletonChargingBowTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<SkeletonChargingBowTestWorld> m_world;
};

// ============================================================================
// DATA_CHARGING_BOW_PARAM 注册测试
// ============================================================================

TEST_F(SkeletonChargingBowTest, DataChargingBowParam_RegisteredOnConstruction_Skeleton)
{
    // AbstractSkeletonEntity 构造函数显式调用 registerData()（参考 WolfEntity 模式），
    // 注册 DATA_CHARGING_BOW_PARAM，默认值 false。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    auto& dm = skeleton->dataManager();
    EXPECT_TRUE(dm.hasParam(paramId)) << "DATA_CHARGING_BOW_PARAM 必须在 AbstractSkeletonEntity::registerData 中注册";
}

TEST_F(SkeletonChargingBowTest, DataChargingBowParam_RegisteredOnConstruction_Stray)
{
    auto stray = std::make_unique<StrayEntity>(EntityId(1));
    stray->setWorld(m_world.get());

    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    auto& dm = stray->dataManager();
    EXPECT_TRUE(dm.hasParam(paramId)) << "StrayEntity 应继承 AbstractSkeletonEntity 的 DATA_CHARGING_BOW_PARAM 注册";
}

TEST_F(SkeletonChargingBowTest, DataChargingBowParam_RegisteredOnConstruction_Bogged)
{
    auto bogged = std::make_unique<BoggedEntity>(EntityId(1));
    bogged->setWorld(m_world.get());

    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    auto& dm = bogged->dataManager();
    EXPECT_TRUE(dm.hasParam(paramId)) << "BoggedEntity 应继承 AbstractSkeletonEntity 的 DATA_CHARGING_BOW_PARAM 注册";
}

TEST_F(SkeletonChargingBowTest, DataChargingBowParam_RegisteredOnConstruction_WitherSkeleton)
{
    // 凋灵骷髅不持弓（走 MeleeAttackGoal），但仍继承 AbstractSkeletonEntity 的 registerData，
    // DATA_CHARGING_BOW_PARAM 会注册但永远不会被设置为 true（不持弓、不拉弓）。
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));
    witherSkeleton->setWorld(m_world.get());

    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    auto& dm = witherSkeleton->dataManager();
    EXPECT_TRUE(dm.hasParam(paramId)) << "WitherSkeletonEntity 虽不持弓，但仍继承 DATA_CHARGING_BOW_PARAM 注册";
}

// ============================================================================
// isChargingBow / setChargingBow 读写测试
// ============================================================================

TEST_F(SkeletonChargingBowTest, IsChargingBow_DefaultFalse)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    EXPECT_FALSE(skeleton->isChargingBow()) << "拉弓状态默认应为 false";
}

TEST_F(SkeletonChargingBowTest, SetChargingBow_True_ReadsBackTrue)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));

    skeleton->setChargingBow(true);
    EXPECT_TRUE(skeleton->isChargingBow());
}

TEST_F(SkeletonChargingBowTest, SetChargingBow_False_ReadsBackFalse)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setChargingBow(true);

    skeleton->setChargingBow(false);
    EXPECT_FALSE(skeleton->isChargingBow());
}

TEST_F(SkeletonChargingBowTest, SetChargingBow_ToggleBackAndForth)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));

    skeleton->setChargingBow(true);
    EXPECT_TRUE(skeleton->isChargingBow());

    skeleton->setChargingBow(false);
    EXPECT_FALSE(skeleton->isChargingBow());

    skeleton->setChargingBow(true);
    EXPECT_TRUE(skeleton->isChargingBow());
}

// ============================================================================
// setChargingBow 标记脏数据测试（EntityTracker 自动广播依赖）
// ============================================================================

TEST_F(SkeletonChargingBowTest, SetChargingBow_True_MarksDirty)
{
    // setChargingBow 通过 m_dataManager.set 写入 DataParameter。
    // EntityTracker 监测 hasDirtyData() 并自动广播到客户端。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    auto& dm = skeleton->dataManager();
    dm.clearDirty(); // 清除构造期间可能的脏标记

    skeleton->setChargingBow(true);
    EXPECT_TRUE(dm.hasDirtyData()) << "拉弓状态变化应标记脏数据以触发网络广播";
}

TEST_F(SkeletonChargingBowTest, SetChargingBow_SameValue_NotDirty)
{
    // 设置相同值不应标记脏数据（EntityDataManager::set 的契约）
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setChargingBow(true);
    auto& dm = skeleton->dataManager();
    dm.clearDirty();

    skeleton->setChargingBow(true); // 相同值
    EXPECT_FALSE(dm.hasDirtyData()) << "设置相同值不应标记脏数据";
}

TEST_F(SkeletonChargingBowTest, SetChargingBow_False_AfterTrue_MarksDirty)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setChargingBow(true);
    auto& dm = skeleton->dataManager();
    dm.clearDirty();

    skeleton->setChargingBow(false);
    EXPECT_TRUE(dm.hasDirtyData()) << "从 true 变 false 应标记脏数据";
}

// ============================================================================
// getChargingBowParamId 一致性测试
// ============================================================================

TEST_F(SkeletonChargingBowTest, GetChargingBowParamId_ConsistentAcrossInstances)
{
    auto skeleton1 = std::make_unique<SkeletonEntity>(EntityId(1));
    auto skeleton2 = std::make_unique<SkeletonEntity>(EntityId(2));
    auto stray = std::make_unique<StrayEntity>(EntityId(3));

    // DATA_CHARGING_BOW_PARAM 是静态成员，所有实例共享同一个参数 ID
    const u16 id1 = AbstractSkeletonEntity::getChargingBowParamId();
    const u16 id2 = AbstractSkeletonEntity::getChargingBowParamId();
    EXPECT_EQ(id1, id2);

    // 客户端 ClientEntity::syncMetadataFromDataManager 依赖此 ID 与服务端匹配
    EXPECT_TRUE(skeleton1->dataManager().hasParam(id1));
    EXPECT_TRUE(skeleton2->dataManager().hasParam(id1));
    EXPECT_TRUE(stray->dataManager().hasParam(id1));
}

// ============================================================================
// DataParameter 直接读写验证（绕过 setter，验证底层存储）
// ============================================================================

TEST_F(SkeletonChargingBowTest, SetChargingBow_WritesToDataManager)
{
    // 验证 setChargingBow 确实通过 DataParameter::set 写入，
    // 而非写入某个孤立的成员变量。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));

    skeleton->setChargingBow(true);

    // 直接从 dataManager 读取，验证与 isChargingBow 一致
    auto& dm = skeleton->dataManager();
    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    const auto* raw = dm.getRaw(paramId);
    ASSERT_NE(raw, nullptr);
    EXPECT_TRUE(raw->get<bool>());

    EXPECT_TRUE(skeleton->isChargingBow());
}

TEST_F(SkeletonChargingBowTest, IsChargingBow_ReadsFromDataManager)
{
    // 反向验证：直接通过 DataParameter 写入，isChargingBow 应读到新值。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));

    // 构造一个与服务端 DATA_CHARGING_BOW_PARAM 相同 ID 的参数键
    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    entity::DataParameter<bool> param(paramId);

    auto& dm = skeleton->dataManager();
    dm.set(param, true);

    EXPECT_TRUE(skeleton->isChargingBow()) << "isChargingBow 应直接读取 DataParameter 的值";
}

// ============================================================================
// tick() 驱动 chargingBow 逻辑（间接验证）
//
// AbstractSkeletonEntity::tick 中：
//   nowCharging = isUsingItem() && getMainHandItem().getItem() == Items::BOW
//   if (wasCharging != nowCharging) setChargingBow(nowCharging)
//
// 由于 tick() 会触发 LivingEntity::tick → Entity::tick 的完整链路，
// 需要 tickManager 等基础设施。此处采用间接验证策略：
// - 验证 isUsingItem() 在未调用 setActiveHand 时返回 false
// - 验证持弓前置条件（getMainHandItem 可正确返回 BOW）
// - 验证 setChargingBow 可被外部调用（attackEntityWithRangedAttack 用此重置）
// ============================================================================

TEST_F(SkeletonChargingBowTest, Tick_Preconditions_IsUsingItemDefaultFalse)
{
    // 验证 tick() 中 nowCharging 计算的前置条件：
    // 未调用 setActiveHand 时 isUsingItem() 必须返回 false，
    // 因此 nowCharging = false && ... = false，不会误设 chargingBow。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    EXPECT_FALSE(skeleton->isUsingItem()) << "未调用 setActiveHand 时 isUsingItem 应为 false";
}

TEST_F(SkeletonChargingBowTest, Tick_Preconditions_BowEquipped_MainHandReturnsBow)
{
    // 验证持弓时 getMainHandItem().getItem() == Items::BOW，
    // 配合 setActiveHand(MainHand) 后 isUsingItem=true，
    // tick() 中 nowCharging 计算应为 true。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 装备弓
    ASSERT_NE(Items::BOW, nullptr);
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));

    const auto& mainHand = skeleton->getMainHandItem();
    ASSERT_NE(mainHand.getItem(), nullptr);
    EXPECT_EQ(mainHand.getItem(), Items::BOW);
}

TEST_F(SkeletonChargingBowTest, Tick_Preconditions_NoBow_MainHandNotBow)
{
    // 空手时 getMainHandItem().getItem() == nullptr，不等于 Items::BOW，
    // 即使 isUsingItem=true，nowCharging 也为 false。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    const auto& mainHand = skeleton->getMainHandItem();
    EXPECT_EQ(mainHand.getItem(), nullptr);
    EXPECT_NE(mainHand.getItem(), Items::BOW);
}

TEST_F(SkeletonChargingBowTest, Tick_Preconditions_SetActiveHand_MarksUsingItem)
{
    // 持弓并调用 setActiveHand(MainHand) 后，isUsingItem 应返回 true。
    // 此时 tick() 中 nowCharging = true && (BOW==BOW) = true。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    ASSERT_NE(Items::BOW, nullptr);
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));

    EXPECT_FALSE(skeleton->isUsingItem());
    skeleton->setActiveHand(Hand::MainHand);
    EXPECT_TRUE(skeleton->isUsingItem()) << "setActiveHand 后 isUsingItem 应为 true";

    // 清理：停止使用物品
    skeleton->stopActiveHand();
    EXPECT_FALSE(skeleton->isUsingItem());
}

// ============================================================================
// attackEntityWithRangedAttack 重置 chargingBow 测试
//
// attackEntityWithRangedAttack 中调用 setChargingBow(false) 重置拉弓状态。
// 此处验证 setChargingBow(false) 可正确重置（attackEntityWithRangedAttack
// 完整调用需要 world + target + 箭矢创建，过于重量级，此处验证核心契约）。
// ============================================================================

TEST_F(SkeletonChargingBowTest, AttackEntityWithRangedAttack_Preconditions_ChargingBowResettable)
{
    // attackEntityWithRangedAttack 开头调用 setChargingBow(false)。
    // 验证拉弓状态可通过 setChargingBow(false) 重置，
    // 这是 attackEntityWithRangedAttack 射击后重置拉弓的基础。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setChargingBow(true);
    ASSERT_TRUE(skeleton->isChargingBow());

    skeleton->setChargingBow(false);
    EXPECT_FALSE(skeleton->isChargingBow()) << "attackEntityWithRangedAttack 应通过 setChargingBow(false) 重置";
}

// ============================================================================
// 子类继承一致性测试
// ============================================================================

TEST_F(SkeletonChargingBowTest, Stray_ChargingBow_InheritsFromAbstractSkeleton)
{
    auto stray = std::make_unique<StrayEntity>(EntityId(1));
    EXPECT_FALSE(stray->isChargingBow());

    stray->setChargingBow(true);
    EXPECT_TRUE(stray->isChargingBow());

    stray->setChargingBow(false);
    EXPECT_FALSE(stray->isChargingBow());
}

TEST_F(SkeletonChargingBowTest, Bogged_ChargingBow_InheritsFromAbstractSkeleton)
{
    auto bogged = std::make_unique<BoggedEntity>(EntityId(1));
    EXPECT_FALSE(bogged->isChargingBow());

    bogged->setChargingBow(true);
    EXPECT_TRUE(bogged->isChargingBow());

    bogged->setChargingBow(false);
    EXPECT_FALSE(bogged->isChargingBow());
}

TEST_F(SkeletonChargingBowTest, WitherSkeleton_ChargingBow_InheritsButNeverSet)
{
    // 凋灵骷髅继承 DATA_CHARGING_BOW_PARAM（hasParam=true），
    // 但因不持弓（走 MeleeAttackGoal），tick 中 nowCharging 永远为 false，
    // setChargingBow(true) 永远不会被调用。
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));
    EXPECT_FALSE(witherSkeleton->isChargingBow());

    // 验证参数已注册（继承自 AbstractSkeletonEntity）
    const u16 paramId = AbstractSkeletonEntity::getChargingBowParamId();
    EXPECT_TRUE(witherSkeleton->dataManager().hasParam(paramId));

    // 凋灵骷髅默认不持弓
    const auto& mainHand = witherSkeleton->getMainHandItem();
    EXPECT_NE(mainHand.getItem(), Items::BOW);
}

} // namespace
} // namespace mc
