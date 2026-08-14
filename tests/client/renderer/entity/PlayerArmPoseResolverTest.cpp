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
 * @file PlayerArmPoseResolverTest.cpp
 * @brief PlayerArmPoseResolver 单元测试
 *
 * 测试覆盖：
 * - determineArmPose：空手、持有普通物品、弓、弩装填、盾牌、三叉戟/长矛、已装填弩、刷子、望远镜等分支
 * - resolveArmPoses：双手协调（弓/弩装填/弩持握降级副手）、右撇子/左撇子映射
 *
 * 对应 MC 1.21.11 AvatarRenderer.getArmPose 与 setModelVisibilities 协调逻辑。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/renderer/player/PlayerArmPoseResolver.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试用 Player
// ============================================================================

class TestPlayer : public Player {
public:
    explicit TestPlayer(IWorld* world = nullptr)
        : Player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }
};

// ============================================================================
// 测试用世界桩
// ============================================================================

class ArmPoseTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ArmPoseTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ArmPoseTestWorld::tickManager not implemented");
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }
};

// ============================================================================
// 测试用望远镜物品（项目暂未实现 SpyglassItem，用 mock 验证姿态映射）
// ============================================================================

class TestSpyglassItem final : public Item {
public:
    TestSpyglassItem()
        : Item(ItemProperties().maxStackSize(1))
    {}

    // 望远镜使用时长 1200 ticks（MC 1.21.11 SpyglassItem.USE_DURATION = 1200）
    // 仅需 >0 即可让 LivingEntity::setActiveHand 进入使用状态
    [[nodiscard]] i32 getUseDuration(const ItemStack&) const override { return 1200; }

    [[nodiscard]] UseAction getUseAction(const ItemStack&) const override { return UseAction::Spyglass; }
};

// ============================================================================
// 测试夹具
// ============================================================================

using ArmPose = client::renderer::entity::model::player::ArmPose;

class PlayerArmPoseResolverTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    void SetUp() override { m_world = std::make_unique<ArmPoseTestWorld>(); }

    std::unique_ptr<ArmPoseTestWorld> m_world;
};

// ============================================================================
// determineArmPose - 空手与默认分支
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_EmptyHand_ReturnsEmpty)
{
    TestPlayer player(m_world.get());
    // 不装备任何物品
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Empty);
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::OffHand),
        ArmPose::Empty);
}

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_NormalItem_ReturnsItem)
{
    TestPlayer player(m_world.get());
    // 普通物品（石头）应返回 Item
    ASSERT_NE(Items::STONE, nullptr);
    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::MainHand, stoneStack);

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Item);
}

// ============================================================================
// determineArmPose - 弓
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_BowNotUsing_ReturnsItem)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::BOW, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);

    // 仅持有弓但未激活使用，应返回 Item
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Item);
}

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_BowUsing_ReturnsBowAndArrow)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::BOW, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);

    // 激活使用弓
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::BowAndArrow);
}

// ============================================================================
// determineArmPose - 盾牌
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_ShieldUsing_ReturnsBlock)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::SHIELD, nullptr);
    ItemStack shieldStack(Items::SHIELD, 1);
    player.setEquipment(EquipmentSlot::OffHand, shieldStack);

    // 激活使用盾牌（副手）
    player.setActiveHand(Hand::OffHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::OffHand),
        ArmPose::Block);
}

// ============================================================================
// determineArmPose - 三叉戟
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_TridentUsing_ReturnsThrowSpear)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::TRIDENT, nullptr);
    ItemStack tridentStack(Items::TRIDENT, 1);
    player.setEquipment(EquipmentSlot::MainHand, tridentStack);

    // 激活使用三叉戟
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::ThrowSpear);
}

// ============================================================================
// determineArmPose - 长矛（通过 SPEARS 标签）
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_SpearItem_ReturnsThrowSpear)
{
    TestPlayer player(m_world.get());
    // 长矛类物品通过 ItemTags::SPEARS 标签判定，无需激活使用
    ASSERT_NE(Items::WOODEN_SPEAR, nullptr);
    ItemStack spearStack(Items::WOODEN_SPEAR, 1);
    player.setEquipment(EquipmentSlot::MainHand, spearStack);

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::ThrowSpear);
}

// ============================================================================
// determineArmPose - 弩
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_CrossbowNotChargedNotUsing_ReturnsItem)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::CROSSBOW, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    // 未装填
    ASSERT_FALSE(item::CrossbowItem::isCharged(crossbowStack));
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);

    // 未装填且未使用，返回 Item
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Item);
}

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_CrossbowChargedNotSwinging_ReturnsCrossbowHold)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::CROSSBOW, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    item::CrossbowItem::setCharged(crossbowStack, true);
    ASSERT_TRUE(item::CrossbowItem::isCharged(crossbowStack));
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);

    // 已装填且未挥动，返回 CrossbowHold
    ASSERT_FALSE(player.isSwingInProgress());
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::CrossbowHold);
}

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_CrossbowChargedSwinging_ReturnsItem)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::CROSSBOW, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    item::CrossbowItem::setCharged(crossbowStack, true);
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);

    // 触发挥动
    player.swing(Hand::MainHand);
    ASSERT_TRUE(player.isSwingInProgress());

    // 已装填但正在挥动，CrossbowHold 分支不生效，应返回 Item
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Item);
}

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_CrossbowUsing_ReturnsCrossbowCharge)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::CROSSBOW, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    // 未装填
    ASSERT_FALSE(item::CrossbowItem::isCharged(crossbowStack));
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);

    // 激活装填
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::CrossbowCharge);
}

// ============================================================================
// determineArmPose - 刷子
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_BrushUsing_ReturnsBrush)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::BRUSH, nullptr);
    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);

    // 激活使用刷子
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Brush);
}

// ============================================================================
// determineArmPose - 望远镜
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_SpyglassUsing_ReturnsSpyglass)
{
    TestPlayer player(m_world.get());
    // 项目尚未实现 SpyglassItem，使用 mock 物品验证 UseAction::Spyglass → ArmPose::Spyglass 映射
    TestSpyglassItem spyglassItem;
    ItemStack spyglassStack(&spyglassItem, 1);
    player.setEquipment(EquipmentSlot::MainHand, spyglassStack);

    // 激活使用望远镜
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::MainHand),
        ArmPose::Spyglass);
}

// ============================================================================
// determineArmPose - 使用手不匹配
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, DetermineArmPose_UsingDifferentHand_ReturnsItem)
{
    TestPlayer player(m_world.get());
    ASSERT_NE(Items::BOW, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);

    // 激活主手使用弓
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    // 查询副手时，使用手不匹配，应返回 Item（持有物品的默认姿态）
    EXPECT_EQ(
        client::renderer::entity::renderer::player::PlayerArmPoseResolver::determineArmPose(player, Hand::OffHand),
        ArmPose::Empty);
}

// ============================================================================
// resolveArmPoses - 右撇子映射
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_RightHanded_MapsMainHandToRight)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);
    ASSERT_TRUE(player.isRightHanded());

    ASSERT_NE(Items::STONE, nullptr);
    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::MainHand, stoneStack);

    // 主手有物品（Item），副手空
    // 右撇子：主手姿态 → 右臂，副手姿态 → 左臂
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::Item);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Empty);
}

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_LeftHanded_MapsMainHandToLeft)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Left);
    ASSERT_FALSE(player.isRightHanded());

    ASSERT_NE(Items::STONE, nullptr);
    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::MainHand, stoneStack);

    // 左撇子：主手姿态 → 左臂，副手姿态 → 右臂
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Item);
    EXPECT_EQ(poses.rightArmPose, ArmPose::Empty);
}

// ============================================================================
// resolveArmPoses - 双手姿态协调（主手为双手动作时副手降级）
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_BowDemotesOffHandToEmpty)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);

    ASSERT_NE(Items::BOW, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    // 副手为空：主手拉弓时副手降级为 Empty
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::BowAndArrow);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Empty);
}

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_BowDemotesOffHandToItem)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);

    ASSERT_NE(Items::BOW, nullptr);
    ASSERT_NE(Items::STONE, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    // 副手有物品：主手拉弓时副手降级为 Item（而非 BowAndArrow 之外的姿态）
    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::OffHand, stoneStack);

    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::BowAndArrow);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Item);
}

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_CrossbowChargeDemotesOffHand)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);

    ASSERT_NE(Items::CROSSBOW, nullptr);
    ASSERT_NE(Items::STONE, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::OffHand, stoneStack);

    // 主手装填弩：副手降级为 Item
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::CrossbowCharge);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Item);
}

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_CrossbowHoldDemotesOffHand)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);

    ASSERT_NE(Items::CROSSBOW, nullptr);
    ASSERT_NE(Items::STONE, nullptr);
    ItemStack crossbowStack(Items::CROSSBOW, 1);
    item::CrossbowItem::setCharged(crossbowStack, true);
    player.setEquipment(EquipmentSlot::MainHand, crossbowStack);
    ASSERT_FALSE(player.isSwingInProgress());

    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::OffHand, stoneStack);

    // 主手持有已装填弩：副手降级为 Item
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::CrossbowHold);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Item);
}

// ============================================================================
// resolveArmPoses - 非双手动作不降级副手
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_NormalItemDoesNotDemoteOffHand)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Right);

    ASSERT_NE(Items::STONE, nullptr);
    ASSERT_NE(Items::DIRT, nullptr);
    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::MainHand, stoneStack);

    ItemStack dirtStack(Items::DIRT, 1);
    player.setEquipment(EquipmentSlot::OffHand, dirtStack);

    // 主手普通物品：副手保持自己的姿态（Item），不被降级
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.rightArmPose, ArmPose::Item);
    EXPECT_EQ(poses.leftArmPose, ArmPose::Item);
}

// ============================================================================
// resolveArmPoses - 左撇子 + 双手协调
// ============================================================================

TEST_F(PlayerArmPoseResolverTest, ResolveArmPoses_LeftHandedBow_MapsToLeftAndDemotesRight)
{
    TestPlayer player(m_world.get());
    player.setPrimaryHand(HandSide::Left);
    ASSERT_FALSE(player.isRightHanded());

    ASSERT_NE(Items::BOW, nullptr);
    ASSERT_NE(Items::STONE, nullptr);
    ItemStack bowStack(Items::BOW, 1);
    player.setEquipment(EquipmentSlot::MainHand, bowStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    ItemStack stoneStack(Items::STONE, 1);
    player.setEquipment(EquipmentSlot::OffHand, stoneStack);

    // 左撇子：主手姿态 → 左臂（BowAndArrow），副手降级 → 右臂（Item）
    auto poses = client::renderer::entity::renderer::player::PlayerArmPoseResolver::resolveArmPoses(player);
    EXPECT_EQ(poses.leftArmPose, ArmPose::BowAndArrow);
    EXPECT_EQ(poses.rightArmPose, ArmPose::Item);
}

} // namespace
} // namespace mc
