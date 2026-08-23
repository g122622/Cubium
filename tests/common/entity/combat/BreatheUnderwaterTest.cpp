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

// CAN_BREATHE_UNDER_WATER 标签查询修复测试（守卫者水下呼吸）。
//
// 验证 LivingEntity::canBreatheUnderwater()（LivingEntity.cpp）对齐 vanilla 1.21.11
// LivingEntity.canBreatheUnderwater():385：
//   return this.getType().is(EntityTypeTags.CAN_BREATHE_UNDER_WATER);
//
// 此前缺陷：Cubium canBreatheUnderwater() 基类仅查 getCreatureAttribute()==Undead。
// GuardianEntity 继承 MonsterEntity（非 WaterMobEntity），getCreatureAttribute=Water（非 Undead），
// 无 canBreatheUnderwater override → 返 false → updateAirSupply 在水中消耗空气 → 守卫者/远古守卫者
// 在水中溺水扣血，与 vanilla 相反（vanilla 守卫者永久水下生存，CAN_BREATHE_UNDER_WATER 标签含
// guardian/elder_guardian）。改查标签对齐 vanilla。
//
// 测试设计（4 例，含正反对照）：
//   - GuardianCanBreatheUnderwater：守卫者 canBreatheUnderwater()==true（标签查询修复）
//   - GuardianDoesNotDrownInWater：守卫者入水多次 updateAirSupply → 空气不降、血量不降（行为级验证）
//   - CreeperDrownsInWater：苦力怕（不在标签）入水 updateAirSupply → 空气下降（无回归：非水生实体仍溺水）
//   - ZombieCanBreatheUnderwater：僵尸（标签内亡灵）canBreatheUnderwater()==true（亡灵仍正确）
//
// 守卫者需 setTypeId("minecraft:guardian") 使 getTypeId() 返回标签成员字符串，标签 contains 命中。
// updateAirSupply 行为测试用 setInWater(true) 模拟入水（isInWater() 返 m_inWater），测试世界
// getBlockState 返 AIR（非气泡柱），故 inBubbleColumn=false 走正常空气消耗路径。
//
// Ref: vanilla LivingEntity.java:385（canBreatheUnderwater 查 CAN_BREATHE_UNDER_WATER 标签）
// Ref: vanilla EntityTypeTagsProvider.java（CAN_BREATHE_UNDER_WATER 含 guardian/elder_guardian）
// Ref: LivingEntity.cpp（canBreatheUnderwater 改查标签 + isInitialized 安全回退）
// Ref: LivingEntity.cpp:2603（updateAirSupply canBreatheUnderwater 门控空气消耗）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/world/IWorld.hpp"

using namespace mc;

namespace {

// BaseTestWorld protected 构造，派生公开。playSound 默认空实现吸收 updateAirSupply 溺水时的音效。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }
};

} // namespace

class BreatheUnderwaterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 实体类型标签初始化（进程级幂等）。CAN_BREATHE_UNDER_WATER 成员集在 initialize() 注册
        // （EntityTypeTags.cpp:592，含 guardian/elder_guardian/亡灵/水生生物）。未初始化时
        // canBreatheUnderwater() 走 isInitialized() 安全回退（仅亡灵），守卫者返 false。
        EntityTypeTags::initialize();
    }

    TestWorld m_world;
};

// 守卫者可在水下呼吸（CAN_BREATHE_UNDER_WATER 标签含 guardian，修复后查标签返 true）。
TEST_F(BreatheUnderwaterTest, GuardianCanBreatheUnderwater)
{
    GuardianEntity guardian(EntityInstanceId(1), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setTypeId("minecraft:guardian"); // 使 getTypeId() 命中标签成员

    // 修复前：getCreatureAttribute()==Water 非 Undead → false → 溺水
    // 修复后：标签含 guardian → true
    EXPECT_TRUE(guardian.canBreatheUnderwater());
}

// 守卫者入水后空气不消耗、血量不降（行为级验证：修复前会溺水扣血）。
//
// updateAirSupply 在 canBreatheUnderwater()==true 时不消耗空气（canBreathe=true 短路）。
// 多次调用后 air 应保持初始值（maxAir）、health 不变。修复前 canBreatheUnderwater=false →
// 空气逐 tick 下降，耗尽后 shouldTakeDrowningDamage 触发溺水扣血。
TEST_F(BreatheUnderwaterTest, GuardianDoesNotDrownInWater)
{
    GuardianEntity guardian(EntityInstanceId(1), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setTypeId("minecraft:guardian");
    guardian.setHealth(guardian.maxHealth());
    guardian.setInWater(true); // 模拟入水（isInWater() 返 m_inWater）

    const i32 airBefore = guardian.air();
    const f32 healthBefore = guardian.health();

    // 推进多次空气更新（远超 maxAir，足以让非水生实体空气耗尽溺水）
    for (int i = 0; i < 400; ++i) {
        guardian.updateAirSupply();
    }

    // 守卫者可水下呼吸 → 空气不消耗、不溺水扣血
    EXPECT_EQ(guardian.air(), airBefore);
    EXPECT_FLOAT_EQ(guardian.health(), healthBefore);
}

// 苦力怕（不在 CAN_BREATHE_UNDER_WATER 标签）入水后空气消耗（无回归：非水生实体仍溺水）。
//
// 对照组：证明修复未误伤普通怪物。苦力怕 canBreatheUnderwater()==false → updateAirSupply
// 在水中消耗空气，多次调用后 air 下降。
TEST_F(BreatheUnderwaterTest, CreeperDrownsInWater)
{
    CreeperEntity creeper(EntityInstanceId(1), mc::test::testEcsRegistry());
    creeper.setWorld(&m_world);
    creeper.setTypeId("minecraft:creeper");
    creeper.setHealth(creeper.maxHealth());
    creeper.setInWater(true);

    const i32 airBefore = creeper.air();
    ASSERT_GT(airBefore, 0);

    // 推进空气更新，苦力怕不可水下呼吸 → 空气下降
    creeper.updateAirSupply();
    EXPECT_LT(creeper.air(), airBefore);
}

// 僵尸（标签内亡灵）可在水下呼吸（亡灵仍正确，无回归）。
//
// 僵尸 getCreatureAttribute==Undead，标签 #undead 子标签成员含 zombie。修复前（Undead 判定）
// 与修复后（标签查询）均返 true。本测试保护亡灵水下呼吸不回归。
TEST_F(BreatheUnderwaterTest, ZombieCanBreatheUnderwater)
{
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setWorld(&m_world);
    zombie.setTypeId("minecraft:zombie");

    EXPECT_TRUE(zombie.canBreatheUnderwater());
}
