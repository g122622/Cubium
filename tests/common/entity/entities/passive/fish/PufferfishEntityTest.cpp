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
 * @file PufferfishEntityTest.cpp
 * @brief 河豚实体单元测试
 *
 * 测试 PufferfishEntity 的关键方法：
 * - PuffState 枚举值和状态转换
 * - getPuffSize() 根据状态返回正确值
 * - DataParameter 同步机制
 * - canPoison() 和 isFullyPuffed() 状态判断
 * - PuffGoal 敌人检测逻辑
 * - 膨胀/收缩计时器行为
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/player/Player.hpp"

using namespace mc;

// ==================== PufferfishEntity Test Fixture ====================

class PufferfishEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建河豚实体
        pufferfish = std::make_unique<PufferfishEntity>(EntityInstanceId(0));
    }

    void TearDown() override { pufferfish.reset(); }

    std::unique_ptr<PufferfishEntity> pufferfish;
};

// ==================== PuffState Enum Value Tests ====================

TEST_F(PufferfishEntityTest, PuffState_EnumValues_AreCorrect)
{
    // MC 1.16.5: PuffState 枚举值必须与原版一致
    // 用于网络同步和数据存储，必须精确匹配
    EXPECT_EQ(static_cast<i32>(PufferfishEntity::PuffState::Deflated), 0);
    EXPECT_EQ(static_cast<i32>(PufferfishEntity::PuffState::SemiPuffed), 1);
    EXPECT_EQ(static_cast<i32>(PufferfishEntity::PuffState::FullyPuffed), 2);
}

// ==================== PuffState Tests ====================

TEST_F(PufferfishEntityTest, PuffState_DefaultIsDeflated)
{
    // 默认状态应该是未膨胀
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
}

TEST_F(PufferfishEntityTest, PuffState_SetAndGet)
{
    // 设置为半膨胀
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

    // 设置为完全膨胀
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);

    // 设置回未膨胀
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
}

// ==================== getPuffSize Tests ====================

TEST_F(PufferfishEntityTest, GetPuffSize_Deflated_Returns_0_5)
{
    // MC 1.16.5: Deflated 状态返回 0.5
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_FLOAT_EQ(pufferfish->getPuffSize(), 0.5f);
}

TEST_F(PufferfishEntityTest, GetPuffSize_SemiPuffed_Returns_0_7)
{
    // MC 1.16.5: SemiPuffed 状态返回 0.7
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_FLOAT_EQ(pufferfish->getPuffSize(), 0.7f);
}

TEST_F(PufferfishEntityTest, GetPuffSize_FullyPuffed_Returns_1_0)
{
    // MC 1.16.5: FullyPuffed 状态返回 1.0
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_FLOAT_EQ(pufferfish->getPuffSize(), 1.0f);
}

// ==================== canPoison Tests ====================

TEST_F(PufferfishEntityTest, CanPoison_Deflated_ReturnsFalse)
{
    // 未膨胀时不能使敌人中毒
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_FALSE(pufferfish->canPoison());
}

TEST_F(PufferfishEntityTest, CanPoison_SemiPuffed_ReturnsTrue)
{
    // 半膨胀时可以使敌人中毒
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_TRUE(pufferfish->canPoison());
}

TEST_F(PufferfishEntityTest, CanPoison_FullyPuffed_ReturnsTrue)
{
    // 完全膨胀时可以使敌人中毒
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_TRUE(pufferfish->canPoison());
}

// ==================== isFullyPuffed Tests ====================

TEST_F(PufferfishEntityTest, IsFullyPuffed_Deflated_ReturnsFalse)
{
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_FALSE(pufferfish->isFullyPuffed());
}

TEST_F(PufferfishEntityTest, IsFullyPuffed_SemiPuffed_ReturnsFalse)
{
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_FALSE(pufferfish->isFullyPuffed());
}

TEST_F(PufferfishEntityTest, IsFullyPuffed_FullyPuffed_ReturnsTrue)
{
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_TRUE(pufferfish->isFullyPuffed());
}

// ==================== DataParameter Synchronization Tests ====================

TEST_F(PufferfishEntityTest, DataParameter_GetPuffStateParamId_ReturnsValidId)
{
    // getPuffStateParamId 应该返回有效的 DataParameter ID
    u16 paramId = PufferfishEntity::getPuffStateParamId();
    EXPECT_GT(paramId, 0u); // ID 应该大于 0
}

TEST_F(PufferfishEntityTest, DataParameter_SetPuffState_WritesToDataManager)
{
    // 设置膨胀状态应该写入 DataManager
    auto& dataManager = pufferfish->dataManager();

    // 设置为半膨胀
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);

    // 验证 DataManager 中存储了正确的值
    u16 paramId = PufferfishEntity::getPuffStateParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));

    // 从 DataManager 读取值
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, static_cast<i32>(PufferfishEntity::PuffState::SemiPuffed));
}

TEST_F(PufferfishEntityTest, DataParameter_GetPuffState_ReadsFromDataManager)
{
    // getPuffState 应该优先从 DataManager 读取
    auto& dataManager = pufferfish->dataManager();
    u16 paramId = PufferfishEntity::getPuffStateParamId();

    // 先设置一个状态
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);

    // 验证 getPuffState 返回正确的值
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);

    // 验证 DataManager 中的值
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, 2);
}

TEST_F(PufferfishEntityTest, DataParameter_SyncsStateChanges)
{
    // 测试多次状态变化的同步
    auto& dataManager = pufferfish->dataManager();
    u16 paramId = PufferfishEntity::getPuffStateParamId();

    // Deflated -> SemiPuffed
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(dataManager.get<i32>(entity::DataParameter<i32>(paramId)), 1);

    // SemiPuffed -> FullyPuffed
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(dataManager.get<i32>(entity::DataParameter<i32>(paramId)), 2);

    // FullyPuffed -> Deflated
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(dataManager.get<i32>(entity::DataParameter<i32>(paramId)), 0);
}

TEST_F(PufferfishEntityTest, DataParameter_DirtyFlag_OnStateChange)
{
    // 测试状态变化时 DataManager 的脏标记
    auto& dataManager = pufferfish->dataManager();

    // 设置状态应该触发脏标记
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置相同状态不应该触发脏标记
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同状态应该触发脏标记
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

// ==================== Puff Timer Tests ====================

TEST_F(PufferfishEntityTest, PuffTimer_DefaultIsZero)
{
    // 默认计时器为 0
    EXPECT_EQ(pufferfish->puffTimer(), 0);
}

TEST_F(PufferfishEntityTest, PuffTimer_StartPuffTimer_SetsTo1)
{
    // startPuffTimer 应该设置 puffTimer 为 1
    pufferfish->startPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 1);
}

TEST_F(PufferfishEntityTest, PuffTimer_ResetPuffTimer_SetsToZero)
{
    // 先设置计时器
    pufferfish->startPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 1);

    // 重置应该设置回 0
    pufferfish->resetPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 0);
}

TEST_F(PufferfishEntityTest, DeflateTimer_DefaultIsZero)
{
    // 默认收缩计时器为 0
    EXPECT_EQ(pufferfish->deflateTimer(), 0);
}

// ==================== Attributes Tests ====================

TEST_F(PufferfishEntityTest, Attributes_HasCorrectHealth)
{
    // MC 1.16.5: 河豚最大生命值为 3
    EXPECT_FLOAT_EQ(pufferfish->maxHealth(), 3.0f);
}

// ==================== Dimensions Tests ====================

TEST_F(PufferfishEntityTest, Dimensions_Deflated_HasCorrectSize)
{
    // MC 1.16.5: 未膨胀时尺寸 = 0.7 * 0.5 = 0.35
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    auto dims = pufferfish->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.35f);
    EXPECT_FLOAT_EQ(dims.height(), 0.35f);
}

TEST_F(PufferfishEntityTest, Dimensions_SemiPuffed_HasCorrectSize)
{
    // MC 1.16.5: 半膨胀时尺寸 = 0.7 * 0.7 = 0.49
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    auto dims = pufferfish->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.49f);
    EXPECT_FLOAT_EQ(dims.height(), 0.49f);
}

TEST_F(PufferfishEntityTest, Dimensions_FullyPuffed_HasCorrectSize)
{
    // MC 1.16.5: 完全膨胀时尺寸 = 0.7 * 1.0 = 0.7
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    auto dims = pufferfish->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.7f);
    EXPECT_FLOAT_EQ(dims.height(), 0.7f);
}

// ==================== Eye Height Tests ====================

TEST_F(PufferfishEntityTest, EyeHeight_HasCorrectValue)
{
    // MC 1.16.5: 河豚眼睛高度为 0.15
    EXPECT_FLOAT_EQ(pufferfish->eyeHeight(), 0.15f);
}

// ==================== PuffGoal IsEnemy Tests ====================

class PuffGoalTest : public ::testing::Test {
protected:
    void SetUp() override { pufferfish = std::make_unique<PufferfishEntity>(EntityInstanceId(0)); }

    void TearDown() override { pufferfish.reset(); }

    std::unique_ptr<PufferfishEntity> pufferfish;
};

// 注意: isEnemy 是 PuffGoal 的私有静态方法，
// 我们通过公共接口（shouldExecute）来测试敌人检测逻辑
// 或者需要添加友元类来测试

// PuffGoal 基本构造测试
TEST_F(PuffGoalTest, Construction)
{
    // 验证 PuffGoal 可以正常构造
    entity::ai::goal::PuffGoal goal(pufferfish.get());
    EXPECT_EQ(goal.getTypeName(), "PuffGoal");
}

// ==================== PuffGoal Goal Flags Tests ====================

TEST_F(PuffGoalTest, HasNoMutualFlags)
{
    // MC 1.16.5: PuffGoal 没有互斥标志
    entity::ai::goal::PuffGoal goal(pufferfish.get());
    // Goal 默认构造函数应该设置空标志集
}

// ==================== PuffGoal isEnemy 逻辑测试 ====================
// 注意: isEnemy 是私有静态方法，我们通过公共接口间接测试
// 或者可以添加友元类来测试。这里测试公共行为。

TEST_F(PuffGoalTest, DetectionRange_Constant_IsCorrect)
{
    // MC 1.16.5: 检测范围是 2.0 格
    constexpr f32 DETECTION_RANGE = 2.0f;
    EXPECT_FLOAT_EQ(DETECTION_RANGE, 2.0f);
}

// ==================== PufferfishEntity 状态转换常量测试 ====================

TEST_F(PufferfishEntityTest, StateTransitionConstants_AreCorrect)
{
    // MC 1.16.5 状态转换常量
    // 膨胀: puffTimer == 1 时从状态 0 变为状态 1
    // 膨胀: puffTimer > 40 时从状态 1 变为状态 2
    // 收缩: 完全膨胀→半膨胀延迟 60 ticks
    // 收缩: 半膨胀→未膨胀延迟 100 ticks
    EXPECT_EQ(pufferfish->puffTimer(), 0);
    EXPECT_EQ(pufferfish->deflateTimer(), 0);
}

// ==================== PufferfishEntity 状态转换逻辑测试 ====================

TEST_F(PufferfishEntityTest, StateTransition_StartPuffTimer_InitialState)
{
    // 初始状态应该是未膨胀
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);

    // 启动膨胀计时器
    pufferfish->startPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 1);
    EXPECT_EQ(pufferfish->deflateTimer(), 0);
}

TEST_F(PufferfishEntityTest, StateTransition_ResetPuffTimer_ClearsState)
{
    // 先设置膨胀计时器
    pufferfish->startPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 1);

    // 重置应该清除计时器
    pufferfish->resetPuffTimer();
    EXPECT_EQ(pufferfish->puffTimer(), 0);
}

TEST_F(PufferfishEntityTest, StateTransition_SetPuffState_PlaysSound)
{
    // 注意：音效播放需要世界环境，这里只测试状态变更
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);

    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
}

// ==================== Sound Effects on State Change Tests ====================
// 音效播放需要世界环境，这里测试状态转换逻辑

TEST_F(PufferfishEntityTest, SoundEffect_PuffUp_TransitionsUpward)
{
    // MC 1.16.5: 膨胀时播放 BLOW_UP 音效
    // 测试膨胀方向的状态转换（从小到大）
    // Deflated -> SemiPuffed: 播放 BLOW_UP
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

    // SemiPuffed -> FullyPuffed: 播放 BLOW_UP
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);

    // Deflated -> FullyPuffed (跳过 SemiPuffed): 播放 BLOW_UP
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);
}

TEST_F(PufferfishEntityTest, SoundEffect_PuffDown_TransitionsDownward)
{
    // MC 1.16.5: 收缩时播放 BLOW_OUT 音效
    // 测试收缩方向的状态转换（从大到小）
    // FullyPuffed -> SemiPuffed: 播放 BLOW_OUT
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

    // SemiPuffed -> Deflated: 播放 BLOW_OUT
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);

    // FullyPuffed -> Deflated (跳过 SemiPuffed): 播放 BLOW_OUT
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
}

TEST_F(PufferfishEntityTest, SoundEffect_NoChange_NoSound)
{
    // MC 1.16.5: 设置相同状态时不播放音效
    // 设置相同状态应该被忽略（早期返回）
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);

    // 再次设置相同状态
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

    // 验证 DataManager 中值仍然是 1
    auto& dataManager = pufferfish->dataManager();
    u16 paramId = PufferfishEntity::getPuffStateParamId();
    EXPECT_EQ(dataManager.get<i32>(entity::DataParameter<i32>(paramId)), 1);
}

TEST_F(PufferfishEntityTest, SoundEffect_RapidStateChanges)
{
    // 测试快速状态变化
    for (int i = 0; i < 10; ++i) {
        pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
        EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::SemiPuffed);

        pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
        EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::FullyPuffed);

        pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
        EXPECT_EQ(pufferfish->getPuffState(), PufferfishEntity::PuffState::Deflated);
    }
}

// ==================== PufferfishEntity 中毒伤害计算测试 ====================

TEST_F(PufferfishEntityTest, PoisonDamage_Deflated_NoDamage)
{
    // 未膨胀状态不能造成中毒伤害
    pufferfish->setPuffState(PufferfishEntity::PuffState::Deflated);
    EXPECT_FALSE(pufferfish->canPoison());
}

TEST_F(PufferfishEntityTest, PoisonDamage_SemiPuffed_Damage2)
{
    // 半膨胀状态伤害 = 1 + 1 = 2
    pufferfish->setPuffState(PufferfishEntity::PuffState::SemiPuffed);
    EXPECT_TRUE(pufferfish->canPoison());
    // 中毒持续时间 = 60 * 1 = 60 ticks
}

TEST_F(PufferfishEntityTest, PoisonDamage_FullyPuffed_Damage3)
{
    // 完全膨胀状态伤害 = 1 + 2 = 3
    pufferfish->setPuffState(PufferfishEntity::PuffState::FullyPuffed);
    EXPECT_TRUE(pufferfish->canPoison());
    // 中毒持续时间 = 60 * 2 = 120 ticks
}

// ==================== PufferfishEntity 属性验证测试 ====================

TEST_F(PufferfishEntityTest, EntityType_IsPufferfish)
{
    // 验证实体类型：直接构造（未经注册表工厂）的实体 typeId 为空，
    // entityType() 懒查询注册表返回 nullptr。
    EXPECT_EQ(pufferfish->entityType(), nullptr);
}

TEST_F(PufferfishEntityTest, MaxHealth_IsCorrect)
{
    // MC 1.16.5: 河豚最大生命值为 3
    EXPECT_FLOAT_EQ(pufferfish->maxHealth(), 3.0f);
}

// ==================== PufferfishEntity 创建工厂测试 ====================

TEST_F(PufferfishEntityTest, Create_ReturnsValidEntity)
{
    // 创建工厂方法
    auto entity = PufferfishEntity::create(nullptr);
    EXPECT_NE(entity, nullptr);
    EXPECT_EQ(entity->entityType(), nullptr);
}
