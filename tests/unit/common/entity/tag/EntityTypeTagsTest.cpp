/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/entity/tag/EntityTypeTagLoader.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;

// ============================================================================
// EntityTypeTag 基本功能测试
// ============================================================================

class EntityTypeTagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试用例使用独立的标签
    }
};

TEST_F(EntityTypeTagTest, ConstructorSetsId)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_tag"));
}

TEST_F(EntityTypeTagTest, AddAndContains)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test"));

    // 添加前不包含
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:zombie")));

    // 添加实体类型
    tag.add(ResourceLocation("minecraft:arrow"));
    tag.add(ResourceLocation("minecraft:zombie"));

    // 添加后包含
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:zombie")));

    // 仍然不包含未添加的类型
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:skeleton")));
}

TEST_F(EntityTypeTagTest, AddAll)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test"));

    std::vector<ResourceLocation> ids = {
        ResourceLocation("minecraft:arrow"),
        ResourceLocation("minecraft:spectral_arrow"),
        ResourceLocation("minecraft:trident"),
    };

    tag.addAll(ids);

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:spectral_arrow")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:trident")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:snowball")));
}

TEST_F(EntityTypeTagTest, ClearRemovesAll)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:arrow"));
    tag.add(ResourceLocation("minecraft:zombie"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_EQ(tag.getEntityTypeIds().size(), 2u);

    tag.clear();

    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:zombie")));
    EXPECT_EQ(tag.getEntityTypeIds().size(), 0u);
}

TEST_F(EntityTypeTagTest, ContainsByString)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:arrow"));

    EXPECT_TRUE(tag.contains("minecraft:arrow"));
    EXPECT_FALSE(tag.contains("minecraft:zombie"));
}

TEST_F(EntityTypeTagTest, DuplicateAddIsIdempotent)
{
    EntityTypeTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:arrow"));
    tag.add(ResourceLocation("minecraft:arrow")); // 重复添加

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_EQ(tag.getEntityTypeIds().size(), 1u);
}

// ============================================================================
// EntityTypeTags 注册表测试
// ============================================================================

class EntityTypeTagsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化实体类型和实体类型标签
        entity::VanillaEntities::registerAll();
        EntityTypeTags::initialize();
    }
};

TEST_F(EntityTypeTagsTest, ImpactProjectilesContainsArrow)
{
    // IMPACT_PROJECTILES 标签应包含 arrow
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:arrow")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesContainsTrident)
{
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:trident")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesContainsSnowball)
{
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:snowball")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesContainsEgg)
{
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:egg")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesContainsFireball)
{
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:fireball")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesDoesNotContainEnderPearl)
{
    // 末影珍珠不在 IMPACT_PROJECTILES 中
    EXPECT_FALSE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:ender_pearl")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesDoesNotContainPotion)
{
    // 药水不在 IMPACT_PROJECTILES 中
    EXPECT_FALSE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:potion")));
}

TEST_F(EntityTypeTagsTest, ImpactProjectilesDoesNotContainFishingBobber)
{
    // 钓鱼浮标不在 IMPACT_PROJECTILES 中
    EXPECT_FALSE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:fishing_bobber")));
}

TEST_F(EntityTypeTagsTest, ArrowsContainsArrowAndSpectralArrow)
{
    EXPECT_TRUE(EntityTypeTags::ARROWS().contains(ResourceLocation("minecraft:arrow")));
    EXPECT_TRUE(EntityTypeTags::ARROWS().contains(ResourceLocation("minecraft:spectral_arrow")));
}

TEST_F(EntityTypeTagsTest, UndeadContainsSkeletonAndZombie)
{
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:skeleton")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:zombie")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:wither")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:phantom")));
}

// 对齐 vanilla 1.21.11 EntityTypeTagsProvider：UNDEAD（= SKELETONS + ZOMBIES + wither + phantom）
// 应包含 ZOMBIES 子标签的全部成员，含 zombie_horse 与 zombie_nautilus。
TEST_F(EntityTypeTagsTest, UndeadContainsZombieHorseAndZombieNautilus)
{
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:zombie_horse")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:zombie_nautilus")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:zombified_piglin")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:zoglin")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:skeleton_horse")));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(ResourceLocation("minecraft:bogged")));
    // 亡灵杀手敏感标签应同步包含新增成员（addAll UNDEAD 派生）
    EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_SMITE().contains(ResourceLocation("minecraft:zombie_horse")));
    EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_SMITE().contains(ResourceLocation("minecraft:zombie_nautilus")));
}

TEST_F(EntityTypeTagsTest, ArthropodContainsBeeAndSpider)
{
    EXPECT_TRUE(EntityTypeTags::ARTHROPOD().contains(ResourceLocation("minecraft:bee")));
    EXPECT_TRUE(EntityTypeTags::ARTHROPOD().contains(ResourceLocation("minecraft:spider")));
    EXPECT_TRUE(EntityTypeTags::ARTHROPOD().contains(ResourceLocation("minecraft:cave_spider")));
}

// 对齐 vanilla 1.21.11 EntityTypeTagsProvider：AQUATIC 应包含 nautilus 与 zombie_nautilus，
// SENSITIVE_TO_IMPALING（= AQUATIC）同步派生，使穿刺附魔对二者额外伤害。
TEST_F(EntityTypeTagsTest, AquaticContainsNautilusAndZombieNautilus)
{
    EXPECT_TRUE(EntityTypeTags::AQUATIC().contains(ResourceLocation("minecraft:nautilus")));
    EXPECT_TRUE(EntityTypeTags::AQUATIC().contains(ResourceLocation("minecraft:zombie_nautilus")));
    EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_IMPALING().contains(ResourceLocation("minecraft:nautilus")));
    EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_IMPALING().contains(ResourceLocation("minecraft:zombie_nautilus")));
    // 已有水生成员不被破坏
    EXPECT_TRUE(EntityTypeTags::AQUATIC().contains(ResourceLocation("minecraft:squid")));
    EXPECT_TRUE(EntityTypeTags::AQUATIC().contains(ResourceLocation("minecraft:turtle")));
}

TEST_F(EntityTypeTagsTest, RaiderContainsEvokerAndPillager)
{
    EXPECT_TRUE(EntityTypeTags::RAIDERS().contains(ResourceLocation("minecraft:evoker")));
    EXPECT_TRUE(EntityTypeTags::RAIDERS().contains(ResourceLocation("minecraft:pillager")));
    EXPECT_TRUE(EntityTypeTags::RAIDERS().contains(ResourceLocation("minecraft:witch")));
}

TEST_F(EntityTypeTagsTest, SensitiveToSmiteEqualsUndead)
{
    // SENSITIVE_TO_SMITE 应包含亡灵标签的所有成员
    for (const auto& id : EntityTypeTags::UNDEAD().getEntityTypeIds()) {
        EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_SMITE().contains(id)) << "SENSITIVE_TO_SMITE 应包含 " << id.toString();
    }
}

TEST_F(EntityTypeTagsTest, SensitiveToBaneOfArthropodsEqualsArthropod)
{
    for (const auto& id : EntityTypeTags::ARTHROPOD().getEntityTypeIds()) {
        EXPECT_TRUE(EntityTypeTags::SENSITIVE_TO_BANE_OF_ARTHROPODS().contains(id))
            << "SENSITIVE_TO_BANE_OF_ARTHROPODS 应包含 " << id.toString();
    }
}

// ============================================================================
// 对齐 vanilla 1.21.11 EntityTypeTagsProvider 的标签成员守护测试
// 以下测试固化本次对齐修正（DEFLECTS_PROJECTILES/IGNORES_POISON_AND_REGEN/
// BURN_IN_DAYLIGHT/CAN_BREATHE_UNDER_WATER/FALL_DAMAGE_IMMUNE/IMMUNE_TO_INFESTED/
// IMMUNE_TO_OOZING/POWDER_SNOW_WALKABLE_MOBS/ACCEPTS_IRON_GOLEM_GIFT），防止回归。
// ============================================================================

// DEFLECTS_PROJECTILES 对齐 vanilla（仅 breeze）。此前误加 shulker 已移除——
// 运行时 Entity::deflection 查本标签，误加 shulker 致潜影贝偏转投射物偏离 vanilla。
TEST_F(EntityTypeTagsTest, DeflectsProjectilesContainsOnlyBreeze)
{
    EXPECT_TRUE(EntityTypeTags::DEFLECTS_PROJECTILES().contains(ResourceLocation("minecraft:breeze")));
    // shulker 在 vanilla 不偏转投射物，不应在本标签中
    EXPECT_FALSE(EntityTypeTags::DEFLECTS_PROJECTILES().contains(ResourceLocation("minecraft:shulker")));
}

// IGNORES_POISON_AND_REGEN 对齐 vanilla（= addTag(UNDEAD)）。此前误加 iron_golem——
// vanilla 铁傀儡受中毒，Cubium 误加致免疫偏差。改为 UNDEAD 派生并移除 iron_golem。
TEST_F(EntityTypeTagsTest, IgnoresPoisonAndRegenEqualsUndeadAndExcludesIronGolem)
{
    // iron_golem 不在 vanilla IGNORES_POISON_AND_REGEN 中（铁傀儡受中毒）
    EXPECT_FALSE(EntityTypeTags::IGNORES_POISON_AND_REGEN().contains(ResourceLocation("minecraft:iron_golem")));
    // 全部亡灵成员均应在标签内（= UNDEAD 派生）
    for (const auto& id : EntityTypeTags::UNDEAD().getEntityTypeIds()) {
        EXPECT_TRUE(EntityTypeTags::IGNORES_POISON_AND_REGEN().contains(id))
            << "IGNORES_POISON_AND_REGEN 应包含亡灵成员 " << id.toString();
    }
    // INVERTED_HEALING_AND_HARM 同样 = UNDEAD 派生
    for (const auto& id : EntityTypeTags::UNDEAD().getEntityTypeIds()) {
        EXPECT_TRUE(EntityTypeTags::INVERTED_HEALING_AND_HARM().contains(id))
            << "INVERTED_HEALING_AND_HARM 应包含亡灵成员 " << id.toString();
    }
}

// BURN_IN_DAYLIGHT 对齐 vanilla：含 zombie_horse/zombie_nautilus，不含 husk（husk 不燃）。
TEST_F(EntityTypeTagsTest, BurnInDaylightContainsZombieHorseAndNautilusExcludesHusk)
{
    EXPECT_TRUE(EntityTypeTags::BURN_IN_DAYLIGHT().contains(ResourceLocation("minecraft:zombie_horse")));
    EXPECT_TRUE(EntityTypeTags::BURN_IN_DAYLIGHT().contains(ResourceLocation("minecraft:zombie_nautilus")));
    EXPECT_TRUE(EntityTypeTags::BURN_IN_DAYLIGHT().contains(ResourceLocation("minecraft:skeleton")));
    // husk 在 vanilla 不在 BURN_IN_DAYLIGHT 标签（尸壳日光下不燃烧）
    EXPECT_FALSE(EntityTypeTags::BURN_IN_DAYLIGHT().contains(ResourceLocation("minecraft:husk")));
}

// CAN_BREATHE_UNDER_WATER 对齐 vanilla：含 fish/armor_stand/copper_golem/nautilus/frog/tadpole，
// 不含 dolphin（vanilla 海豚需浮出水面呼吸）。
TEST_F(EntityTypeTagsTest, CanBreatheUnderWaterContainsAquaticNonDolphinExcludesDolphin)
{
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:cod")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:salmon")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:armor_stand")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:copper_golem")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:nautilus")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:frog")));
    EXPECT_TRUE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:tadpole")));
    // dolphin 在 vanilla 不在 CAN_BREATHE_UNDER_WATER 标签（海豚需浮出水面呼吸）
    EXPECT_FALSE(EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(ResourceLocation("minecraft:dolphin")));
}

// FALL_DAMAGE_IMMUNE 对齐 vanilla：含 copper_golem/magma_cube/ocelot/breeze/allay/happy_ghast。
TEST_F(EntityTypeTagsTest, FallDamageImmuneContainsCopperGolemMagmaCubeBreezeEtc)
{
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:copper_golem")));
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:magma_cube")));
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:ocelot")));
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:breeze")));
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:allay")));
    EXPECT_TRUE(EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(ResourceLocation("minecraft:happy_ghast")));
}

// IMMUNE_TO_INFESTED 对齐 vanilla（仅 silverfish）。此前误加 spider/cave_spider/endermite。
TEST_F(EntityTypeTagsTest, ImmuneToInfestedContainsOnlySilverfish)
{
    EXPECT_TRUE(EntityTypeTags::IMMUNE_TO_INFESTED().contains(ResourceLocation("minecraft:silverfish")));
    EXPECT_FALSE(EntityTypeTags::IMMUNE_TO_INFESTED().contains(ResourceLocation("minecraft:spider")));
    EXPECT_FALSE(EntityTypeTags::IMMUNE_TO_INFESTED().contains(ResourceLocation("minecraft:cave_spider")));
    EXPECT_FALSE(EntityTypeTags::IMMUNE_TO_INFESTED().contains(ResourceLocation("minecraft:endermite")));
}

// IMMUNE_TO_OOZING 对齐 vanilla（仅 slime）。此前误加 magma_cube。
TEST_F(EntityTypeTagsTest, ImmuneToOozingContainsOnlySlime)
{
    EXPECT_TRUE(EntityTypeTags::IMMUNE_TO_OOZING().contains(ResourceLocation("minecraft:slime")));
    EXPECT_FALSE(EntityTypeTags::IMMUNE_TO_OOZING().contains(ResourceLocation("minecraft:magma_cube")));
}

// POWDER_SNOW_WALKABLE_MOBS 对齐 vanilla：rabbit/endermite/silverfish/fox。
// 此前误为 rabbit/fox/ocelot/cat（漏 endermite/silverfish，多 ocelot/cat）。
TEST_F(EntityTypeTagsTest, PowderSnowWalkableMobsContainsRabbitEndermiteSilverfishFox)
{
    EXPECT_TRUE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:rabbit")));
    EXPECT_TRUE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:endermite")));
    EXPECT_TRUE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:silverfish")));
    EXPECT_TRUE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:fox")));
    EXPECT_FALSE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:ocelot")));
    EXPECT_FALSE(EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(ResourceLocation("minecraft:cat")));
}

// NO_ANGER_FROM_WIND_CHARGE 对齐 vanilla（9 成员）。
TEST_F(EntityTypeTagsTest, NoAngerFromWindChargeContainsNineMembers)
{
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:breeze")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:skeleton")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:bogged")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:stray")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:zombie")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:husk")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:spider")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:cave_spider")));
    EXPECT_TRUE(EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(ResourceLocation("minecraft:slime")));
}

// ACCEPTS/CANDIDATE_FOR_IRON_GOLEM_GIFT 对齐 vanilla：此前 initialize() 未填充（空标签）致
// 铁傀儡赠花链路失效，单元测试 SetUpTestSuite 手动 addAll 掩盖生产缺陷。现 initialize() 填充。
TEST_F(EntityTypeTagsTest, IronGolemGiftTagsContainCopperGolemAndVillager)
{
    EXPECT_TRUE(EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT().contains(ResourceLocation("minecraft:copper_golem")));
    EXPECT_TRUE(EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().contains(ResourceLocation("minecraft:villager")));
    EXPECT_TRUE(EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().contains(ResourceLocation("minecraft:copper_golem")));
}

TEST_F(EntityTypeTagsTest, GetTagReturnsExistingTag)
{
    auto* tag = EntityTypeTags::getTag(ResourceLocation("minecraft:impact_projectiles"));
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:impact_projectiles"));
}

TEST_F(EntityTypeTagsTest, GetTagReturnsNullForNonExistent)
{
    auto* tag = EntityTypeTags::getTag(ResourceLocation("minecraft:nonexistent_tag"));
    EXPECT_EQ(tag, nullptr);
}

TEST_F(EntityTypeTagsTest, RegisterTagCreatesNewTag)
{
    auto& tag = EntityTypeTags::registerTag(ResourceLocation("minecraft:test_custom_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_custom_tag"));

    // 注册后应能通过 getTag 查找
    auto* found = EntityTypeTags::getTag(ResourceLocation("minecraft:test_custom_tag"));
    ASSERT_NE(found, nullptr);
}

TEST_F(EntityTypeTagsTest, InitializeIsIdempotent)
{
    // 重复调用 initialize 不应崩溃
    EntityTypeTags::initialize();
    EntityTypeTags::initialize();

    // 验证标签内容未变
    EXPECT_TRUE(EntityTypeTags::IMPACT_PROJECTILES().contains(ResourceLocation("minecraft:arrow")));
}

// ============================================================================
// EntityType::isIn 测试
// ============================================================================

TEST_F(EntityTypeTagsTest, EntityTypeIsInReturnsTrueForMatchingTag)
{
    // 查找 arrow 实体类型
    const entity::EntityType* arrowType = entity::EntityRegistry::instance().getType("minecraft:arrow");
    if (arrowType != nullptr) {
        EXPECT_TRUE(arrowType->isIn(EntityTypeTags::IMPACT_PROJECTILES()));
        EXPECT_TRUE(arrowType->isIn(EntityTypeTags::ARROWS()));
    }
}

TEST_F(EntityTypeTagsTest, EntityTypeIsInReturnsFalseForNonMatchingTag)
{
    const entity::EntityType* arrowType = entity::EntityRegistry::instance().getType("minecraft:arrow");
    if (arrowType != nullptr) {
        EXPECT_FALSE(arrowType->isIn(EntityTypeTags::UNDEAD()));
        EXPECT_FALSE(arrowType->isIn(EntityTypeTags::ARTHROPOD()));
    }
}

// ============================================================================
// EntityTypeTagLoader JSON 解析测试
// ============================================================================

class EntityTypeTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        entity::VanillaEntities::registerAll();
        EntityTypeTags::initialize();
    }
};

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonBasicDirectEntities)
{
    const std::string json = R"({
        "values": [
            "minecraft:zombie",
            "minecraft:skeleton",
            "minecraft:creeper"
        ]
    })";

    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:test_tag"));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:zombie")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:skeleton")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:creeper")));
    EXPECT_FALSE(tag->contains(ResourceLocation("minecraft:arrow")));
}

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonWithTagReference)
{
    // 先确保 ARROWS 标签已注册
    EntityTypeTags::ARROWS();

    const std::string json = R"({
        "values": [
            "#minecraft:arrows",
            "minecraft:fireball"
        ]
    })";

    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_with_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // ARROWS 标签的成员应被展开
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:arrow")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:spectral_arrow")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:fireball")));
}

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonWithReplace)
{
    const std::string json = R"({
        "replace": true,
        "values": [
            "minecraft:zombie"
        ]
    })";

    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_replace"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:zombie")));
}

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonMissingValuesFails)
{
    const std::string json = R"({
        "replace": false
    })";

    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_missing"));
    EXPECT_FALSE(result.success());
}

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonInvalidJsonFails)
{
    const std::string json = "not valid json";
    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_invalid"));
    EXPECT_FALSE(result.success());
}

TEST_F(EntityTypeTagLoaderTest, LoadFromJsonWithOptionalEntry)
{
    const std::string json = R"({
        "values": [
            "minecraft:arrow",
            {"id": "minecraft:nonexistent_entity", "required": false}
        ]
    })";

    auto result = EntityTypeTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_optional"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:arrow")));
    // 不存在的实体类型也应该被添加（EntityTypeTag 不验证实体是否存在）
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:nonexistent_entity")));
}

// ============================================================================
// ProjectileEntity::mayBreak() 和 PROJECTILES_CAN_BREAK_BLOCKS 测试
// ============================================================================

class ProjectileMayBreakTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        entity::VanillaEntities::registerAll();
        EntityTypeTags::initialize();
    }
};

TEST_F(ProjectileMayBreakTest, GameRuleDefaultIsTrue)
{
    // 默认 GameRules 应允许投射物破坏方块
    world::gamerule::GameRules rules;
    EXPECT_TRUE(rules.getBoolean(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS));
}

TEST_F(ProjectileMayBreakTest, GameRuleKeyProperties)
{
    // 验证游戏规则键属性
    EXPECT_EQ(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS.getName(), "projectilesCanBreakBlocks");
    EXPECT_EQ(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS.getCategory(),
        world::gamerule::GameRuleCategory::Drops);
}

TEST_F(ProjectileMayBreakTest, GameRuleCanBeSetFalse)
{
    world::gamerule::GameRules rules;
    rules.setBoolean(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(world::gamerule::GameRuleKeys::PROJECTILES_CAN_BREAK_BLOCKS));
}

TEST_F(ProjectileMayBreakTest, ImpactProjectilesTagMembers)
{
    // 验证 IMPACT_PROJECTILES 标签包含所有 MC Java 规定的成员
    auto& tag = EntityTypeTags::IMPACT_PROJECTILES();

    // 箭矢
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:arrow")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:spectral_arrow")));

    // 直接成员
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:firework_rocket")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:snowball")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:fireball")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:small_fireball")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:egg")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:trident")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:dragon_fireball")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:wither_skull")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:wind_charge")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:breeze_wind_charge")));

    // 非成员
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:ender_pearl")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:potion")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:experience_bottle")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:llama_spit")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:fishing_bobber")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:shulker_bullet")));
}

// ============================================================================
// DISMOUNTS_UNDERWATER 标签测试
// ============================================================================

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsHorse)
{
    // 马属于水下强制下坐骑标签
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:horse")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsPig)
{
    // 猪属于水下强制下坐骑标签
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:pig")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsStrider)
{
    // 炽足兽属于水下强制下坐骑标签
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:strider")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsSpider)
{
    // 蜘蛛属于水下强制下坐骑标签（蜘蛛骑士场景）
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:spider")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsCamel)
{
    // 骆驼属于水下强制下坐骑标签
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:camel")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterContainsAllEquines)
{
    // 所有马科动物都属于水下强制下坐骑标签
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:horse")));
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:donkey")));
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:mule")));
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:llama")));
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:trader_llama")));
    EXPECT_TRUE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:zombie_horse")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterDoesNotContainBoat)
{
    // 船不属于水下强制下坐骑标签（船有自己的水下沉没逻辑）
    EXPECT_FALSE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:boat")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterDoesNotContainMinecart)
{
    // 矿车不属于水下强制下坐骑标签
    EXPECT_FALSE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:minecart")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterDoesNotContainPlayer)
{
    // 玩家不属于水下强制下坐骑标签
    EXPECT_FALSE(EntityTypeTags::DISMOUNTS_UNDERWATER().contains(ResourceLocation("minecraft:player")));
}

TEST_F(EntityTypeTagsTest, DismountsUnderwaterTagId)
{
    // 验证标签ID
    EXPECT_EQ(EntityTypeTags::DISMOUNTS_UNDERWATER().getId(), ResourceLocation("minecraft:dismounts_underwater"));
}
