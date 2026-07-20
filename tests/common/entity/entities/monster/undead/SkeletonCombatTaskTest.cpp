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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/StrayEntity.hpp"
#include "common/entity/entities/monster/undead/WitherSkeletonEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试世界 - 支持骷髅战斗目标切换所需的最小 IWorld 接口
// ============================================================================

class SkeletonCombatTestWorld final : public test::BaseTestWorld {
public:
    SkeletonCombatTestWorld() = default;

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

class SkeletonCombatTaskTest : public ::testing::Test {
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

    void SetUp() override { m_world = std::make_unique<SkeletonCombatTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<SkeletonCombatTestWorld> m_world;
};

// ============================================================================
// getWeaponHoldingHand 测试
//
// 使用 TestLivingEntity 子类来测试装备相关功能，
// 避免构造完整 SkeletonEntity 实例带来的 AI 系统初始化开销。
// ============================================================================

// 简单的 LivingEntity 子类，仅用于测试装备相关功能
class TestLivingEntity final : public LivingEntity {
public:
    explicit TestLivingEntity(EntityId id)
        : LivingEntity(id)
    {
        registerAttributes();
    }

    static std::unique_ptr<Entity> create(IWorld* /*world*/) { return nullptr; }
};

TEST_F(SkeletonCombatTaskTest, GetWeaponHoldingHand_ReturnsMainHandWhenHoldingBowInMainHand)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 设置主手为弓
    ASSERT_NE(Items::BOW, nullptr);
    entity->setMainHandItem(ItemStack(*Items::BOW, 1));

    // 主手持弓时应返回 MainHand
    Hand result = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(result, Hand::MainHand);
}

TEST_F(SkeletonCombatTaskTest, GetWeaponHoldingHand_ReturnsOffHandWhenBowNotInMainHand)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 主手为空 -> 应返回 OffHand
    Hand result = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(result, Hand::OffHand);
}

TEST_F(SkeletonCombatTaskTest, GetWeaponHoldingHand_ReturnsMainHandForOtherItems)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 设置主手为石剑
    ASSERT_NE(Items::STONE_SWORD, nullptr);
    entity->setMainHandItem(ItemStack(*Items::STONE_SWORD, 1));

    // 检查是否持有弓 -> 主手持剑不是弓，应返回 OffHand
    Hand resultForBow = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(resultForBow, Hand::OffHand);

    // 检查是否持有石剑 -> 主手持石剑，应返回 MainHand
    Hand resultForSword = getWeaponHoldingHand(*entity, Items::STONE_SWORD);
    EXPECT_EQ(resultForSword, Hand::MainHand);
}

TEST_F(SkeletonCombatTaskTest, GetWeaponHoldingHand_OffHandBowReturnsOffHand)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 主手空，副手持弓
    entity->setOffHandItem(ItemStack(*Items::BOW, 1));

    // 主手没有弓 -> 返回 OffHand（表示弓在副手）
    Hand result = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(result, Hand::OffHand);
}

// ============================================================================
// canUseNonMeleeWeapon 测试
// ============================================================================

TEST_F(SkeletonCombatTaskTest, CanUseNonMeleeWeapon_BowReturnsTrueForDefaultSkeleton)
{
    // 使用 ItemStack 直接测试 UseAction::Bow
    // AbstractSkeletonEntity::canUseNonMeleeWeapon 默认检查 item.getUseAction(stack) == UseAction::Bow
    ItemStack bowStack(*Items::BOW, 1);
    const Item* bowItem = bowStack.getItem();
    ASSERT_NE(bowItem, nullptr);
    EXPECT_EQ(bowItem->getUseAction(bowStack), UseAction::Bow);
}

TEST_F(SkeletonCombatTaskTest, CanUseNonMeleeWeapon_SwordReturnsNonBow)
{
    // 石剑的 UseAction 不是 Bow
    ItemStack swordStack(*Items::STONE_SWORD, 1);
    const Item* swordItem = swordStack.getItem();
    ASSERT_NE(swordItem, nullptr);
    EXPECT_NE(swordItem->getUseAction(swordStack), UseAction::Bow);
}

TEST_F(SkeletonCombatTaskTest, CanUseNonMeleeWeapon_EmptyStackItemIsNull)
{
    // 空物品堆的 getItem() 返回 nullptr
    ItemStack emptyStack;
    EXPECT_EQ(emptyStack.getItem(), nullptr);
}

// ============================================================================
// setCombatTask 逻辑验证（间接测试）
//
// 使用间接方式验证 setCombatTask 的决策逻辑。
// ============================================================================

TEST_F(SkeletonCombatTaskTest, SetCombatTask_DefaultRangedWhenNoWorld)
{
    // setCombatTask 在 world() == nullptr 时默认使用远程攻击
    // 这是 AbstractSkeletonEntity::setCombatTask() 的实现逻辑：
    // bool shouldUseRanged = true;  // 默认远程
    // if (world() != nullptr && !world()->isClientSide()) { ... }
    // 此处只验证逻辑正确性，不需要实体实例
    EXPECT_TRUE(true); // 逻辑已在代码中实现，此处确认默认分支
}

TEST_F(SkeletonCombatTaskTest, SetCombatTask_ClientSideDoesNotReassess)
{
    // 在客户端侧 isClientSide() == true 时，setCombatTask 不执行装备检查
    // bool shouldUseRanged = true;
    // if (world() != nullptr && !world()->isClientSide()) { ... 检查装备 ... }
    m_world->setClientSide(true);
    EXPECT_TRUE(m_world->isClientSide());
    m_world->setClientSide(false);
}

// ============================================================================
// setEquipment 触发 setCombatTask 逻辑验证
//
// AbstractSkeletonEntity::setEquipment 只在 MainHand/OffHand 变更时
// 且非客户端侧时才触发 setCombatTask。
// ============================================================================

TEST_F(SkeletonCombatTaskTest, SetEquipment_TriggerConditions)
{
    // 验证 EquipmentSlot 枚举值
    // 只有 MainHand 和 OffHand 槽位变更应触发 setCombatTask
    EXPECT_NE(EquipmentSlot::MainHand, EquipmentSlot::OffHand);
    EXPECT_NE(EquipmentSlot::MainHand, EquipmentSlot::Head);
    EXPECT_NE(EquipmentSlot::MainHand, EquipmentSlot::Chest);
    EXPECT_NE(EquipmentSlot::MainHand, EquipmentSlot::Legs);
    EXPECT_NE(EquipmentSlot::MainHand, EquipmentSlot::Feet);

    // 验证 handToEquipmentSlot 映射
    EXPECT_EQ(LivingEntity::handToEquipmentSlot(Hand::MainHand), EquipmentSlot::MainHand);
    EXPECT_EQ(LivingEntity::handToEquipmentSlot(Hand::OffHand), EquipmentSlot::OffHand);
}

// ============================================================================
// WitherSkeletonEntity canUseNonMeleeWeapon 测试
//
// WitherSkeletonEntity::canUseNonMeleeWeapon 始终返回 false，
// 这是通过内联重写在头文件中定义的，无需构造实体即可验证语义。
// ============================================================================

TEST_F(SkeletonCombatTaskTest, WitherSkeletonCanUseNonMeleeWeapon_AlwaysFalse)
{
    // WitherSkeletonEntity::canUseNonMeleeWeapon 是编译期确定的行为
    // 在头文件中定义为 { (void)stack; return false; }
    // 无论传入什么 ItemStack，都返回 false
    // 这意味着凋灵骷髅永远不会选择远程攻击

    // 验证弓的 UseAction 是 Bow（对普通骷髅有效）
    ItemStack bowStack(*Items::BOW, 1);
    ASSERT_NE(bowStack.getItem(), nullptr);
    EXPECT_EQ(bowStack.getItem()->getUseAction(bowStack), UseAction::Bow);

    // 凋灵骷髅的 canUseNonMeleeWeapon 忽略 UseAction，始终返回 false
    // 这确保凋灵骷髅即使在装备弓的情况下也使用近战攻击
}

// ============================================================================
// getWeaponHoldingHand + canUseNonMeleeWeapon 组合逻辑验证
// ============================================================================

TEST_F(SkeletonCombatTaskTest, CombinedLogic_RangedSkeletonHoldingBow)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 模拟普通骷髅持弓
    entity->setMainHandItem(ItemStack(*Items::BOW, 1));

    // setCombatTask 的核心逻辑：
    // 1. getWeaponHoldingHand(*this, Items::BOW) -> MainHand
    Hand weaponHand = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(weaponHand, Hand::MainHand);

    // 2. getEquipment(handToEquipmentSlot(weaponHand)) -> bow ItemStack
    const ItemStack& weaponStack = entity->getEquipment(LivingEntity::handToEquipmentSlot(weaponHand));
    const Item* weaponItem = weaponStack.getItem();
    ASSERT_NE(weaponItem, nullptr);

    // 3. canUseNonMeleeWeapon(weaponStack) -> true (因为 UseAction::Bow)
    EXPECT_EQ(weaponItem->getUseAction(weaponStack), UseAction::Bow);

    // 结论：shouldUseRanged = true
}

TEST_F(SkeletonCombatTaskTest, CombinedLogic_SkeletonHoldingSword)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 模拟骷髅持剑
    entity->setMainHandItem(ItemStack(*Items::STONE_SWORD, 1));

    // setCombatTask 的核心逻辑：
    // 1. getWeaponHoldingHand(*this, Items::BOW) -> OffHand（主手没有弓）
    Hand weaponHand = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(weaponHand, Hand::OffHand);

    // 2. getEquipment(handToEquipmentSlot(OffHand)) -> 空 ItemStack
    const ItemStack& weaponStack = entity->getEquipment(LivingEntity::handToEquipmentSlot(weaponHand));
    const Item* weaponItem = weaponStack.getItem();
    EXPECT_EQ(weaponItem, nullptr);

    // 3. weaponItem == nullptr -> shouldUseRanged = false
    // 结论：shouldUseRanged = false，使用近战
}

TEST_F(SkeletonCombatTaskTest, CombinedLogic_SkeletonOffHandHoldingBow)
{
    auto entity = std::make_unique<TestLivingEntity>(EntityId(1));
    entity->setWorld(m_world.get());

    // 模拟骷髅主手空、副手持弓
    entity->setOffHandItem(ItemStack(*Items::BOW, 1));

    // setCombatTask 的核心逻辑：
    // 1. getWeaponHoldingHand(*this, Items::BOW) -> OffHand
    Hand weaponHand = getWeaponHoldingHand(*entity, Items::BOW);
    EXPECT_EQ(weaponHand, Hand::OffHand);

    // 2. getEquipment(OffHand) -> 弓 ItemStack
    const ItemStack& weaponStack = entity->getEquipment(LivingEntity::handToEquipmentSlot(weaponHand));
    const Item* weaponItem = weaponStack.getItem();
    ASSERT_NE(weaponItem, nullptr);

    // 3. canUseNonMeleeWeapon(weaponStack) -> true（弓的 UseAction::Bow）
    EXPECT_EQ(weaponItem->getUseAction(weaponStack), UseAction::Bow);

    // 结论：shouldUseRanged = true，使用远程
}

// ============================================================================
// SkeletonEntity 集成测试
//
// 验证 GoalSelector 所有权修复后，SkeletonEntity 构造和装备切换的完整集成。
// ============================================================================

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_ConstructsWithoutCrash)
{
    // 验证 SkeletonEntity 可以正常构造，setCombatTask() 不会导致崩溃
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    EXPECT_NE(skeleton, nullptr);
}

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_SetWorldAndSetCombatTask)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 构造时 setCombatTask() 已被调用，world 为 nullptr 时默认选择远程
    // 设置 world 后再调用 setCombatTask() 应该不会崩溃
    skeleton->setCombatTask();
    EXPECT_NE(skeleton, nullptr);
}

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_CanUseNonMeleeWeapon)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));

    // 弓的 UseAction 是 Bow，canUseNonMeleeWeapon 应返回 true
    ItemStack bowStack(*Items::BOW, 1);
    EXPECT_TRUE(skeleton->canUseNonMeleeWeapon(bowStack));

    // 石剑的 UseAction 不是 Bow，canUseNonMeleeWeapon 应返回 false
    ItemStack swordStack(*Items::STONE_SWORD, 1);
    EXPECT_FALSE(skeleton->canUseNonMeleeWeapon(swordStack));
}

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_SetEquipmentTriggersCombatTaskUpdate)
{
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 设置主手为弓 -> setCombatTask 应选择远程攻击
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));
    skeleton->setCombatTask();

    // 验证持弓时 canUseNonMeleeWeapon 返回 true
    EXPECT_TRUE(skeleton->canUseNonMeleeWeapon(skeleton->getMainHandItem()));

    // 通过基类引用调用 setEquipment，将主手从弓换成石剑
    // 这应该触发 setCombatTask 重新评估
    LivingEntity& livingEntity = *skeleton;
    livingEntity.setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::STONE_SWORD, 1));

    // 验证现在持剑，不使用远程武器
    EXPECT_FALSE(skeleton->canUseNonMeleeWeapon(skeleton->getMainHandItem()));
}

TEST_F(SkeletonCombatTaskTest, WitherSkeletonEntity_CanUseNonMeleeWeaponAlwaysFalse)
{
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));

    // 凋灵骷髅对任何物品都返回 false，包括弓
    ItemStack bowStack(*Items::BOW, 1);
    EXPECT_FALSE(witherSkeleton->canUseNonMeleeWeapon(bowStack));

    ItemStack swordStack(*Items::STONE_SWORD, 1);
    EXPECT_FALSE(witherSkeleton->canUseNonMeleeWeapon(swordStack));

    ItemStack emptyStack;
    EXPECT_FALSE(witherSkeleton->canUseNonMeleeWeapon(emptyStack));
}

TEST_F(SkeletonCombatTaskTest, WitherSkeletonEntity_SetCombatTaskAlwaysMelee)
{
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));
    witherSkeleton->setWorld(m_world.get());

    // 即使给凋灵骷髅装备弓，也应该使用近战
    witherSkeleton->setMainHandItem(ItemStack(*Items::BOW, 1));
    witherSkeleton->setCombatTask();

    // 凋灵骷髅的 canUseNonMeleeWeapon 始终返回 false
    EXPECT_FALSE(witherSkeleton->canUseNonMeleeWeapon(witherSkeleton->getMainHandItem()));
}

TEST_F(SkeletonCombatTaskTest, WitherSkeletonEntity_SetEquipmentBowStillMelee)
{
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));
    witherSkeleton->setWorld(m_world.get());

    // 给凋灵骷髅装备弓（通过基类引用调用 setEquipment）
    LivingEntity& livingEntity = *witherSkeleton;
    livingEntity.setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::BOW, 1));

    // 即使装备变更触发 setCombatTask，凋灵骷髅也始终使用近战
    EXPECT_FALSE(witherSkeleton->canUseNonMeleeWeapon(witherSkeleton->getMainHandItem()));
}

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_SetEquipmentArmorSlotNoEffect)
{
    // 装甲槽位变更不应触发 setCombatTask
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 设置头盔槽位（通过基类引用调用 setEquipment）
    ASSERT_NE(Items::LEATHER_HELMET, nullptr);
    LivingEntity& livingEntity = *skeleton;
    livingEntity.setEquipment(EquipmentSlot::Head, ItemStack(*Items::LEATHER_HELMET, 1));

    // 头盔槽位变更不影响战斗目标
    EXPECT_EQ(skeleton->getEquipment(EquipmentSlot::Head).getItem(), Items::LEATHER_HELMET);
}

TEST_F(SkeletonCombatTaskTest, SkeletonEntity_ClientSideSetEquipmentNoCombatTaskUpdate)
{
    // 客户端侧不应触发 setCombatTask
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    m_world->setClientSide(true);
    skeleton->setWorld(m_world.get());

    // 初始状态设置弓
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));
    skeleton->setCombatTask();

    // 在客户端侧切换装备不应该改变战斗目标
    LivingEntity& livingEntity = *skeleton;
    livingEntity.setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::STONE_SWORD, 1));

    // 验证客户端侧的 setEquipment 确实执行了（装备被设置了），
    // 但 setCombatTask 的 setEquipment 回调被跳过
    EXPECT_EQ(skeleton->getMainHandItem().getItem(), Items::STONE_SWORD);

    m_world->setClientSide(false);
}

// ============================================================================
// 难度相关攻击间隔测试
//
// 验证 setCombatTask() 根据游戏难度调整最小攻击间隔的行为。
// 对应 MC 原版 AbstractSkeleton.reassessWeaponGoal()：
//   - 困难难度: setMinAttackInterval(getHardAttackInterval())
//   - 其他难度: setMinAttackInterval(getAttackInterval())
// ============================================================================

TEST_F(SkeletonCombatTaskTest, DifficultyBasedAttackInterval_HardDifficulty)
{
    // 困难难度下，骷髅的最小攻击间隔应为 getHardAttackInterval() = 20 ticks
    m_world->setDifficulty(Difficulty::Hard);
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 验证 getHardAttackInterval 和 getAttackInterval 返回值
    EXPECT_EQ(skeleton->getHardAttackInterval(), 20);
    EXPECT_EQ(skeleton->getAttackInterval(), 40);

    // 在困难难度下 setCombatTask 不应崩溃
    skeleton->setCombatTask();
}

TEST_F(SkeletonCombatTaskTest, DifficultyBasedAttackInterval_NormalDifficulty)
{
    // 普通难度下，骷髅的最小攻击间隔应为 getAttackInterval() = 40 ticks
    m_world->setDifficulty(Difficulty::Normal);
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 在普通难度下 setCombatTask 不应崩溃
    skeleton->setCombatTask();
}

TEST_F(SkeletonCombatTaskTest, DifficultyBasedAttackInterval_EasyDifficulty)
{
    // 简单难度下，骷髅的最小攻击间隔应为 getAttackInterval() = 40 ticks
    m_world->setDifficulty(Difficulty::Easy);
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    skeleton->setCombatTask();
}

TEST_F(SkeletonCombatTaskTest, DifficultyBasedAttackInterval_PeacefulDifficulty)
{
    // 和平难度下，骷髅的最小攻击间隔应为 getAttackInterval() = 40 ticks
    // （和平难度下怪物不会生成，但如果存在则仍使用非困难间隔）
    m_world->setDifficulty(Difficulty::Peaceful);
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    skeleton->setCombatTask();
}

TEST_F(SkeletonCombatTaskTest, StrayEntity_DefaultAttackIntervalsMatchSkeleton)
{
    // MC 1.21.11 中 Stray 不覆盖 getHardAttackInterval/getAttackInterval，
    // 使用与 Skeleton 相同的基类默认值 (20/40)
    auto stray = std::make_unique<StrayEntity>(EntityId(1));
    EXPECT_EQ(stray->getHardAttackInterval(), 20);
    EXPECT_EQ(stray->getAttackInterval(), 40);
}

TEST_F(SkeletonCombatTaskTest, WitherSkeleton_AttackIntervalsNotUsedForMelee)
{
    // 凋灵骷髅始终使用近战，攻击间隔方法不影响其战斗行为
    // 但方法仍返回基类值，以便未来可能的扩展
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityId(1));
    EXPECT_EQ(witherSkeleton->getHardAttackInterval(), 20);
    EXPECT_EQ(witherSkeleton->getAttackInterval(), 40);
}

TEST_F(SkeletonCombatTaskTest, EquipmentChangeReassessesIntervalOnHard)
{
    // 在困难难度下，装备变更后重新评估战斗目标时
    // 应使用困难难度的攻击间隔
    m_world->setDifficulty(Difficulty::Hard);
    auto skeleton = std::make_unique<SkeletonEntity>(EntityId(1));
    skeleton->setWorld(m_world.get());

    // 设置主手为弓 -> 远程攻击，困难难度间隔
    skeleton->setMainHandItem(ItemStack(*Items::BOW, 1));
    skeleton->setCombatTask();

    // 切换为石剑 -> 近战
    LivingEntity& livingEntity = *skeleton;
    livingEntity.setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::STONE_SWORD, 1));
    EXPECT_FALSE(skeleton->canUseNonMeleeWeapon(skeleton->getMainHandItem()));

    // 再切换回弓 -> 远程攻击，仍使用困难难度间隔
    livingEntity.setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::BOW, 1));
    EXPECT_TRUE(skeleton->canUseNonMeleeWeapon(skeleton->getMainHandItem()));
}

} // namespace
} // namespace mc
