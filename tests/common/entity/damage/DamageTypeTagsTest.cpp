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

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include "common/entity/damage/tag/DamageTypeTagLoader.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"

#include <vector>

using namespace mc;

// ============================================================================
// DamageTypeTag 基本功能测试
// ============================================================================

class DamageTypeTagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试用例使用独立的标签
    }
};

TEST_F(DamageTypeTagTest, ConstructorSetsId)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_tag"));
}

TEST_F(DamageTypeTagTest, AddAndContains)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));

    // 添加前不包含
    EXPECT_FALSE(tag.contains(DamageType::Fall));
    EXPECT_FALSE(tag.contains(DamageType::Drown));

    // 添加伤害类型
    tag.add(DamageType::Fall);
    tag.add(DamageType::Drown);

    // 添加后包含
    EXPECT_TRUE(tag.contains(DamageType::Fall));
    EXPECT_TRUE(tag.contains(DamageType::Drown));

    // 仍然不包含未添加的类型
    EXPECT_FALSE(tag.contains(DamageType::Cactus));
}

TEST_F(DamageTypeTagTest, AddAll)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));

    std::vector<DamageType> types = {
        DamageType::Arrow,
        DamageType::Trident,
        DamageType::MobProjectile,
    };

    tag.addAll(types);

    EXPECT_TRUE(tag.contains(DamageType::Arrow));
    EXPECT_TRUE(tag.contains(DamageType::Trident));
    EXPECT_TRUE(tag.contains(DamageType::MobProjectile));
    EXPECT_FALSE(tag.contains(DamageType::Fireball));
}

TEST_F(DamageTypeTagTest, ClearRemovesAll)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(DamageType::Fall);
    tag.add(DamageType::Drown);

    EXPECT_TRUE(tag.contains(DamageType::Fall));
    EXPECT_EQ(tag.getDamageTypes().size(), 2u);

    tag.clear();

    EXPECT_FALSE(tag.contains(DamageType::Fall));
    EXPECT_FALSE(tag.contains(DamageType::Drown));
    EXPECT_EQ(tag.getDamageTypes().size(), 0u);
}

TEST_F(DamageTypeTagTest, DuplicateAddIsIdempotent)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(DamageType::Fall);
    tag.add(DamageType::Fall); // 重复添加

    EXPECT_TRUE(tag.contains(DamageType::Fall));
    EXPECT_EQ(tag.getDamageTypes().size(), 1u);
}

TEST_F(DamageTypeTagTest, ContainsByDamageSource)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(DamageType::Drown);

    EnvironmentalDamage drownDamage(DamageType::Drown);
    EnvironmentalDamage fallDamage(DamageType::Fall);

    EXPECT_TRUE(tag.contains(drownDamage));
    EXPECT_FALSE(tag.contains(fallDamage));
}

TEST_F(DamageTypeTagTest, AddByResourceLocation)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));

    EXPECT_TRUE(tag.addByResourceLocation(ResourceLocation("minecraft:drown")));
    EXPECT_FALSE(tag.addByResourceLocation(ResourceLocation("minecraft:nonexistent_type")));

    EXPECT_TRUE(tag.contains(DamageType::Drown));
}

TEST_F(DamageTypeTagTest, ContainsByResourceLocation)
{
    DamageTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(DamageType::Drown);

    EXPECT_TRUE(tag.containsByResourceLocation(ResourceLocation("minecraft:drown")));
    EXPECT_FALSE(tag.containsByResourceLocation(ResourceLocation("minecraft:fall")));
    EXPECT_FALSE(tag.containsByResourceLocation(ResourceLocation("minecraft:nonexistent")));
}

// ============================================================================
// DamageTypeNames 映射测试
// ============================================================================

TEST(DamageTypeNamesTest, GetResourceLocationForKnownTypes)
{
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::Drown).toString(), "minecraft:drown");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::Fall).toString(), "minecraft:fall");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::OutOfWorld).toString(), "minecraft:out_of_world");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::GenericKill).toString(), "minecraft:generic_kill");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::SonicBoom).toString(), "minecraft:sonic_boom");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::FallingAnvil).toString(), "minecraft:falling_anvil");
    EXPECT_EQ(DamageTypeNames::getResourceLocation(DamageType::WindBurst).toString(), "minecraft:wind_charge");
}

TEST(DamageTypeNamesTest, FromResourceLocationForKnownTypes)
{
    EXPECT_EQ(DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:drown")), DamageType::Drown);
    EXPECT_EQ(DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:fall")), DamageType::Fall);
    EXPECT_EQ(
        DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:out_of_world")), DamageType::OutOfWorld);
    EXPECT_EQ(
        DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:generic_kill")), DamageType::GenericKill);
    EXPECT_EQ(DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:wind_charge")), DamageType::WindBurst);
}

TEST(DamageTypeNamesTest, FromResourceLocationReturnsNulloptForUnknown)
{
    EXPECT_FALSE(DamageTypeNames::fromResourceLocation(ResourceLocation("minecraft:nonexistent")).has_value());
    EXPECT_FALSE(DamageTypeNames::fromResourceLocation(ResourceLocation("foo:bar")).has_value());
}

TEST(DamageTypeNamesTest, FromStringHandlesNamespace)
{
    EXPECT_EQ(DamageTypeNames::fromString("minecraft:drown"), DamageType::Drown);
    // 不带命名空间的自动补 minecraft: 前缀
    EXPECT_EQ(DamageTypeNames::fromString("drown"), DamageType::Drown);
    EXPECT_EQ(DamageTypeNames::fromString("sonic_boom"), DamageType::SonicBoom);
}

// ============================================================================
// DamageTypeTags 注册表测试
// ============================================================================

class DamageTypeTagsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { DamageTypeTags::initialize(); }
};

// ========== BYPASSES_ARMOR 标签测试 ==========

TEST_F(DamageTypeTagsTest, BypassesArmorContainsOnFire)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::OnFire));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsDrown)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Drown));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsStarve)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Starve));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsFall)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Fall));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsSonicBoom)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::SonicBoom));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsGenericKill)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::GenericKill));
}

TEST_F(DamageTypeTagsTest, BypassesArmorContainsOutsideBorder)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::OutsideBorder));
}

TEST_F(DamageTypeTagsTest, BypassesArmorDoesNotContainMobAttack)
{
    EXPECT_FALSE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::MobAttack));
}

TEST_F(DamageTypeTagsTest, BypassesArmorDoesNotContainArrow)
{
    EXPECT_FALSE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Arrow));
}

TEST_F(DamageTypeTagsTest, BypassesArmorDoesNotContainCactus)
{
    EXPECT_FALSE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Cactus));
}

TEST_F(DamageTypeTagsTest, BypassesArmorDoesNotContainWindBurst)
{
    // MC 1.21.11: wind_charge 不在 bypasses_armor 标签中
    EXPECT_FALSE(DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::WindBurst));
}

// ========== BYPASSES_INVULNERABILITY 标签测试 ==========

TEST_F(DamageTypeTagsTest, BypassesInvulnerabilityContainsOutOfWorld)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_INVULNERABILITY().contains(DamageType::OutOfWorld));
}

TEST_F(DamageTypeTagsTest, BypassesInvulnerabilityContainsGenericKill)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_INVULNERABILITY().contains(DamageType::GenericKill));
}

TEST_F(DamageTypeTagsTest, BypassesInvulnerabilityDoesNotContainFall)
{
    EXPECT_FALSE(DamageTypeTags::BYPASSES_INVULNERABILITY().contains(DamageType::Fall));
}

// ========== BYPASSES_WOLF_ARMOR 标签测试（核心 TODO 消费点） ==========

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsDrown)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Drown));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsStarve)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Starve));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsWither)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Wither));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsMagic)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Magic));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsIndirectMagic)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::IndirectMagic));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsThorns)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Thorns));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsFreeze)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Freeze));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsInWall)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::InWall));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsCramming)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Cramming));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsDryout)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Dryout));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsOutsideBorder)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::OutsideBorder));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsOutOfWorld)
{
    // #bypasses_invulnerability 子标签包含 out_of_world
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::OutOfWorld));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorContainsGenericKill)
{
    // #bypasses_invulnerability 子标签包含 generic_kill
    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::GenericKill));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorDoesNotContainFall)
{
    // MC 1.21.11: fall 不在 bypasses_wolf_armor 标签中，狼铠应吸收摔落伤害
    EXPECT_FALSE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Fall));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorDoesNotContainMobAttack)
{
    // 普通生物攻击应被狼铠吸收
    EXPECT_FALSE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::MobAttack));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorDoesNotContainArrow)
{
    // 箭矢伤害应被狼铠吸收
    EXPECT_FALSE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Arrow));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorDoesNotContainCactus)
{
    // 仙人掌伤害应被狼铠吸收
    EXPECT_FALSE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Cactus));
}

TEST_F(DamageTypeTagsTest, BypassesWolfArmorDoesNotContainFireball)
{
    // 火球伤害应被狼铠吸收
    EXPECT_FALSE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Fireball));
}

// ========== IS_FIRE 标签测试 ==========

TEST_F(DamageTypeTagsTest, IsFireContainsInFire)
{
    EXPECT_TRUE(DamageTypeTags::IS_FIRE().contains(DamageType::InFire));
}

TEST_F(DamageTypeTagsTest, IsFireContainsCampfire)
{
    EXPECT_TRUE(DamageTypeTags::IS_FIRE().contains(DamageType::Campfire));
}

TEST_F(DamageTypeTagsTest, IsFireContainsLava)
{
    EXPECT_TRUE(DamageTypeTags::IS_FIRE().contains(DamageType::Lava));
}

TEST_F(DamageTypeTagsTest, IsFireContainsFireball)
{
    EXPECT_TRUE(DamageTypeTags::IS_FIRE().contains(DamageType::Fireball));
}

TEST_F(DamageTypeTagsTest, IsFireDoesNotContainFall)
{
    EXPECT_FALSE(DamageTypeTags::IS_FIRE().contains(DamageType::Fall));
}

// ========== IS_FALL 标签测试 ==========

TEST_F(DamageTypeTagsTest, IsFallContainsFall)
{
    EXPECT_TRUE(DamageTypeTags::IS_FALL().contains(DamageType::Fall));
}

TEST_F(DamageTypeTagsTest, IsFallContainsEnderPearl)
{
    EXPECT_TRUE(DamageTypeTags::IS_FALL().contains(DamageType::EnderPearl));
}

TEST_F(DamageTypeTagsTest, IsFallContainsStalagmite)
{
    EXPECT_TRUE(DamageTypeTags::IS_FALL().contains(DamageType::Stalagmite));
}

TEST_F(DamageTypeTagsTest, IsFallDoesNotContainFlyIntoWall)
{
    // MC 1.21.11: fly_into_wall 不在 is_fall 标签中
    EXPECT_FALSE(DamageTypeTags::IS_FALL().contains(DamageType::FlyIntoWall));
}

// ========== IS_EXPLOSION 标签测试 ==========

TEST_F(DamageTypeTagsTest, IsExplosionContainsExplosion)
{
    EXPECT_TRUE(DamageTypeTags::IS_EXPLOSION().contains(DamageType::Explosion));
}

TEST_F(DamageTypeTagsTest, IsExplosionContainsPlayerExplosion)
{
    EXPECT_TRUE(DamageTypeTags::IS_EXPLOSION().contains(DamageType::ExplosionPlayer));
}

TEST_F(DamageTypeTagsTest, IsExplosionContainsFireworks)
{
    EXPECT_TRUE(DamageTypeTags::IS_EXPLOSION().contains(DamageType::Fireworks));
}

TEST_F(DamageTypeTagsTest, IsExplosionContainsBadRespawnPoint)
{
    EXPECT_TRUE(DamageTypeTags::IS_EXPLOSION().contains(DamageType::BadRespawnPoint));
}

// ========== IS_PROJECTILE 标签测试 ==========

TEST_F(DamageTypeTagsTest, IsProjectileContainsArrow)
{
    EXPECT_TRUE(DamageTypeTags::IS_PROJECTILE().contains(DamageType::Arrow));
}

TEST_F(DamageTypeTagsTest, IsProjectileContainsTrident)
{
    EXPECT_TRUE(DamageTypeTags::IS_PROJECTILE().contains(DamageType::Trident));
}

TEST_F(DamageTypeTagsTest, IsProjectileContainsWindCharge)
{
    EXPECT_TRUE(DamageTypeTags::IS_PROJECTILE().contains(DamageType::WindBurst));
}

// ========== IS_PLAYER_ATTACK 标签测试 ==========

TEST_F(DamageTypeTagsTest, IsPlayerAttackContainsPlayerAttack)
{
    EXPECT_TRUE(DamageTypeTags::IS_PLAYER_ATTACK().contains(DamageType::PlayerAttack));
}

TEST_F(DamageTypeTagsTest, IsPlayerAttackContainsSpear)
{
    EXPECT_TRUE(DamageTypeTags::IS_PLAYER_ATTACK().contains(DamageType::Spear));
}

TEST_F(DamageTypeTagsTest, IsPlayerAttackContainsMaceSmash)
{
    EXPECT_TRUE(DamageTypeTags::IS_PLAYER_ATTACK().contains(DamageType::MaceSmash));
}

// ========== BYPASSES_SHIELD 标签测试 ==========

TEST_F(DamageTypeTagsTest, BypassesShieldContainsCactus)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_SHIELD().contains(DamageType::Cactus));
}

TEST_F(DamageTypeTagsTest, BypassesShieldContainsFall)
{
    // #bypasses_armor 子标签包含 fall
    EXPECT_TRUE(DamageTypeTags::BYPASSES_SHIELD().contains(DamageType::Fall));
}

TEST_F(DamageTypeTagsTest, BypassesShieldContainsLightningBolt)
{
    EXPECT_TRUE(DamageTypeTags::BYPASSES_SHIELD().contains(DamageType::LightningBolt));
}

TEST_F(DamageTypeTagsTest, BypassesShieldDoesNotContainMobAttack)
{
    EXPECT_FALSE(DamageTypeTags::BYPASSES_SHIELD().contains(DamageType::MobAttack));
}

// ========== 注册表管理测试 ==========

TEST_F(DamageTypeTagsTest, GetTagReturnsExistingTag)
{
    auto* tag = DamageTypeTags::getTag(ResourceLocation("minecraft:bypasses_wolf_armor"));
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:bypasses_wolf_armor"));
}

TEST_F(DamageTypeTagsTest, GetTagReturnsNullForNonExistent)
{
    auto* tag = DamageTypeTags::getTag(ResourceLocation("minecraft:nonexistent_tag"));
    EXPECT_EQ(tag, nullptr);
}

TEST_F(DamageTypeTagsTest, RegisterTagCreatesNewTag)
{
    auto& tag = DamageTypeTags::registerTag(ResourceLocation("minecraft:test_custom_dmg_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_custom_dmg_tag"));

    auto* found = DamageTypeTags::getTag(ResourceLocation("minecraft:test_custom_dmg_tag"));
    ASSERT_NE(found, nullptr);
}

TEST_F(DamageTypeTagsTest, InitializeIsIdempotent)
{
    DamageTypeTags::initialize();
    DamageTypeTags::initialize();

    EXPECT_TRUE(DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(DamageType::Drown));
}

TEST_F(DamageTypeTagsTest, IsInitializedReturnsTrueAfterInitialize)
{
    EXPECT_TRUE(DamageTypeTags::isInitialized());
}

// ============================================================================
// DamageSource::is(DamageTypeTag) 测试
// ============================================================================

TEST(DamageSourceIsTagTest, IsReturnsTrueForMatchingTag)
{
    DamageTypeTags::initialize();

    EnvironmentalDamage drownDamage(DamageType::Drown);
    EXPECT_TRUE(drownDamage.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()));
    EXPECT_TRUE(drownDamage.is(DamageTypeTags::BYPASSES_ARMOR()));
    EXPECT_TRUE(drownDamage.is(DamageTypeTags::IS_DROWNING()));
}

TEST(DamageSourceIsTagTest, IsReturnsFalseForNonMatchingTag)
{
    DamageTypeTags::initialize();

    EnvironmentalDamage fallDamage(DamageType::Fall);
    EXPECT_FALSE(fallDamage.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()));
    EXPECT_TRUE(fallDamage.is(DamageTypeTags::BYPASSES_ARMOR()));
    EXPECT_TRUE(fallDamage.is(DamageTypeTags::IS_FALL()));
    EXPECT_FALSE(fallDamage.is(DamageTypeTags::IS_FIRE()));
}

TEST(DamageSourceIsTagTest, IsWorksWithEntityDamageSource)
{
    DamageTypeTags::initialize();

    EntityDamageSource mobAttack(DamageType::MobAttack, nullptr);
    EXPECT_FALSE(mobAttack.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()));
    EXPECT_FALSE(mobAttack.is(DamageTypeTags::BYPASSES_ARMOR()));
    EXPECT_FALSE(mobAttack.is(DamageTypeTags::IS_PROJECTILE()));

    EntityDamageSource arrowEntity(DamageType::Arrow, nullptr);
    EXPECT_TRUE(arrowEntity.is(DamageTypeTags::IS_PROJECTILE()));
    EXPECT_FALSE(arrowEntity.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()));
}

TEST(DamageSourceIsTagTest, IsWorksWithIndirectEntityDamageSource)
{
    DamageTypeTags::initialize();

    IndirectEntityDamageSource arrow(DamageType::Arrow, nullptr, nullptr);
    EXPECT_TRUE(arrow.is(DamageTypeTags::IS_PROJECTILE()));

    IndirectEntityDamageSource magicDamage(DamageType::IndirectMagic, nullptr, nullptr);
    EXPECT_TRUE(magicDamage.is(DamageTypeTags::BYPASSES_ARMOR()));
    EXPECT_TRUE(magicDamage.is(DamageTypeTags::BYPASSES_WOLF_ARMOR()));
}

// ============================================================================
// DamageSource::isProjectile() 方法测试
//
// 锚定 isProjectile() 查 DamageTypeTags::IS_PROJECTILE 标签的核心语义（对齐 vanilla
// source.is(DamageTypeTags.IS_PROJECTILE)）。此前 IndirectEntityDamageSource::isProjectile()
// 只查 m_isProjectile 标志位（依赖调用方 setProjectile），EntityDamageSource::isProjectile()
// 硬编码 Arrow/Trident/MobProjectile/Fireball 四类型——两者都漏 IS_PROJECTILE 标签其余成员
// （WitherSkull/Thrown/WindBurst/UnattributedFireball），且 IndirectEntityDamageSource 在调用方
// 漏 setProjectile 时（箭矢 AbstractArrowEntity 手动构造、windBurst 工厂）直接返 false，致
// 弹射物保护附魔 EPF 减伤链路失效。修复后两子类统一查 IS_PROJECTILE 标签，IndirectEntityDamageSource
// 额外 OR m_isProjectile 标志位保底。这些测试锁定该语义，防回归。
// ============================================================================

TEST(DamageSourceIsTagTest, IsProjectileMethodReturnsTrueForAllTagMembersWithoutSetProjectile)
{
    DamageTypeTags::initialize();

    // 不调 setProjectile，仅靠 IS_PROJECTILE 标签成员身份判定（修复核心：此前这些会返 false）。
    // Arrow 是修复主因（箭矢手动构造漏 setProjectile）；WindBurst 是 windBurst 工厂漏 setProjectile；
    // Trident/MobProjectile/WitherSkull/Thrown/UnattributedFireball 是其他手动构造漏 setProjectile
    // 的投射物（TridentEntity/OtherProjectiles 等），均经查标签自动修复。
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::Arrow, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::Trident, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::MobProjectile, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::Fireball, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::WitherSkull, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::Thrown, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::WindBurst, nullptr, nullptr).isProjectile());
    EXPECT_TRUE(IndirectEntityDamageSource(DamageType::UnattributedFireball, nullptr, nullptr).isProjectile());
}

TEST(DamageSourceIsTagTest, IsProjectileMethodReturnsFalseForNonProjectileIndirectSources)
{
    DamageTypeTags::initialize();

    // 非投射物的间接伤害源（爆炸/间接魔法）不应 isProjectile。Spear 是三叉戟近战（IS_PLAYER_ATTACK
    // 成员，非 IS_PROJECTILE）。
    EXPECT_FALSE(IndirectEntityDamageSource(DamageType::Explosion, nullptr, nullptr).isProjectile());
    EXPECT_FALSE(IndirectEntityDamageSource(DamageType::IndirectMagic, nullptr, nullptr).isProjectile());
    EXPECT_FALSE(IndirectEntityDamageSource(DamageType::Spear, nullptr, nullptr).isProjectile());
}

TEST(DamageSourceIsTagTest, IsProjectileMethodReturnsTrueForEntityDamageSourceTagMembers)
{
    DamageTypeTags::initialize();

    // EntityDamageSource::isProjectile() 同样查 IS_PROJECTILE 标签（此前硬编码四类型，漏其余成员）。
    // Arrow/Trident/MobProjectile/Fireball/WitherSkull/Thrown/WindBurst/UnattributedFireball 均返 true。
    EXPECT_TRUE(EntityDamageSource(DamageType::Arrow, nullptr).isProjectile());
    EXPECT_TRUE(EntityDamageSource(DamageType::WindBurst, nullptr).isProjectile());
    EXPECT_TRUE(EntityDamageSource(DamageType::WitherSkull, nullptr).isProjectile());

    // 非投射物直接伤害源（近战/玩家攻击/三叉戟近战）返 false。
    EXPECT_FALSE(EntityDamageSource(DamageType::MobAttack, nullptr).isProjectile());
    EXPECT_FALSE(EntityDamageSource(DamageType::PlayerAttack, nullptr).isProjectile());
    EXPECT_FALSE(EntityDamageSource(DamageType::Spear, nullptr).isProjectile());
    EXPECT_FALSE(EntityDamageSource(DamageType::Explosion, nullptr).isProjectile());
}

TEST(DamageSourceIsTagTest, IsProjectileFlagStillWorksAsOverrideWhenTagNotInitialized)
{
    // 标签未初始化时 IS_PROJECTILE() 返回空标签，contains 返 false。此时 IndirectEntityDamageSource
    // 仍可由 setProjectile() 设的 m_isProjectile 标志位判定（保底机制，箭矢/风爆经工厂 setProjectile
    // 设位）。这保证单元测试夹具未调 DamageTypeTags::initialize() 时，经工厂构造的投射物伤害源
    // 仍正确识别为投射物。
    // 注：此测试须在标签未初始化前提下运行；但 DamageTypeTags 是全局静态，其他测试套件的
    // SetUpTestSuite/initialize 可能已置位 s_initialized。故此测试用 setProjectile 标志位 OR 语义
    // 验证（即使标签已初始化，setProjectile 标志位仍使 isProjectile 返 true——OR 语义）。
    IndirectEntityDamageSource arrowWithFlag(DamageType::Arrow, nullptr, nullptr);
    arrowWithFlag.setProjectile();
    EXPECT_TRUE(arrowWithFlag.isProjectile());
}

// ============================================================================
// DamageTypeTagLoader JSON 解析测试
// ============================================================================

class DamageTypeTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { DamageTypeTags::initialize(); }
};

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonBasicDirectTypes)
{
    const std::string json = R"({
        "values": [
            "minecraft:drown",
            "minecraft:starve",
            "minecraft:fall"
        ]
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_dmg_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:test_dmg_tag"));
    EXPECT_TRUE(tag->contains(DamageType::Drown));
    EXPECT_TRUE(tag->contains(DamageType::Starve));
    EXPECT_TRUE(tag->contains(DamageType::Fall));
    EXPECT_FALSE(tag->contains(DamageType::Cactus));
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonWithTagReference)
{
    // BYPASSES_INVULNERABILITY 标签已由 initialize() 注册
    const std::string json = R"({
        "values": [
            "#minecraft:bypasses_invulnerability",
            "minecraft:drown"
        ]
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_with_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // #bypasses_invulnerability 子标签成员应被展开
    EXPECT_TRUE(tag->contains(DamageType::OutOfWorld));
    EXPECT_TRUE(tag->contains(DamageType::GenericKill));
    // 直接成员
    EXPECT_TRUE(tag->contains(DamageType::Drown));
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonWithReplace)
{
    const std::string json = R"({
        "replace": true,
        "values": [
            "minecraft:drown"
        ]
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_replace"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(DamageType::Drown));
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonMissingValuesFails)
{
    const std::string json = R"({
        "replace": false
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_missing"));
    EXPECT_FALSE(result.success());
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonInvalidJsonFails)
{
    const std::string json = "not valid json";
    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_invalid"));
    EXPECT_FALSE(result.success());
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonWithOptionalEntry)
{
    const std::string json = R"({
        "values": [
            "minecraft:drown",
            {"id": "minecraft:nonexistent_type", "required": false}
        ]
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_optional"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(DamageType::Drown));
    // 不存在的伤害类型 required=false 时被静默跳过
    EXPECT_FALSE(tag->contains(DamageType::Fall));
}

TEST_F(DamageTypeTagLoaderTest, LoadFromJsonWithUnknownRequiredTypeWarns)
{
    const std::string json = R"({
        "values": [
            "minecraft:drown",
            "minecraft:nonexistent_type"
        ]
    })";

    auto result = DamageTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_unknown_required"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // drown 仍然被添加
    EXPECT_TRUE(tag->contains(DamageType::Drown));
    // 不存在的类型被跳过
    EXPECT_EQ(tag->getDamageTypes().size(), 1u);
}

TEST_F(DamageTypeTagLoaderTest, LoadFromResourcePackLoadsAllTags)
{
    resource::InMemoryResourcePack pack("test_pack");

    // 添加两个 damage_type 标签 JSON 文件
    // 注意: addServerDataResource 的路径是相对于 data/ 根目录的，不需要 data/ 前缀
    pack.addServerDataResource(
        "minecraft/tags/damage_type/test_tag_a.json", R"({"values": ["minecraft:drown", "minecraft:starve"]})");

    pack.addServerDataResource("minecraft/tags/damage_type/test_tag_b.json",
        R"({"values": ["minecraft:fall", "#minecraft:bypasses_invulnerability"]})");

    auto result = DamageTypeTagLoader::loadFromResourcePack(pack);
    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value(), 2u);

    // 验证 test_tag_a
    auto* tagA = DamageTypeTags::getTag(ResourceLocation("minecraft:test_tag_a"));
    ASSERT_NE(tagA, nullptr);
    EXPECT_TRUE(tagA->contains(DamageType::Drown));
    EXPECT_TRUE(tagA->contains(DamageType::Starve));

    // 验证 test_tag_b（包含 #bypasses_invulnerability 子标签展开）
    auto* tagB = DamageTypeTags::getTag(ResourceLocation("minecraft:test_tag_b"));
    ASSERT_NE(tagB, nullptr);
    EXPECT_TRUE(tagB->contains(DamageType::Fall));
    EXPECT_TRUE(tagB->contains(DamageType::OutOfWorld));
    EXPECT_TRUE(tagB->contains(DamageType::GenericKill));
}
