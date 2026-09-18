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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace test {

// ==================== Vindicator Johnny 彩蛋测试 ====================
//
// 对齐 vanilla 1.21.11 Vindicator（Vindicator.java）：
//   - isJohnny 字段（Vindicator.java:54），默认 false。
//   - setCustomName override（Vindicator.java:145-150）：调基类后，若 !isJohnny 且
//     name.getString().equals("Johnny") 则 isJohnny = true（锁存，后续改名不取消）。
//   - registerGoals（Vindicator.java:73）：targetSelector.addGoal(4, new VindicatorJohnnyAttackGoal(this))。
//   - VindicatorJohnnyAttackGoal extends NearestAttackableTargetGoal<LivingEntity>
//     （Vindicator.java:209-224），canUse() = isJohnny && super.canUse()，谓词 attackable()。
//
// 修复前：Cubium VindicatorEntity 无 isJohnny 字段、无 Johnny goal、setCustomName 非虚，
//   命名为 "Johnny" 的卫道士不会攻击除玩家/村民/铁傀儡外的生物（Johnny 彩蛋完全失效）。
// 修复后：
//   - Entity::setCustomNameComponent 改 virtual，setCustomName(string) 委托之，统一虚入口。
//   - VindicatorEntity 加 m_isJohnny 字段 + setCustomNameComponent override（命名 "Johnny"
//     时锁存 isJohnny=true）。
//   - VindicatorJohnnyAttackGoal 继承 NearestAttackableTargetGoal<LivingEntity>，
//     shouldExecute 门控 isJohnny 后委托基类，registerGoals 优先级 4 注册。
//
// 本组验证锁存语义 + goal 触发链路。goal shouldExecute 经 findClosestEntity<LivingEntity>
// 查找候选，内部调 world->getEntitiesInRange(pos, range, except)，故测试世界 override
// getEntitiesInRange 注入预设目标（同 DefendVillageTargetGoalTest 夹具模式）。目标须经
// isSuitableTarget→canAttackType(*targetType)，targetType 由 entityType() 懒查询得到，
// 故目标须 setTypeId 对齐工厂路径（详见 entity/ai/goal/goals/target/README.md §13）。

namespace {

/// @brief Johnny 测试用世界：override getEntitiesInRange 返回预设候选目标列表。
///
/// VindicatorJohnnyAttackGoal::shouldExecute→NearestAttackableTargetGoal::shouldExecute
/// →findClosestEntity<LivingEntity> 经此方法取候选，搜索半径取 FOLLOW_RANGE（卫道士 12 格）。
/// except 为卫道士自身（findClosestEntity 传入），此处忽略以简化。
class VindicatorJohnnyTestWorld final : public mc::test::BaseTestWorld {
public:
    void setNearbyTargets(std::vector<mc::Entity*> targets) { m_targets = std::move(targets); }

    [[nodiscard]] std::vector<mc::Entity*> getEntitiesInRange(const mc::Vector3&, f32, const mc::Entity*) const override
    {
        return m_targets;
    }

private:
    std::vector<mc::Entity*> m_targets;
};

} // namespace

class VindicatorJohnnyTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 注册所有实体类型以使 entityType() 查表非 null——VindicatorJohnnyAttackGoal
        // 经基类 isSuitableTarget→canAttackType(*targetType) 依赖目标 entityType() 非 null，
        // 未注册时返 false，Johnny 锁敌链路无法触发。registerAll 进程级幂等。
        mc::entity::VanillaEntities::registerAll();
    }

    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
        mc::Items::initialize();

        // 卫道士：Johnny 彩蛋载体。setTypeId 对齐工厂路径（EntityType::create 会 setTypeId），
        // 直接构造的实体 m_typeId 默认空。设 targetSelector 每 tick 评估（绕 tickRate=2 节流），
        // 便于单测稳定触发。setWorld 使 goal shouldExecute 的 world() 非 null。
        m_vindicator = std::make_unique<mc::VindicatorEntity>(mc::EntityInstanceId(1), mc::test::testEcsRegistry());
        m_vindicator->setTypeId("minecraft:vindicator");
        m_vindicator->setWorld(&m_world);
        m_vindicator->setPosition(0.0, 64.0, 0.0);
        m_vindicator->targetSelector().setTickRate(1);
    }

    void TearDown() override { m_vindicator.reset(); }

    VindicatorJohnnyTestWorld m_world;
    std::unique_ptr<mc::VindicatorEntity> m_vindicator;
};

// ---------- isJohnny 锁存语义 ----------

TEST_F(VindicatorJohnnyTest, IsJohnnyFalseByDefault)
{
    // 对齐 vanilla Vindicator.isJohnny 默认 false（Vindicator.java:54）。
    EXPECT_FALSE(m_vindicator->isJohnny());
}

TEST_F(VindicatorJohnnyTest, SetCustomName_Johnny_ActivatesJohnny)
{
    // 对齐 vanilla Vindicator.setCustomName（Vindicator.java:145-150）：
    // 命名 "Johnny" 时 isJohnny 锁存为 true。setCustomName(string) 委托
    // setCustomNameComponent（virtual），文本路径同样触发 override。
    EXPECT_FALSE(m_vindicator->isJohnny());
    m_vindicator->setCustomName("Johnny");
    EXPECT_TRUE(m_vindicator->isJohnny());
    // 名称也应正确设置（基类行为不因 override 丢失）。
    EXPECT_EQ(m_vindicator->customNameText(), "Johnny");
}

TEST_F(VindicatorJohnnyTest, SetCustomName_OtherName_DoesNotActivateJohnny)
{
    // 非 "Johnny" 名称不激活。对齐 vanilla：仅精确匹配 "Johnny" 触发。
    m_vindicator->setCustomName("Vindicator");
    EXPECT_FALSE(m_vindicator->isJohnny());
    EXPECT_EQ(m_vindicator->customNameText(), "Vindicator");
}

TEST_F(VindicatorJohnnyTest, SetCustomName_CaseSensitive_DoesNotActivateJohnny)
{
    // 对齐 vanilla name.getString().equals("Johnny")——大小写敏感，"johnny" 不触发。
    m_vindicator->setCustomName("johnny");
    EXPECT_FALSE(m_vindicator->isJohnny());
}

TEST_F(VindicatorJohnnyTest, JohnnyIsLatched_RenameDoesNotDeactivate)
{
    // 对齐 vanilla 锁存语义：isJohnny 一旦激活，后续改名不再取消（vanilla setCustomName
    // 仅在 !isJohnny 时检查激活，无取消分支）。先激活再改为其他名，isJohnny 仍 true。
    m_vindicator->setCustomName("Johnny");
    ASSERT_TRUE(m_vindicator->isJohnny());

    m_vindicator->setCustomName("NotJohnnyAnymore");
    EXPECT_TRUE(m_vindicator->isJohnny()) << "Johnny should remain latched after rename";
    EXPECT_EQ(m_vindicator->customNameText(), "NotJohnnyAnymore");
}

TEST_F(VindicatorJohnnyTest, SetJohnnyDirectly_TogglesState)
{
    // setJohnny 是测试/存档恢复用直接访问器，验证其读写正确（不经过命名路径）。
    EXPECT_FALSE(m_vindicator->isJohnny());
    m_vindicator->setJohnny(true);
    EXPECT_TRUE(m_vindicator->isJohnny());
    m_vindicator->setJohnny(false);
    EXPECT_FALSE(m_vindicator->isJohnny());
}

// ---------- VindicatorJohnnyAttackGoal 触发链路 ----------
//
// goal 经 targetSelector.tick() 评估。Johnny 激活且附近有可攻击 LivingEntity 时，
// shouldExecute 返 true → startExecuting 调 setAttackTarget(目标)，attackTarget 非 null。
// 非 Johnny 时 shouldExecute 提前返 false（isJohnny 门控），不锁敌。

TEST_F(VindicatorJohnnyTest, GoalDoesNotLockTarget_WhenNotJohnny)
{
    // 非 Johnny 时 Johnny goal shouldExecute 门控返 false，即便附近有目标也不锁敌。
    // 注意：targetSelector 含优先级 2 的 NearestAttackableTargetGoal<Player> 等其他 goal，
    // 但测试目标用 ChickenEntity（非 Player/Villager/IronGolem），不被那些 goal 选中，
    // 故 attackTarget 应保持 null，仅 Johnny goal（优先级 4）会选鸡——而它被 isJohnny 门控关闭。
    mc::ChickenEntity chicken(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    chicken.setTypeId("minecraft:chicken");
    chicken.setWorld(&m_world);
    chicken.setPosition(2.0, 64.0, 0.0);

    m_world.setNearbyTargets({&chicken});
    EXPECT_FALSE(m_vindicator->isJohnny());
    m_vindicator->targetSelector().tick();
    EXPECT_EQ(m_vindicator->attackTarget(), nullptr);
}

TEST_F(VindicatorJohnnyTest, GoalLocksTarget_WhenJohnnyAndLivingEntityNearby)
{
    // 核心链路：命名 "Johnny" 激活 → 附近有鸡（LivingEntity，非玩家/村民/铁傀儡）→
    // Johnny goal shouldExecute 委托基类 findClosestEntity<LivingEntity> 选中鸡 →
    // startExecuting 调 setAttackTarget(鸡)。验证 Johnny 彩蛋攻击所有生物的核心行为。
    mc::ChickenEntity chicken(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    chicken.setTypeId("minecraft:chicken");
    chicken.setWorld(&m_world);
    chicken.setPosition(2.0, 64.0, 0.0);

    m_world.setNearbyTargets({&chicken});
    m_vindicator->setCustomName("Johnny");
    ASSERT_TRUE(m_vindicator->isJohnny());

    EXPECT_EQ(m_vindicator->attackTarget(), nullptr);
    m_vindicator->targetSelector().tick();
    EXPECT_EQ(m_vindicator->attackTarget(), &chicken);
}

TEST_F(VindicatorJohnnyTest, GoalDoesNotLockTarget_WhenJohnnyButNoLivingEntityNearby)
{
    // Johnny 激活但附近无可攻击目标时，基类 findClosestEntity 返空，shouldExecute 返 false，
    // 不锁敌。
    m_vindicator->setCustomName("Johnny");
    ASSERT_TRUE(m_vindicator->isJohnny());

    m_world.setNearbyTargets({});
    m_vindicator->targetSelector().tick();
    EXPECT_EQ(m_vindicator->attackTarget(), nullptr);
}

TEST_F(VindicatorJohnnyTest, GoalDoesNotLockDeadTarget_WhenJohnny)
{
    // 基类 isSuitableTarget 拒绝已死亡/移除目标（isAlive 基于 m_removed）。
    // Johnny 谓词 isAlive 亦过滤死亡目标。鸡已 discard 时不应锁敌。
    mc::ChickenEntity chicken(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    chicken.setTypeId("minecraft:chicken");
    chicken.setWorld(&m_world);
    chicken.setPosition(2.0, 64.0, 0.0);
    chicken.discard(); // 已移除（isAlive()==false）

    m_world.setNearbyTargets({&chicken});
    m_vindicator->setCustomName("Johnny");
    ASSERT_TRUE(m_vindicator->isJohnny());

    m_vindicator->targetSelector().tick();
    EXPECT_EQ(m_vindicator->attackTarget(), nullptr);
}

} // namespace test
} // namespace mc
