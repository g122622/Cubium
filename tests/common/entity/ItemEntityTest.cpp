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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::serialization::nbt_keys;
using namespace mc::entity::serialization::nbt_helper;
using namespace mc::item::tag;

// ============================================================================
// 测试用 MobEntity 子类（用于 mobGriefing 伤害源测试）
// ============================================================================

namespace {

class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityId(100))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

// ============================================================================
// 测试用 Mock World（支持 GameRules 和 GameEvent 捕获）
// ============================================================================

class ItemEntityTestWorld final : public mc::test::BaseTestWorld {
public:
    ItemEntityTestWorld() = default;

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_lastGameEventId = event.id();
        m_lastGameEventPos = pos;
        m_lastGameEventSourceEntity = context.sourceEntity();
        m_gameEventCount++;
    }

    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] const std::string& lastGameEventId() const { return m_lastGameEventId; }
    [[nodiscard]] const BlockPos& lastGameEventPos() const { return m_lastGameEventPos; }
    [[nodiscard]] const Entity* lastGameEventSourceEntity() const { return m_lastGameEventSourceEntity; }

    void clearGameState()
    {
        m_gameEventCount = 0;
        m_lastGameEventId.clear();
        m_lastGameEventPos = BlockPos(0, 0, 0);
        m_lastGameEventSourceEntity = nullptr;
    }

private:
    i32 m_gameEventCount = 0;
    std::string m_lastGameEventId;
    BlockPos m_lastGameEventPos{0, 0, 0};
    const Entity* m_lastGameEventSourceEntity = nullptr;
};

// ============================================================================
// 带世界支持的 ItemEntity 测试固定装置
// 所有 hurt/canBeHurtBy/isImmuneToFire/mobGriefing/GameEvent/NBT 测试
// 统一放在此固定装置中，以确保初始化顺序正确
// ============================================================================

class ItemEntityWorldTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        ItemTags::initialize();
    }

    void SetUp() override { m_world.clearGameState(); }

    ItemEntityTestWorld m_world;
};

// ============================================================================
// ItemEntity 默认值测试
// ============================================================================

TEST_F(ItemEntityWorldTest, DefaultHealthIsFive)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.getHealth(), 5);
}

TEST_F(ItemEntityWorldTest, DefaultPickupDelayIsTen)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.getPickupDelay(), 10);
}

TEST_F(ItemEntityWorldTest, DefaultLifetimeIs6000)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.DEFAULT_LIFETIME, 6000);
}

TEST_F(ItemEntityWorldTest, Hurt_DefaultHealthIsFive)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.DEFAULT_HEALTH, 5);
}

// ============================================================================
// ItemEntity::hurt 基础测试（stone 物品）
// ============================================================================

TEST_F(ItemEntityWorldTest, Hurt_InvulnerableEntityReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    auto genericDamage = DamageSources::generic();
    EXPECT_FALSE(entity.hurt(genericDamage, 1.0f));
}

TEST_F(ItemEntityWorldTest, Hurt_NormalItemHurtByFireReducesHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto fireDamage = DamageSources::inFire();
    bool result = entity.hurt(fireDamage, 3.0f);
    EXPECT_TRUE(result);
    EXPECT_EQ(entity.getHealth(), 2);
}

TEST_F(ItemEntityWorldTest, Hurt_NormalItemHurtByGenericDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 2.0f);
    EXPECT_TRUE(result);
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityWorldTest, Hurt_HealthZeroDiscardsEntity)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 6.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityWorldTest, Hurt_ExactHealthZeroDiscardsEntity)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 5.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityWorldTest, Hurt_MultipleHitsReduceHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();

    // 第一击：5 - 2 = 3
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 3);
    EXPECT_FALSE(entity.isRemoved());

    // 第二击：3 - 2 = 1
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 1);
    EXPECT_FALSE(entity.isRemoved());

    // 第三击：1 - 2 = -1 -> 销毁
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityWorldTest, Hurt_VoidDamageBypassesInvulnerability)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    auto voidDamage = DamageSources::outOfWorld();
    bool result = entity.hurt(voidDamage, 100.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

// ============================================================================
// ItemStack::canBeHurtBy 测试
// ============================================================================

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NormalItemCanBeHurtByFire)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_TRUE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NormalItemCanBeHurtByLava)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto lavaDamage = DamageSources::lava();
    EXPECT_TRUE(stack.canBeHurtBy(lavaDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NormalItemCanBeHurtByGenericDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(stack.canBeHurtBy(genericDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NetheriteItemCannotBeHurtByFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NetheriteItemCannotBeHurtByLava)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    auto lavaDamage = DamageSources::lava();
    EXPECT_FALSE(stack.canBeHurtBy(lavaDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NetheriteItemCanBeHurtByGenericDamage)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(stack.canBeHurtBy(genericDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_NetherStarCannotBeHurtByFire)
{
    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(netherStar, nullptr);
    ItemStack stack(*netherStar, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityWorldTest, CanBeHurtBy_EmptyStackReturnsFalse)
{
    ItemStack emptyStack;
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(emptyStack.canBeHurtBy(fireDamage));
}

// ============================================================================
// ItemEntity::isImmuneToFire 测试
// ============================================================================

TEST_F(ItemEntityWorldTest, IsImmuneToFire_NormalItemNotImmune)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(entity.isImmuneToFire());
}

TEST_F(ItemEntityWorldTest, IsImmuneToFire_NetheriteItemImmune)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

TEST_F(ItemEntityWorldTest, IsImmuneToFire_NetherStarImmune)
{
    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(netherStar, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netherStar, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

TEST_F(ItemEntityWorldTest, IsImmuneToFire_AncientDebrisImmune)
{
    Item* debris = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "ancient_debris"));
    ASSERT_NE(debris, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*debris, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

// ============================================================================
// FIRE_RESISTANT / 下界合金物品 hurt 测试
// ============================================================================

TEST_F(ItemEntityWorldTest, Hurt_FireResistantItemNotHurtByFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(entity.hurt(fireDamage, 3.0f));
}

TEST_F(ItemEntityWorldTest, Hurt_NetheriteItemHurtByGenericNotFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 2.0f);
    EXPECT_TRUE(result);
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityWorldTest, Hurt_NetheriteItemVoidDamageKills)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    auto voidDamage = DamageSources::outOfWorld();
    bool result = entity.hurt(voidDamage, 100.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

// ============================================================================
// mobGriefing 游戏规则测试
// ============================================================================

TEST_F(ItemEntityWorldTest, MobDamage_MobGriefingOn_AllowsDamage)
{
    // mobGriefing 默认为 true，Mob 伤害应被允许
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    TestMobEntity mob;
    auto damage = DamageSources::mobAttack(&mob);

    EXPECT_TRUE(entity.hurt(damage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityWorldTest, MobDamage_MobGriefingOff_RejectsDamage)
{
    // 关闭 mobGriefing 后，Mob 伤害应被拒绝
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    ASSERT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    TestMobEntity mob;
    auto damage = DamageSources::mobAttack(&mob);

    EXPECT_FALSE(entity.hurt(damage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 5); // 生命值不变
}

TEST_F(ItemEntityWorldTest, EnvironmentalDamage_MobGriefingOff_AllowsDamage)
{
    // mobGriefing 关闭时，非 Mob 来源的环境伤害应正常生效
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    ASSERT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityWorldTest, MobDamage_NoWorld_SkipsMobGriefingCheck)
{
    // 没有 world 时，mobGriefing 检查被跳过，Mob 伤害正常生效
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    // 不设置 world -> m_world == nullptr

    TestMobEntity mob;
    auto damage = DamageSources::mobAttack(&mob);

    EXPECT_TRUE(entity.hurt(damage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityWorldTest, MobDamage_MobGriefingOff_DoesNotSetMarkHurt)
{
    // mobGriefing 关闭时，被拒绝的 Mob 伤害不应设置 hurtMarked
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    TestMobEntity mob;
    auto damage = DamageSources::mobAttack(&mob);

    EXPECT_FALSE(entity.hurt(damage, 2.0f));
    EXPECT_FALSE(entity.isHurtMarked());
}

// ============================================================================
// ENTITY_DAMAGE 游戏事件派发测试
// ============================================================================

TEST_F(ItemEntityWorldTest, Hurt_DispatchesEntityDamageGameEvent)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.5f, 64.3f, 10.7f);
    entity.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));

    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventId(), "entity_damage");
}

TEST_F(ItemEntityWorldTest, Hurt_GameEventPositionMatchesEntityBlockPos)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    // 位置 (5.5, 64.3, 10.7) -> BlockPos (5, 64, 10)
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.5f, 64.3f, 10.7f);
    entity.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));

    EXPECT_EQ(m_world.lastGameEventPos(), BlockPos(5, 64, 10));
}

TEST_F(ItemEntityWorldTest, Hurt_GameEventContextContainsSourceEntity)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    TestMobEntity mob;
    auto mobDamage = DamageSources::mobAttack(&mob);
    EXPECT_TRUE(entity.hurt(mobDamage, 1.0f));

    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventSourceEntity(), &mob);
}

TEST_F(ItemEntityWorldTest, Hurt_GameEventContextNullForEnvironmentalDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));

    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventSourceEntity(), nullptr);
}

TEST_F(ItemEntityWorldTest, Hurt_InvulnerableDoesNotDispatchGameEvent)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);
    entity.setInvulnerable(true);

    auto genericDamage = DamageSources::generic();
    EXPECT_FALSE(entity.hurt(genericDamage, 1.0f));
    EXPECT_EQ(m_world.gameEventCount(), 0);
}

TEST_F(ItemEntityWorldTest, Hurt_FireResistantNotHurtByFire_NoGameEvent)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(entity.hurt(fireDamage, 3.0f));
    EXPECT_EQ(m_world.gameEventCount(), 0);
}

TEST_F(ItemEntityWorldTest, Hurt_MobGriefingOff_NoGameEvent)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    TestMobEntity mob;
    auto mobDamage = DamageSources::mobAttack(&mob);
    EXPECT_FALSE(entity.hurt(mobDamage, 1.0f));
    EXPECT_EQ(m_world.gameEventCount(), 0);
}

TEST_F(ItemEntityWorldTest, Hurt_MultipleHitsDispatchMultipleEvents)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    entity.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();

    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    EXPECT_EQ(m_world.gameEventCount(), 1);

    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    EXPECT_EQ(m_world.gameEventCount(), 2);
}

TEST_F(ItemEntityWorldTest, Hurt_NoWorld_DoesNotDispatchGameEvent)
{
    // 没有 world 时不会派发游戏事件
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 5.0f, 64.0f, 10.0f);
    // 不设置 world

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    // m_world 是 nullptr，不会派发游戏事件，也不会崩溃
}

// ============================================================================
// markHurt() / isHurtMarked() 验证测试
// ============================================================================

TEST_F(ItemEntityWorldTest, Hurt_Success_SetsMarkHurt)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(entity.isHurtMarked()); // 默认为 false

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_TRUE(entity.isHurtMarked()); // 受伤后应为 true
}

TEST_F(ItemEntityWorldTest, Hurt_Invulnerable_DoesNotSetMarkHurt)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    auto genericDamage = DamageSources::generic();
    EXPECT_FALSE(entity.hurt(genericDamage, 1.0f));
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST_F(ItemEntityWorldTest, Hurt_FireResistantItem_NotHurtByFire_DoesNotSetMarkHurt)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(entity.hurt(fireDamage, 3.0f));
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST_F(ItemEntityWorldTest, Hurt_FireResistantItem_HurtByGeneric_SetsMarkHurt)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    // 防火物品被普通伤害击中时，hurt() 成功，应设置 hurtMarked
    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST_F(ItemEntityWorldTest, Hurt_MultipleHits_KeepHurtMarkedTrue)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();

    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    EXPECT_TRUE(entity.isHurtMarked());

    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    EXPECT_TRUE(entity.isHurtMarked()); // 多次受伤后仍为 true
}

TEST_F(ItemEntityWorldTest, Hurt_ClearHurtMarkedResetsFlag)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(entity.hurt(genericDamage, 1.0f));
    EXPECT_TRUE(entity.isHurtMarked());

    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST_F(ItemEntityWorldTest, Hurt_VoidDamage_SetsMarkHurtBeforeDiscard)
{
    // 即使是虚空伤害导致立即销毁，markHurt() 也应被调用
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    auto voidDamage = DamageSources::outOfWorld();
    EXPECT_TRUE(entity.hurt(voidDamage, 100.0f));
    EXPECT_TRUE(entity.isHurtMarked());
}

// ============================================================================
// NBT Health 字段序列化/反序列化往返测试
// ============================================================================

class ItemEntityNbtTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        ItemTags::initialize();
    }
};

TEST_F(ItemEntityNbtTest, Health_DefaultRoundTrip)
{
    // 默认生命值 5 的往返序列化
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 序列化
    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证序列化的 Health 字段
    auto healthVal = tryGetShort(tag, HEALTH);
    ASSERT_TRUE(healthVal.has_value());
    EXPECT_EQ(healthVal.value(), static_cast<i16>(5));

    // 反序列化到新实体
    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), 5);
}

TEST_F(ItemEntityNbtTest, Health_CustomValueRoundTrip)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setHealth(3);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    auto healthVal = tryGetShort(tag, HEALTH);
    ASSERT_TRUE(healthVal.has_value());
    EXPECT_EQ(healthVal.value(), static_cast<i16>(3));

    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), 3);
}

TEST_F(ItemEntityNbtTest, Health_ZeroValueRoundTrip)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setHealth(0);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    auto healthVal = tryGetShort(tag, HEALTH);
    ASSERT_TRUE(healthVal.has_value());
    EXPECT_EQ(healthVal.value(), static_cast<i16>(0));

    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), 0);
}

TEST_F(ItemEntityNbtTest, Health_NegativeValueRoundTrip)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setHealth(-1);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    auto healthVal = tryGetShort(tag, HEALTH);
    ASSERT_TRUE(healthVal.has_value());
    EXPECT_EQ(healthVal.value(), static_cast<i16>(-1));

    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), -1);
}

TEST_F(ItemEntityNbtTest, Health_MissingKeyInNbtPreservesDefault)
{
    // 空 NBT 标签不应改变默认生命值
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.getHealth(), 5);

    nbt::tags::compound_tag emptyTag;
    auto result = entity.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(entity.getHealth(), 5); // 默认值应保持不变
}

TEST_F(ItemEntityNbtTest, Health_AfterDamageRoundTrip)
{
    // 模拟受伤后的序列化场景
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    // 受伤后生命值：5 - 3 = 2
    entity.setHealth(2);

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), 2);
}

TEST_F(ItemEntityNbtTest, AgeAndPickupDelayRoundTrip)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setHealth(4);
    entity.setPickupDelay(20);
    // Age 通过 tick() 增长，这里直接通过 NBT 设置来测试

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证 Age 和 PickupDelay 也被正确序列化
    auto ageVal = tryGetInt(tag, AGE);
    EXPECT_TRUE(ageVal.has_value());

    auto pickupDelayVal = tryGetInt(tag, PICKUP_DELAY);
    ASSERT_TRUE(pickupDelayVal.has_value());
    EXPECT_EQ(pickupDelayVal.value(), 20);

    // 反序列化
    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.getHealth(), 4);
    EXPECT_EQ(loaded.getPickupDelay(), 20);
}

TEST_F(ItemEntityNbtTest, OwnerAndThrowerRoundTrip)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setOwner("player-uuid-123", "thrower-uuid-456");

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证 Owner 和 Thrower 被序列化
    auto ownerVal = tryGetString(tag, OWNER);
    ASSERT_TRUE(ownerVal.has_value());
    EXPECT_EQ(ownerVal.value(), "player-uuid-123");

    auto throwerVal = tryGetString(tag, THROWER);
    ASSERT_TRUE(throwerVal.has_value());
    EXPECT_EQ(throwerVal.value(), "thrower-uuid-456");

    // 反序列化
    ItemEntity loaded(EntityId(2), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(loaded.ownerUuid(), "player-uuid-123");
    EXPECT_EQ(loaded.throwerUuid(), "thrower-uuid-456");
}
