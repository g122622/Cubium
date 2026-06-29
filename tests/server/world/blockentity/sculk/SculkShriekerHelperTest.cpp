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
 * @file SculkShriekerHelperTest.cpp
 * @brief SculkShriekerHelper 和 SculkShriekerBlockEntity 尖啸体逻辑测试
 *
 * 测试范围：
 * - SculkShriekerBlockEntity: shriekingFinished 标志的设置/清除/序列化
 * - SculkShriekerBlockEntity: warningLevel 与 canSummonWarden 的边界条件
 * - WardenWarningEffect: 警告等级递增/递减/冷却逻辑
 * - SculkShriekerHelper: tryGetPlayer 实体解析逻辑
 * - SculkShriekerHelper: 常量值验证
 *
 * 注意：SculkShriekerHelper 的 tryShriek/tryRespond/trySummonWarden 等方法
 * 依赖 ServerWorld，需要集成测试覆盖。此处仅测试不依赖 ServerWorld 的纯逻辑。
 */

#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "server/world/blockentity/sculk/SculkShriekerHelper.hpp"

using namespace mc;
using namespace mc::blockentity;
using namespace mc::server;

// ============================================================================
// SculkShriekerBlockEntity shriekingFinished 测试
// ============================================================================

class SculkShriekerBlockEntityShriekTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(10, 64, -20); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedDefaultFalse)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, SetShriekingFinished)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());

    entity->setShriekingFinished(false);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedIndependentOfWarningLevel)
{
    // shriekingFinished 和 warningLevel 应该独立
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(3);
    EXPECT_FALSE(entity->isShriekingFinished());

    entity->setShriekingFinished(true);
    EXPECT_EQ(entity->getWarningLevel(), 3);
    EXPECT_TRUE(entity->isShriekingFinished());

    entity->setShriekingFinished(false);
    EXPECT_EQ(entity->getWarningLevel(), 3);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedWithMaxWarningLevel)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 递增到最大警告等级
    for (i32 i = 0; i < 4; ++i) {
        entity->incrementWarningLevel();
    }
    EXPECT_TRUE(entity->canSummonWarden());

    // 设置 shriekingFinished（模拟尖啸结束）
    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityShriekTest, WarningLevelResetAfterTryShriek)
{
    // tryShriek 每次调用都重置 warningLevel 为 0
    // 此测试验证 BlockEntity 的 setWarningLevel(0) 行为
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->incrementWarningLevel();
    entity->incrementWarningLevel();
    entity->incrementWarningLevel();
    EXPECT_EQ(entity->getWarningLevel(), 3);

    // 模拟 tryShriek 的重置操作
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityShriekTest, JsonSerialization_WithShriekingFinished)
{
    // 注意：shriekingFinished 不应序列化（运行时标志）
    // 但 warningLevel 和 vibrationData 需要序列化
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->incrementWarningLevel();
    entity->setShriekingFinished(true);

    nlohmann::json data;
    entity->save(data);

    // warning_level 应该被保存
    EXPECT_TRUE(data.contains("warning_level"));
    EXPECT_EQ(data["warning_level"], 1);

    // shriekingFinished 是运行时标志，不应保存到存档
    EXPECT_FALSE(data.contains("shrieking_finished"));
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedNotPersistedAcrossLoad)
{
    // shriekingFinished 不应从存档中恢复（每次加载默认为 false）
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);
    entity->incrementWarningLevel();
    entity->setShriekingFinished(true);

    nlohmann::json data;
    entity->save(data);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getWarningLevel(), 1);
    EXPECT_FALSE(loaded->isShriekingFinished());
}

// ============================================================================
// SculkShriekerBlockEntity canSummonWarden 边界条件测试
// ============================================================================

class SculkShriekerBlockEntityWarningTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(0, 0, 0); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityWarningTest, CanSummonWardenAtLevel4Only)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    EXPECT_FALSE(entity->canSummonWarden()); // level 0

    entity->incrementWarningLevel();
    EXPECT_FALSE(entity->canSummonWarden()); // level 1

    entity->incrementWarningLevel();
    EXPECT_FALSE(entity->canSummonWarden()); // level 2

    entity->incrementWarningLevel();
    EXPECT_FALSE(entity->canSummonWarden()); // level 3

    entity->incrementWarningLevel();
    EXPECT_TRUE(entity->canSummonWarden()); // level 4
}

TEST_F(SculkShriekerBlockEntityWarningTest, WarningLevelDoesNotExceedMax)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 递增超过最大值
    for (i32 i = 0; i < 10; ++i) {
        entity->incrementWarningLevel();
    }
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityWarningTest, SetWarningLevelDirectly)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 直接设置到各等级
    entity->setWarningLevel(0);
    EXPECT_FALSE(entity->canSummonWarden());

    entity->setWarningLevel(4);
    EXPECT_TRUE(entity->canSummonWarden());

    // 重置回 0（tryShriek 的行为）
    entity->setWarningLevel(0);
    EXPECT_FALSE(entity->canSummonWarden());
}

// ============================================================================
// WardenWarningEffect 测试
// ============================================================================

class WardenWarningEffectTest : public ::testing::Test {
protected:
    void SetUp() override {}

    entity::WardenWarningEffect effect;
};

TEST_F(WardenWarningEffectTest, DefaultState)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);
    EXPECT_EQ(effect.getSourcePos(), BlockPos());
}

TEST_F(WardenWarningEffectTest, IncreaseWarning)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);

    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);

    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 3);

    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 4);
}

TEST_F(WardenWarningEffectTest, IncreaseWarningCappedAt4)
{
    for (i32 i = 0; i < 8; ++i) {
        effect.increaseWarning();
    }
    EXPECT_EQ(effect.getWarningLevel(), 4);
}

TEST_F(WardenWarningEffectTest, DecreaseWarning)
{
    effect.increaseWarning();
    effect.increaseWarning();
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 3);

    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);

    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);
}

TEST_F(WardenWarningEffectTest, DecreaseWarningFloorAt0)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);
    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenWarningEffectTest, SetSourcePos)
{
    BlockPos shriekerPos(100, -50, 200);
    effect.setSourcePos(shriekerPos);
    EXPECT_EQ(effect.getSourcePos(), shriekerPos);
}

TEST_F(WardenWarningEffectTest, TickDecreasesAfterCooldown)
{
    // 递增警告等级（这会设置冷却）
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 冷却期间 tick 不应递减
    // WardenWarningEffect 内部 DECREASE_INTERVAL = 200
    // 我们只验证前几个 tick 不会立即递减
    for (i32 i = 0; i < 100; ++i) {
        effect.tick();
    }
    EXPECT_EQ(effect.getWarningLevel(), 1);
}

TEST_F(WardenWarningEffectTest, TickEventuallyDecreasesTo0)
{
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 模拟足够多的 tick 让警告等级递减到 0
    // DECREASE_INTERVAL = 200 tick, 加上 1 个冷却周期 + 递减周期
    for (i32 i = 0; i < 500; ++i) {
        effect.tick();
    }
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenWarningEffectTest, WarningRadius)
{
    // 默认警告半径应为 10.0
    EXPECT_FLOAT_EQ(effect.getWarningRadius(), 10.0f);
}

// ============================================================================
// SculkShriekerHelper 常量验证测试
// ============================================================================

class SculkShriekerHelperConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SculkShriekerHelperConstantsTest, WardenSearchRadius)
{
    // 搜索半径 48 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::WARDEN_SEARCH_RADIUS, 48.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, PlayerSearchRadius)
{
    // 搜索半径 16 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::PLAYER_SEARCH_RADIUS, 16.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessRadius)
{
    // 黑暗效果半径 40 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::DARKNESS_RADIUS, 40.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessDuration)
{
    // 260 tick = 13 秒
    EXPECT_EQ(SculkShriekerHelper::DARKNESS_DURATION, 260);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessCooldown)
{
    // 黑暗效果应用冷却 200 tick
    EXPECT_EQ(SculkShriekerHelper::DARKNESS_COOLDOWN, 200);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonAttempts)
{
    // 20 次尝试
    EXPECT_EQ(SculkShriekerHelper::SUMMON_ATTEMPTS, 20);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonHorizontalRange)
{
    // 水平偏移 +/-5
    EXPECT_EQ(SculkShriekerHelper::SUMMON_HORIZONTAL_RANGE, 5);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonVerticalRange)
{
    // 垂直偏移 +/-6
    EXPECT_EQ(SculkShriekerHelper::SUMMON_VERTICAL_RANGE, 6);
}

TEST_F(SculkShriekerHelperConstantsTest, WardenSoundByLevel)
{
    // 验证声音映射：level 0 无声音，level 1-4 对应不同声音
    EXPECT_EQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[0], nullptr);
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[1], "minecraft:entity.warden.nearby_close");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[2], "minecraft:entity.warden.nearby_closer");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[3], "minecraft:entity.warden.nearby_closest");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[4], "minecraft:entity.warden.listening_angry");
}

// ============================================================================
// SculkShriekerHelper::tryGetPlayer 测试
// ============================================================================

class SculkShriekerHelperTryGetPlayerTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SculkShriekerHelperTryGetPlayerTest, NullptrEntityReturnsNullptr)
{
    // 传入 nullptr 实体应返回 nullptr
    Player* result = SculkShriekerHelper::tryGetPlayer(nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, DirectPlayerReturnsPlayer)
{
    // 直接传入玩家实体应返回该玩家
    auto player = std::make_unique<Player>(EntityId(1), "TestPlayer");
    Player* result = SculkShriekerHelper::tryGetPlayer(player.get());
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, player.get());
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, NonPlayerEntityReturnsNullptr)
{
    // 传入非玩家实体应返回 nullptr
    // 当前 tryGetPlayer 仅支持直接玩家解析
    // 投射物、载具、掉落物的主人解析为 TODO
    // 使用 LivingEntity 作为非玩家实体
    auto entity = std::make_unique<LivingEntity>(EntityId(2));
    Player* result = SculkShriekerHelper::tryGetPlayer(entity.get());
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ConstPlayerReturnsMutablePlayer)
{
    // 传入 const Player* 应返回可变 Player*
    auto player = std::make_unique<Player>(EntityId(3), "ConstPlayer");
    const Player* constPlayer = player.get();
    Player* result = SculkShriekerHelper::tryGetPlayer(constPlayer);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, player.get());
}

// ============================================================================
// SculkShriekerHelper canRespond 条件逻辑说明
// ============================================================================
//
// _canRespond 的逻辑依赖 ServerWorld，无法在纯单元测试中覆盖。
// 其条件为：
//   1. CAN_SUMMON 方块状态属性为 true（自然生成的尖啸体）
//   2. 非和平难度
//   3. 游戏规则 DO_WARDEN_SPAWNING 为 true
// 三个条件全部满足时才返回 true。
// 这些条件需要集成测试覆盖。

// ============================================================================
// SculkShriekerBlockEntity 综合场景测试
// ============================================================================

class SculkShriekerBlockEntityScenarioTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(100, -60, 200); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityScenarioTest, FullShriekCycle)
{
    // 模拟完整的尖啸周期：重置→递增→递增→递增→递增→可召唤→重置
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 模拟 tryShriek: 重置 warningLevel
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);

    // 模拟 4 次 tryWarn 递增
    entity->incrementWarningLevel(); // level 1
    EXPECT_EQ(entity->getWarningLevel(), 1);
    EXPECT_FALSE(entity->canSummonWarden());

    entity->incrementWarningLevel(); // level 2
    entity->incrementWarningLevel(); // level 3
    entity->incrementWarningLevel(); // level 4
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());

    // 模拟尖啸结束
    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());

    // 模拟 tryRespond 处理后清除标志
    entity->setShriekingFinished(false);
    entity->setChanged();
    EXPECT_FALSE(entity->isShriekingFinished());

    // 模拟下一次 tryShriek 重置
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityScenarioTest, MultipleShrieksBeforeRespond)
{
    // 模拟多次尖啸但警告等级未达到阈值
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 第一次 tryShriek
    entity->setWarningLevel(0);
    entity->incrementWarningLevel(); // level 1
    EXPECT_FALSE(entity->canSummonWarden());

    // 模拟尖啸结束并响应（不会召唤监守者，但会播放声音）
    entity->setShriekingFinished(true);
    entity->setShriekingFinished(false);

    // 第二次 tryShriek（重置再递增）
    entity->setWarningLevel(0);
    entity->incrementWarningLevel(); // level 1
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityScenarioTest, WarningLevelPersistence)
{
    // 验证警告等级在序列化/反序列化后保留
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->incrementWarningLevel();
    entity->incrementWarningLevel();
    entity->incrementWarningLevel();

    nlohmann::json data;
    entity->save(data);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getWarningLevel(), 3);

    // 继续递增到 4
    loaded->incrementWarningLevel();
    EXPECT_TRUE(loaded->canSummonWarden());
}

// ============================================================================
// SculkShriekerVibrationUser isSculkShrieker 区分测试
// ============================================================================

#include "server/world/blockentity/sculk/SculkVibrationSystem.hpp"

using namespace mc::blockentity;
using namespace mc::gameevent;

class SculkVibrationUserTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        sensorPos_ = BlockPos(0, 64, 0);
        shriekerPos_ = BlockPos(10, 64, 10);
    }

    BlockPos sensorPos_;
    BlockPos shriekerPos_;
};

TEST_F(SculkVibrationUserTypeTest, SensorUser_IsNotSculkShrieker)
{
    SculkSensorBlockEntity sensorEntity(sensorPos_);
    SculkSensorVibrationUser sensorUser(sensorEntity);

    // SculkSensorVibrationUser 不是 SculkShrieker
    EXPECT_FALSE(sensorUser.isSculkShrieker());

    // SculkSensorVibrationUser 可以触发规避振动成就
    EXPECT_TRUE(sensorUser.canTriggerAvoidVibration());
}

TEST_F(SculkVibrationUserTypeTest, ShriekerUser_IsSculkShrieker)
{
    SculkShriekerBlockEntity shriekerEntity(shriekerPos_);
    SculkShriekerVibrationUser shriekerUser(shriekerEntity);

    // SculkShriekerVibrationUser 是 SculkShrieker
    EXPECT_TRUE(shriekerUser.isSculkShrieker());

    // SculkShriekerVibrationUser 不可以触发规避振动成就
    EXPECT_FALSE(shriekerUser.canTriggerAvoidVibration());
}

TEST_F(SculkVibrationUserTypeTest, ShriekerUser_OnlyReceivesShriekEvent)
{
    // SculkShriekerVibrationUser 的 canReceiveVibration 只接受 SHRIEK 事件
    // 这在 SculkVibrationSystem.cpp 中实现，此处验证基础属性
    SculkShriekerBlockEntity shriekerEntity(shriekerPos_);
    SculkShriekerVibrationUser shriekerUser(shriekerEntity);

    // 检测半径应为 8
    EXPECT_EQ(shriekerUser.getListenerRadius(), 8);

    // 需要相邻区块 tick
    EXPECT_TRUE(shriekerUser.requiresAdjacentChunksToBeTicking());
}

TEST_F(SculkVibrationUserTypeTest, SensorUser_ListenerRadius)
{
    SculkSensorBlockEntity sensorEntity(sensorPos_);
    SculkSensorVibrationUser sensorUser(sensorEntity);

    // 检测半径应为 8
    EXPECT_EQ(sensorUser.getListenerRadius(), 8);

    // 需要相邻区块 tick
    EXPECT_TRUE(sensorUser.requiresAdjacentChunksToBeTicking());
}

// ============================================================================
// SculkVibrationSystem 类型区分测试
// ============================================================================

TEST(SculkVibrationSystemTypeTest, SensorSystem_UserIsNotShrieker)
{
    SculkSensorBlockEntity sensorEntity(BlockPos(0, 0, 0));
    SculkVibrationSystem system(sensorEntity);

    auto& user = system.getVibrationUser();
    EXPECT_FALSE(user.isSculkShrieker());
}

TEST(SculkVibrationSystemTypeTest, ShriekerSystem_UserIsShrieker)
{
    SculkShriekerBlockEntity shriekerEntity(BlockPos(0, 0, 0));
    SculkVibrationSystem system(shriekerEntity);

    auto& user = system.getVibrationUser();
    EXPECT_TRUE(user.isSculkShrieker());
}
