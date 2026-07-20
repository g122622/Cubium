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
 * @file MobEntityAggressiveFlagTest.cpp
 * @brief MobEntity DATA_MOB_FLAGS_PARAM 位 2 (aggressive) 同步单元测试
 *
 * 验证 MobEntity::setAggressive / isAggressive 正确读写 DATA_MOB_FLAGS_PARAM
 * 的位 2 (0x04)，对应 MC 1.21.11 Mob.MOB_FLAG_AGGRESSIVE。
 *
 * 同时验证 isAggroed / setAggroed 作为向后兼容委托方法的行为：
 *   - isAggroed() 委托 isAggressive()
 *   - setAggroed(bool) 委托 setAggressive(bool)
 *
 * 数据流：
 *   setAggressive(true) → m_aggroed=true + DATA_MOB_FLAGS_PARAM |= 0x04
 *   setAggressive(false) → m_aggroed=false + DATA_MOB_FLAGS_PARAM &= ~0x04
 *   isAggressive() → (DATA_MOB_FLAGS_PARAM & 0x04) != 0
 *
 * 对应 MC 1.21.11 Mob：
 *   public boolean isAggressive() { return (this.entityData.get(DATA_MOB_FLAGS_ID) & 4) != 0; }
 *   public void setAggressive(boolean aggressive) {
 *       byte b0 = this.entityData.get(DATA_MOB_FLAGS_ID);
 *       this.entityData.set(DATA_MOB_FLAGS_ID, (byte)(aggressive ? b0 | 4 : b0 & -5));
 *   }
 */

#include <gtest/gtest.h>

#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"

using namespace mc;

namespace {

// 测试用 MobEntity 子类
// MobEntity 是抽象基类（含纯虚函数），需通过派生类实例化。
// 构造时 MobEntity::MobEntity 已显式调用 registerData() 注册 DATA_MOB_FLAGS_PARAM。
class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    /**
     * @brief 暴露 DATA_MOB_FLAGS_PARAM 的参数 ID 供测试直接读取原始 i8 值
     *
     * DATA_MOB_FLAGS_PARAM 是 MobEntity 的 protected static 成员，
     * 测试通过 getMobFlagsParamId() 静态访问器获取 ID 后用 dataManager().get<i8>(id)
     * 读取原始字节，以验证位 2 的置位/清除不破坏其他位。
     */
    [[nodiscard]] i8 readRawMobFlags() const
    {
        const u16 paramId = MobEntity::getMobFlagsParamId();
        const ::mc::entity::DataParameter<i8> param(paramId);
        return dataManager().get<i8>(param);
    }

    /**
     * @brief 直接设置 DATA_MOB_FLAGS_PARAM 的原始字节（用于测试位隔离）
     *
     * 用于验证 setAggressive 只修改位 2，不影响其他位（如 NO_AI、LEFTHANDED）。
     */
    void writeRawMobFlags(i8 value)
    {
        const u16 paramId = MobEntity::getMobFlagsParamId();
        const ::mc::entity::DataParameter<i8> param(paramId);
        const_cast<::mc::entity::EntityDataManager&>(dataManager()).set(param, value);
    }
};

} // namespace

// ============================================================================
// 默认状态测试
// ============================================================================

TEST(MobEntityAggressiveFlagTest, Default_IsNotAggressive)
{
    TestMobEntity entity;
    EXPECT_FALSE(entity.isAggressive()) << "新建的 MobEntity 默认不处于激怒状态";
}

TEST(MobEntityAggressiveFlagTest, Default_RawFlagsZero)
{
    TestMobEntity entity;
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0)) << "新建的 MobEntity DATA_MOB_FLAGS_PARAM 应为 0";
}

// ============================================================================
// setAggressive / isAggressive 往返测试
// ============================================================================

TEST(MobEntityAggressiveFlagTest, SetAggressive_True_SetsFlagAndField)
{
    TestMobEntity entity;
    entity.setAggressive(true);

    EXPECT_TRUE(entity.isAggressive()) << "setAggressive(true) 后 isAggressive() 应为 true";
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(MobEntity::getAggressiveFlagMask()))
        << "setAggressive(true) 后 DATA_MOB_FLAGS_PARAM 位 2 应置位（值为 0x04）";
}

TEST(MobEntityAggressiveFlagTest, SetAggressive_TrueThenFalse_ClearsFlag)
{
    TestMobEntity entity;
    entity.setAggressive(true);
    EXPECT_TRUE(entity.isAggressive());

    entity.setAggressive(false);
    EXPECT_FALSE(entity.isAggressive()) << "setAggressive(false) 后 isAggressive() 应为 false";
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0)) << "setAggressive(false) 后 DATA_MOB_FLAGS_PARAM 应回到 0";
}

TEST(MobEntityAggressiveFlagTest, SetAggressive_ToggleMultipleTimes_StaysConsistent)
{
    TestMobEntity entity;
    for (i32 i = 0; i < 5; ++i) {
        entity.setAggressive(true);
        ASSERT_TRUE(entity.isAggressive()) << "第 " << i << " 次 setAggressive(true) 后应为激怒";
        entity.setAggressive(false);
        ASSERT_FALSE(entity.isAggressive()) << "第 " << i << " 次 setAggressive(false) 后应非激怒";
    }
}

// ============================================================================
// 位隔离测试：setAggressive 只修改位 2，不影响其他位
// ============================================================================

TEST(MobEntityAggressiveFlagTest, SetAggressive_True_PreservesOtherFlags)
{
    // 先设置位 0 (NO_AI=0x01) 和位 1 (LEFTHANDED=0x02)，值为 0x03
    TestMobEntity entity;
    entity.writeRawMobFlags(static_cast<i8>(0x03));

    entity.setAggressive(true);

    // 位 0、位 1 应保留，位 2 应置位 → 0x07
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0x07))
        << "setAggressive(true) 应只置位 2，保留位 0 (NO_AI) 和位 1 (LEFTHANDED)";
    EXPECT_TRUE(entity.isAggressive());
}

TEST(MobEntityAggressiveFlagTest, SetAggressive_False_PreservesOtherFlags)
{
    // 先设置所有位：0x07 (NO_AI | LEFTHANDED | AGGRESSIVE)
    TestMobEntity entity;
    entity.writeRawMobFlags(static_cast<i8>(0x07));

    entity.setAggressive(false);

    // 位 0、位 1 应保留，位 2 应清除 → 0x03
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0x03))
        << "setAggressive(false) 应只清位 2，保留位 0 (NO_AI) 和位 1 (LEFTHANDED)";
    EXPECT_FALSE(entity.isAggressive());
}

TEST(MobEntityAggressiveFlagTest, SetAggressive_True_Idempotent)
{
    // 连续 setAggressive(true) 不应累加位或改变其他位
    TestMobEntity entity;
    entity.writeRawMobFlags(static_cast<i8>(0x03));

    entity.setAggressive(true);
    const i8 afterFirst = entity.readRawMobFlags();
    entity.setAggressive(true);
    entity.setAggressive(true);

    EXPECT_EQ(entity.readRawMobFlags(), afterFirst) << "重复 setAggressive(true) 应幂等，不累加位";
    EXPECT_EQ(afterFirst, static_cast<i8>(0x07));
}

// ============================================================================
// getAggressiveFlagMask 静态访问器测试
// ============================================================================

TEST(MobEntityAggressiveFlagTest, GetAggressiveFlagMask_IsBit2)
{
    EXPECT_EQ(MobEntity::getAggressiveFlagMask(), static_cast<i8>(4)) << "MOB_FLAG_AGGRESSIVE 应为位 2 (0x04)";
    EXPECT_EQ(MobEntity::getAggressiveFlagMask(), static_cast<i8>(0x04));
}

TEST(MobEntityAggressiveFlagTest, GetMobFlagsParamId_IsConsistent)
{
    // 多个实例应共享同一个 DATA_MOB_FLAGS_PARAM ID（static 成员）
    TestMobEntity entity1;
    TestMobEntity entity2;
    EXPECT_EQ(MobEntity::getMobFlagsParamId(), MobEntity::getMobFlagsParamId())
        << "DATA_MOB_FLAGS_PARAM 是 static 成员，ID 应全局唯一且稳定";
}

// ============================================================================
// isAggroed / setAggroed 向后兼容委托测试
// ============================================================================

TEST(MobEntityAggressiveFlagTest, IsAggroed_DelegatesToIsAggressive)
{
    TestMobEntity entity;
    entity.setAggressive(true);
    EXPECT_TRUE(entity.isAggroed()) << "isAggroed() 应委托 isAggressive()，激怒时返回 true";

    entity.setAggressive(false);
    EXPECT_FALSE(entity.isAggroed()) << "isAggroed() 应委托 isAggressive()，非激怒时返回 false";
}

TEST(MobEntityAggressiveFlagTest, SetAggroed_DelegatesToSetAggressive)
{
    TestMobEntity entity;

    entity.setAggroed(true);
    EXPECT_TRUE(entity.isAggroed()) << "setAggroed(true) 应委托 setAggressive(true)";
    EXPECT_TRUE(entity.isAggressive()) << "setAggroed(true) 应同步写入 aggressive 状态";
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0x04)) << "setAggroed(true) 应置位 DATA_MOB_FLAGS_PARAM 位 2";

    entity.setAggroed(false);
    EXPECT_FALSE(entity.isAggroed()) << "setAggroed(false) 应委托 setAggressive(false)";
    EXPECT_FALSE(entity.isAggressive());
    EXPECT_EQ(entity.readRawMobFlags(), static_cast<i8>(0));
}

TEST(MobEntityAggressiveFlagTest, SetAggroed_AndSetAggressive_Interoperable)
{
    // 验证 setAggroed 和 setAggressive 操作的是同一份数据（通过 DATA_MOB_FLAGS_PARAM）
    TestMobEntity entity;

    entity.setAggressive(true);
    EXPECT_TRUE(entity.isAggroed()) << "setAggressive(true) 后 isAggroed() 应为 true（共享状态）";

    entity.setAggroed(false);
    EXPECT_FALSE(entity.isAggressive()) << "setAggroed(false) 后 isAggressive() 应为 false（共享状态）";
}
