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
 * @brief AbstractSkeletonEntity 拉弓渲染状态同步机制单元测试
 *
 * 验证内容（对齐 vanilla 1.21.11 AbstractSkeletonRenderer.getArmPose）：
 * - AbstractSkeletonEntity 及其 4 子类（Skeleton/Stray/Bogged/WitherSkeleton）
 *   registerData 不注册任何 id16 SynchedEntityData 字段——vanilla Stray/
 *   WitherSkeleton 客户端访问器数组长度=16，发送 id16 致
 *   "Index 16 out of bounds for length 16" 崩溃，故项目删除原
 *   DATA_CHARGING_BOW_PARAM(id16)，本测试守护此对齐不被回退。
 * - 拉弓渲染状态改由 Mob.isAggressive（DATA_MOB_FLAGS_PARAM id15 位 2）同步，
 *   由 RangedBowAttackGoal::startExecuting/resetTask 经 setAggroed 写入。
 * - setAggressive/isAggressive 经 DATA_MOB_FLAGS_PARAM 位 2 读写。
 * - setAggressive 标记脏数据（EntityTracker 自动广播）。
 *
 * 对应 MC 1.21.11：
 *   AbstractSkeletonRenderer.getArmPose:
 *     getMainArm()==arm && isAggressive() && mainHandItem.is(Items.BOW) → BOW_AND_ARROW
 *   Mob.DATA_MOB_FLAGS_ID（id15）位 2 = MOB_FLAG_AGGRESSIVE
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
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
// 测试世界 - 支持骷髅激怒状态测试所需的最小 IWorld 接口
// ============================================================================

class SkeletonChargingBowTestWorld final : public mc::test::BaseTestWorld {
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
// 协议对齐核心：AbstractSkeleton 不注册 id16 字段
//
// vanilla 1.21.11 Stray/WitherSkeleton/Skeleton/Bogged 客户端 SynchedEntityData
// 访问器数组长度=16（id 0..15，末位即 Mob.DATA_MOB_FLAGS_ID=15）。项目曾注册
// DATA_CHARGING_BOW_PARAM(id16) 致真 Java 客户端 set_entity_data 越界崩溃
// "Index 16 out of bounds for length 16"。删除后 hasParam(16)==false，对齐 vanilla。
// 此组测试守护该对齐，防止 id16 字段被误加回。
// ============================================================================

TEST_F(SkeletonChargingBowTest, Skeleton_NoId16FieldRegistered_AlignedWithVanilla)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    auto& dm = skeleton->dataManager();

    // id16 必须未注册（vanilla Skeleton 客户端数组长度=16，无 id16 槽位）
    EXPECT_FALSE(dm.hasParam(16)) << "SkeletonEntity 不应注册 id16 字段（对齐 vanilla，防 set_entity_data 越界）";
    // id15（Mob.DATA_MOB_FLAGS_PARAM）必须已注册
    const u16 mobFlagsId = MobEntity::getMobFlagsParamId();
    EXPECT_TRUE(dm.hasParam(mobFlagsId)) << "MobEntity::DATA_MOB_FLAGS_PARAM(id15) 必须注册";
}

TEST_F(SkeletonChargingBowTest, Stray_NoId16FieldRegistered_AlignedWithVanilla)
{
    auto stray = std::make_unique<StrayEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    stray->setWorld(m_world.get());
    auto& dm = stray->dataManager();

    // vanilla Stray 客户端数组长度=16，收到 id16 即越界崩溃——正是本修复的触发场景
    EXPECT_FALSE(dm.hasParam(16)) << "StrayEntity 不应注册 id16 字段（vanilla Stray 数组长度=16）";
    EXPECT_TRUE(dm.hasParam(MobEntity::getMobFlagsParamId()));
}

TEST_F(SkeletonChargingBowTest, Bogged_NoId16FieldRegistered_AlignedWithVanilla)
{
    auto bogged = std::make_unique<BoggedEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    bogged->setWorld(m_world.get());
    auto& dm = bogged->dataManager();

    EXPECT_FALSE(dm.hasParam(16)) << "BoggedEntity 不应注册 id16 字段（vanilla Bogged 数组长度=16）";
    EXPECT_TRUE(dm.hasParam(MobEntity::getMobFlagsParamId()));
}

TEST_F(SkeletonChargingBowTest, WitherSkeleton_NoId16FieldRegistered_AlignedWithVanilla)
{
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    witherSkeleton->setWorld(m_world.get());
    auto& dm = witherSkeleton->dataManager();

    // vanilla WitherSkeleton 客户端数组长度=16，收到 id16 即越界崩溃——本修复触发场景
    EXPECT_FALSE(dm.hasParam(16)) << "WitherSkeletonEntity 不应注册 id16 字段（vanilla WitherSkeleton 数组长度=16）";
    EXPECT_TRUE(dm.hasParam(MobEntity::getMobFlagsParamId()));
}

// ============================================================================
// isAggressive / setAggressive 经 DATA_MOB_FLAGS_PARAM 位 2 读写
//
// 拉弓渲染状态不再有独立字段，改由 Mob.isAggressive（DATA_MOB_FLAGS_PARAM
// id15 位 2，MOB_FLAG_AGGRESSIVE=0x04）承载。RangedBowAttackGoal::startExecuting
// 调 setAggroed(true)（setAggressive 别名）置位，resetTask 调 setAggroed(false) 清位。
// ============================================================================

TEST_F(SkeletonChargingBowTest, IsAggressive_DefaultFalse)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    EXPECT_FALSE(skeleton->isAggressive()) << "激怒状态默认应为 false（未拉弓）";
}

TEST_F(SkeletonChargingBowTest, SetAggressive_True_ReadsBackTrue)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    skeleton->setAggressive(true);
    EXPECT_TRUE(skeleton->isAggressive());
}

TEST_F(SkeletonChargingBowTest, SetAggressive_False_ReadsBackFalse)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    skeleton->setAggressive(true);

    skeleton->setAggressive(false);
    EXPECT_FALSE(skeleton->isAggressive());
}

TEST_F(SkeletonChargingBowTest, SetAggroed_AliasForSetAggressive)
{
    // setAggroed 是 setAggressive 的别名（RangedBowAttackGoal 用 setAggroed）
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    EXPECT_FALSE(skeleton->isAggroed());
    skeleton->setAggroed(true);
    EXPECT_TRUE(skeleton->isAggroed());
    EXPECT_TRUE(skeleton->isAggressive()) << "setAggroed(true) 应等价于 setAggressive(true)";

    skeleton->setAggroed(false);
    EXPECT_FALSE(skeleton->isAggressive()) << "setAggroed(false) 应等价于 setAggressive(false)";
}

TEST_F(SkeletonChargingBowTest, SetAggressive_ToggleBackAndForth)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    skeleton->setAggressive(true);
    EXPECT_TRUE(skeleton->isAggressive());

    skeleton->setAggressive(false);
    EXPECT_FALSE(skeleton->isAggressive());

    skeleton->setAggressive(true);
    EXPECT_TRUE(skeleton->isAggressive());
}

// ============================================================================
// setAggressive 标记脏数据测试（EntityTracker 自动广播依赖）
//
// setAggressive 写 DATA_MOB_FLAGS_PARAM 位 2，值变化时标记脏数据，
// EntityTracker 监测 hasDirtyData() 并自动广播到客户端。
// ============================================================================

TEST_F(SkeletonChargingBowTest, SetAggressive_True_MarksDirty)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    auto& dm = skeleton->dataManager();
    dm.clearDirty(); // 清除构造期间可能的脏标记

    skeleton->setAggressive(true);
    EXPECT_TRUE(dm.hasDirtyData()) << "激怒状态变化应标记脏数据以触发网络广播";
}

TEST_F(SkeletonChargingBowTest, SetAggressive_SameValue_NotDirty)
{
    // 设置相同值不应标记脏数据（EntityDataManager::set 的契约）
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    skeleton->setAggressive(true);
    auto& dm = skeleton->dataManager();
    dm.clearDirty();

    skeleton->setAggressive(true); // 相同值
    EXPECT_FALSE(dm.hasDirtyData()) << "设置相同值不应标记脏数据";
}

TEST_F(SkeletonChargingBowTest, SetAggressive_False_AfterTrue_MarksDirty)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    skeleton->setAggressive(true);
    auto& dm = skeleton->dataManager();
    dm.clearDirty();

    skeleton->setAggressive(false);
    EXPECT_TRUE(dm.hasDirtyData()) << "从 true 变 false 应标记脏数据";
}

// ============================================================================
// DATA_MOB_FLAGS_PARAM 位 2 直接读写验证（绕过 setter，验证底层存储）
//
// setAggressive 写 DATA_MOB_FLAGS_PARAM 位 2（0x04），不影响其他位。
// 验证：置 aggressive 后 flags 的位 2 = 1；清后位 2 = 0。
// ============================================================================

TEST_F(SkeletonChargingBowTest, SetAggressive_SetsBit2OfMobFlags)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    skeleton->setAggressive(true);

    const u16 mobFlagsId = MobEntity::getMobFlagsParamId();
    auto& dm = skeleton->dataManager();
    const auto* raw = dm.getRaw(mobFlagsId);
    ASSERT_NE(raw, nullptr);
    const i8 flags = raw->get<i8>();
    EXPECT_NE(flags & static_cast<i8>(MobEntity::getAggressiveFlagMask()), 0)
        << "setAggressive(true) 应置 DATA_MOB_FLAGS_PARAM 位 2（0x04）";
}

TEST_F(SkeletonChargingBowTest, SetAggressiveFalse_ClearsBit2OfMobFlags)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());
    skeleton->setAggressive(true);

    skeleton->setAggressive(false);

    const u16 mobFlagsId = MobEntity::getMobFlagsParamId();
    auto& dm = skeleton->dataManager();
    const auto* raw = dm.getRaw(mobFlagsId);
    ASSERT_NE(raw, nullptr);
    const i8 flags = raw->get<i8>();
    EXPECT_EQ(flags & static_cast<i8>(MobEntity::getAggressiveFlagMask()), 0)
        << "setAggressive(false) 应清 DATA_MOB_FLAGS_PARAM 位 2";
}

TEST_F(SkeletonChargingBowTest, IsAggressive_ReadsBit2OfMobFlags)
{
    // 反向验证：直接通过 DataParameter 写位 2，isAggressive 应读到对应值。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    const u16 mobFlagsId = MobEntity::getMobFlagsParamId();
    entity::DataParameter<i8> param(mobFlagsId);

    auto& dm = skeleton->dataManager();
    dm.set(param, static_cast<i8>(MobEntity::getAggressiveFlagMask())); // 仅置位 2

    EXPECT_TRUE(skeleton->isAggressive()) << "isAggressive 应读取 DATA_MOB_FLAGS_PARAM 位 2";

    dm.set(param, static_cast<i8>(0)); // 清所有位
    EXPECT_FALSE(skeleton->isAggressive());
}

// ============================================================================
// 持弓前置条件验证（拉弓渲染 = isAggressive && 持弓）
//
// 对齐 vanilla AbstractSkeletonRenderer.getArmPose：
//   isAggressive && mainHandItem.is(Items.BOW) → BOW_AND_ARROW
// 客户端 _applySkeletonArmPose 同此判定。此处验证服务端持弓前置条件。
// ============================================================================

TEST_F(SkeletonChargingBowTest, BowEquipped_MainHandReturnsBow)
{
    // 验证持弓时 getMainHandItem().getItem() == Items::BOW，
    // 配合 isAggressive=true 即拉弓渲染条件。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    ASSERT_NE(Items::BOW, nullptr);
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));

    const auto& mainHand = skeleton->getMainHandItem();
    ASSERT_NE(mainHand.getItem(), nullptr);
    EXPECT_EQ(mainHand.getItem(), Items::BOW);
}

TEST_F(SkeletonChargingBowTest, NoBow_MainHandNotBow)
{
    // 空手时 getMainHandItem().getItem() == nullptr，不等于 Items::BOW，
    // 即使 isAggressive=true 也不进入 BowAndArrow（vanilla 判定）。
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    skeleton->setWorld(m_world.get());

    // SkeletonEntity 构造期主动装备弓（GameTest 的 test.spawn 不走 finalizeSpawn/
    // populateDefaultEquipmentSlots，故构造期补弓确保 spawn 的骷髅能远程攻击）。
    // 本测试验证"空手"场景：显式清空主手装备，确认 getItem()==nullptr 且非 BOW。
    skeleton->setMainHandItem(ItemStack());

    const auto& mainHand = skeleton->getMainHandItem();
    EXPECT_EQ(mainHand.getItem(), nullptr);
    EXPECT_NE(mainHand.getItem(), Items::BOW);
}

TEST_F(SkeletonChargingBowTest, WitherSkeleton_DefaultNotAggressive_NoBow)
{
    // 凋灵骷髅走 MeleeAttackGoal（不持弓、不拉弓），
    // 默认不激怒、主手非弓——拉弓渲染条件永不成立。
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    witherSkeleton->setWorld(m_world.get());

    EXPECT_FALSE(witherSkeleton->isAggressive());
    const auto& mainHand = witherSkeleton->getMainHandItem();
    EXPECT_NE(mainHand.getItem(), Items::BOW);
}

// ============================================================================
// 子类继承一致性测试
//
// 所有骷髅子类继承 MobEntity 的 isAggressive/setAggressive（经 DATA_MOB_FLAGS_PARAM），
// 无需各自覆写。验证 Stray/Bogged/WitherSkeleton 行为一致。
// ============================================================================

TEST_F(SkeletonChargingBowTest, Stray_Aggressive_InheritsFromMobEntity)
{
    auto stray = std::make_unique<StrayEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    stray->setWorld(m_world.get());
    EXPECT_FALSE(stray->isAggressive());

    stray->setAggressive(true);
    EXPECT_TRUE(stray->isAggressive());

    stray->setAggressive(false);
    EXPECT_FALSE(stray->isAggressive());
}

TEST_F(SkeletonChargingBowTest, Bogged_Aggressive_InheritsFromMobEntity)
{
    auto bogged = std::make_unique<BoggedEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    bogged->setWorld(m_world.get());
    EXPECT_FALSE(bogged->isAggressive());

    bogged->setAggressive(true);
    EXPECT_TRUE(bogged->isAggressive());

    bogged->setAggressive(false);
    EXPECT_FALSE(bogged->isAggressive());
}

TEST_F(SkeletonChargingBowTest, WitherSkeleton_Aggressive_InheritsFromMobEntity)
{
    // 凋灵骷髅虽走近战，但仍继承 MobEntity 的 aggressive 机制（MeleeAttackGoal 用之）
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    witherSkeleton->setWorld(m_world.get());
    EXPECT_FALSE(witherSkeleton->isAggressive());

    witherSkeleton->setAggressive(true);
    EXPECT_TRUE(witherSkeleton->isAggressive());

    witherSkeleton->setAggressive(false);
    EXPECT_FALSE(witherSkeleton->isAggressive());
}

} // namespace
} // namespace mc
