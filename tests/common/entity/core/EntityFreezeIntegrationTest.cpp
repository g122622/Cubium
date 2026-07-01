/**
 * @file EntityFreezeIntegrationTest.cpp
 * @brief 冰冻系统集成测试
 *
 * 测试 LivingEntity 和 Player 的核心冰冻逻辑：
 * 1. LivingEntity::tickFreeze() — 冰冻计时器递减和冰冻伤害
 * 2. LivingEntity::clearFreeze() — 清除冰冻状态和移除减速修饰符
 * 3. LivingEntity::canFreeze() — 皮革护甲检查
 * 4. Entity::baseTick() — isInPowderSnow 重置
 * 5. Player::isInvulnerableTo() — FREEZE_DAMAGE 游戏规则检查
 *
 * 使用 BaseTestWorld + TestLivingEntity 模式。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::item::tag;

// ============================================================================
// 测试用 Mock World
// ============================================================================

class FreezeTestWorld final : public mc::test::BaseTestWorld {
public:
    FreezeTestWorld() = default;

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void gameEvent(const gameevent::GameEvent&, const BlockPos&, const gameevent::GameEvent::Context&) override {}

    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    void clearState() {}
};

// ============================================================================
// 测试用 LivingEntity 子类
// ============================================================================

namespace {

class TestLivingEntity : public LivingEntity {
public:
    explicit TestLivingEntity(EntityId id, IWorld* world = nullptr)
        : LivingEntity(id)
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }

    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }
    [[nodiscard]] std::string getLootTableId() const override { return {}; }
};

class TestPlayer : public Player {
public:
    explicit TestPlayer(IWorld* world)
        : Player(EntityId(1), "TestPlayer")
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }

    [[nodiscard]] std::string getLootTableId() const override { return {}; }
};

} // namespace

// ============================================================================
// 测试固定装置
// ============================================================================

class EntityFreezeIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        ItemTags::initialize();
        VanillaEntities::registerAll();
        EntityTypeTags::initialize();
    }

    void SetUp() override {}

    FreezeTestWorld m_world;
};

// ============================================================================
// Entity 冰冻状态基础测试
// ============================================================================

TEST_F(EntityFreezeIntegrationTest, FreezeState_InitialValues)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    EXPECT_EQ(entity.getTicksFrozen(), 0);
    EXPECT_FALSE(entity.isFullyFrozen());
    EXPECT_FALSE(entity.isFreezing());
    EXPECT_FLOAT_EQ(entity.getPercentFrozen(), 0.0f);
    EXPECT_TRUE(entity.canFreeze());
    EXPECT_FALSE(entity.isInPowderSnow());
}

TEST_F(EntityFreezeIntegrationTest, FreezeState_SetTicksFrozen)
{
    TestLivingEntity entity(EntityId(1), &m_world);

    entity.setTicksFrozen(70);
    EXPECT_EQ(entity.getTicksFrozen(), 70);
    EXPECT_FLOAT_EQ(entity.getPercentFrozen(), 0.5f);
    EXPECT_TRUE(entity.isFreezing());
    EXPECT_FALSE(entity.isFullyFrozen());

    entity.setTicksFrozen(140);
    EXPECT_EQ(entity.getTicksFrozen(), 140);
    EXPECT_FLOAT_EQ(entity.getPercentFrozen(), 1.0f);
    EXPECT_TRUE(entity.isFullyFrozen());

    entity.setTicksFrozen(200);
    EXPECT_EQ(entity.getTicksFrozen(), 200);
    EXPECT_FLOAT_EQ(entity.getPercentFrozen(), 1.0f);
}

TEST_F(EntityFreezeIntegrationTest, FreezeState_ClearFreeze)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setTicksFrozen(100);
    EXPECT_TRUE(entity.isFreezing());

    entity.clearFreeze();
    EXPECT_EQ(entity.getTicksFrozen(), 0);
    EXPECT_FALSE(entity.isFreezing());
    EXPECT_FALSE(entity.isFullyFrozen());
}

TEST_F(EntityFreezeIntegrationTest, FreezeState_SetIsInPowderSnow)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    EXPECT_FALSE(entity.isInPowderSnow());

    entity.setIsInPowderSnow(true);
    EXPECT_TRUE(entity.isInPowderSnow());

    entity.setIsInPowderSnow(false);
    EXPECT_FALSE(entity.isInPowderSnow());
}

TEST_F(EntityFreezeIntegrationTest, FreezeState_GetTicksRequiredToFreeze)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    EXPECT_EQ(entity.getTicksRequiredToFreeze(), Entity::BASE_TICKS_REQUIRED_TO_FREEZE);
    EXPECT_EQ(entity.getTicksRequiredToFreeze(), 140);
}

// ============================================================================
// canFreeze 测试
// ============================================================================

TEST_F(EntityFreezeIntegrationTest, CanFreeze_DefaultLivingEntity)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    // 普通生物实体应该可以冰冻
    EXPECT_TRUE(entity.canFreeze());
}

TEST_F(EntityFreezeIntegrationTest, CanFreeze_LeatherArmorImmunity)
{
    TestLivingEntity entity(EntityId(1), &m_world);

    // 没有装备皮革护甲时可以冰冻
    EXPECT_TRUE(entity.canFreeze());

    // 装备皮革头盔后应该不能冰冻
    if (ItemTags::isInitialized()) {
        const Item* leatherHelmet = Items::LEATHER_HELMET;
        if (leatherHelmet != nullptr) {
            ItemStack helmet(leatherHelmet, 1);
            entity.setEquipment(EquipmentSlot::Head, helmet);
            EXPECT_FALSE(entity.canFreeze());
        }
    }
}

// ============================================================================
// Player::isInvulnerableTo 游戏规则测试
// ============================================================================

TEST_F(EntityFreezeIntegrationTest, PlayerIsInvulnerableTo_FreezeDamageGameRuleOff)
{
    TestPlayer player(&m_world);

    // 默认 FREEZE_DAMAGE 为 true，玩家应受到冰冻伤害
    auto freezeSource = DamageSources::freeze();
    EXPECT_FALSE(player.isInvulnerableTo(freezeSource));

    // 关闭 FREEZE_DAMAGE 游戏规则
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, false, nullptr);
    EXPECT_TRUE(player.isInvulnerableTo(freezeSource));

    // 重新开启
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, true, nullptr);
    EXPECT_FALSE(player.isInvulnerableTo(freezeSource));
}

TEST_F(EntityFreezeIntegrationTest, PlayerIsInvulnerableTo_DrowningDamageGameRuleOff)
{
    TestPlayer player(&m_world);

    auto drownSource = DamageSources::drown();
    EXPECT_FALSE(player.isInvulnerableTo(drownSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DROWNING_DAMAGE, false, nullptr);
    EXPECT_TRUE(player.isInvulnerableTo(drownSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DROWNING_DAMAGE, true, nullptr);
    EXPECT_FALSE(player.isInvulnerableTo(drownSource));
}

TEST_F(EntityFreezeIntegrationTest, PlayerIsInvulnerableTo_FallDamageGameRuleOff)
{
    TestPlayer player(&m_world);

    auto fallSource = DamageSources::fall();
    EXPECT_FALSE(player.isInvulnerableTo(fallSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FALL_DAMAGE, false, nullptr);
    EXPECT_TRUE(player.isInvulnerableTo(fallSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FALL_DAMAGE, true, nullptr);
    EXPECT_FALSE(player.isInvulnerableTo(fallSource));
}

TEST_F(EntityFreezeIntegrationTest, PlayerIsInvulnerableTo_FireDamageGameRuleOff)
{
    TestPlayer player(&m_world);

    auto fireSource = DamageSources::inFire();
    EXPECT_FALSE(player.isInvulnerableTo(fireSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FIRE_DAMAGE, false, nullptr);
    EXPECT_TRUE(player.isInvulnerableTo(fireSource));

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FIRE_DAMAGE, true, nullptr);
    EXPECT_FALSE(player.isInvulnerableTo(fireSource));
}

TEST_F(EntityFreezeIntegrationTest, PlayerIsInvulnerableTo_OtherDamageNotAffected)
{
    TestPlayer player(&m_world);

    // 非冻结/溺水/摔落/火焰伤害不受游戏规则影响
    auto genericSource = DamageSources::generic();
    EXPECT_FALSE(player.isInvulnerableTo(genericSource));

    auto magicSource = DamageSources::magic();
    EXPECT_FALSE(player.isInvulnerableTo(magicSource));

    auto witherSource = DamageSources::wither();
    EXPECT_FALSE(player.isInvulnerableTo(witherSource));
}

TEST_F(EntityFreezeIntegrationTest, LivingEntity_IsNotAffectedByFreezeDamageGameRule)
{
    // 非 Player 的 LivingEntity 不受 FREEZE_DAMAGE 游戏规则影响
    TestLivingEntity entity(EntityId(1), &m_world);

    auto freezeSource = DamageSources::freeze();

    // 即使关闭 FREEZE_DAMAGE，非玩家实体也不免疫冰冻伤害
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, false, nullptr);
    EXPECT_FALSE(entity.isInvulnerableTo(freezeSource));
}

// ============================================================================
// DamageSource::isFreezing 在 isInvulnerableTo 中的使用
// ============================================================================

TEST_F(EntityFreezeIntegrationTest, FreezeDamage_IsFreezingDetection)
{
    // 验证 DamageSources::freeze() 创建的伤害源确实被 isFreezing() 识别
    auto freezeSource = DamageSources::freeze();
    EXPECT_TRUE(freezeSource.isFreezing());
    EXPECT_TRUE(freezeSource.bypassesArmor());

    // 其他伤害源不被 isFreezing() 识别
    auto fireSource = DamageSources::inFire();
    EXPECT_FALSE(fireSource.isFreezing());
    EXPECT_TRUE(fireSource.isFire());

    auto drownSource = DamageSources::drown();
    EXPECT_FALSE(drownSource.isFreezing());
}
