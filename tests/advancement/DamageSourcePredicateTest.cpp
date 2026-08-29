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

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief DamageSourcePredicate 单元测试
 *
 * 测试伤害源谓词的功能：
 * - 伤害类型标志检查
 * - JSON 解析和序列化
 * - MC 1.16.5 兼容性
 */
class DamageSourcePredicateTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

// ========== isAny 测试 ==========

TEST_F(DamageSourcePredicateTest, DefaultIsAny)
{
    DamageSourcePredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(DamageSourcePredicateTest, AnyPredicateMatchesAllSources)
{
    DamageSourcePredicate predicate;

    // 默认谓词应该匹配所有伤害类型
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_TRUE(predicate.test(fire));

    EnvironmentalDamage magic(DamageType::Magic);
    EXPECT_TRUE(predicate.test(magic));

    EnvironmentalDamage explosion(DamageType::Explosion);
    EXPECT_TRUE(predicate.test(explosion));
}

// ========== isProjectile 测试 ==========

TEST_F(DamageSourcePredicateTest, ProjectilePredicateMatches)
{
    // 解析 JSON: is_projectile = true
    nlohmann::json json = R"({"is_projectile": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    EXPECT_FALSE(predicate.isAny());
    ASSERT_TRUE(predicate.isProjectile().has_value());
    EXPECT_TRUE(predicate.isProjectile().value());

    // 注意: EnvironmentalDamage 的 isProjectile() 始终返回 false
    // 投射物伤害需要使用 EntityDamageSource，这里只测试 JSON 解析和标志存储

    // 非投射物伤害不应该匹配
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_FALSE(predicate.test(fire));
}

TEST_F(DamageSourcePredicateTest, NonProjectilePredicateMatches)
{
    // 解析 JSON: is_projectile = false
    nlohmann::json json = R"({"is_projectile": false})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 非投射物伤害应该匹配
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_TRUE(predicate.test(fire));

    // 注意: EnvironmentalDamage 的 isProjectile() 始终返回 false
    // 所以都会匹配 is_projectile = false 的谓词
}

// ========== isFire 测试 ==========

TEST_F(DamageSourcePredicateTest, FirePredicateMatches)
{
    // 解析 JSON: is_fire = true
    nlohmann::json json = R"({"is_fire": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 火焰伤害应该匹配
    EnvironmentalDamage inFire(DamageType::InFire);
    EXPECT_TRUE(predicate.test(inFire));

    EnvironmentalDamage onFire(DamageType::OnFire);
    EXPECT_TRUE(predicate.test(onFire));

    EnvironmentalDamage lava(DamageType::Lava);
    EXPECT_TRUE(predicate.test(lava));

    // 非火焰伤害不应该匹配
    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_FALSE(predicate.test(drown));
}

// ========== isMagic 测试 ==========

TEST_F(DamageSourcePredicateTest, MagicPredicateMatches)
{
    // 解析 JSON: is_magic = true
    nlohmann::json json = R"({"is_magic": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 魔法伤害应该匹配
    EnvironmentalDamage magic(DamageType::Magic);
    EXPECT_TRUE(predicate.test(magic));

    EnvironmentalDamage wither(DamageType::Wither);
    EXPECT_TRUE(predicate.test(wither));

    // 非魔法伤害不应该匹配
    EnvironmentalDamage arrow(DamageType::Arrow);
    EXPECT_FALSE(predicate.test(arrow));
}

// ========== isExplosion 测试 ==========

TEST_F(DamageSourcePredicateTest, ExplosionPredicateMatches)
{
    // 解析 JSON: is_explosion = true
    nlohmann::json json = R"({"is_explosion": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    EXPECT_FALSE(predicate.isAny());
    ASSERT_TRUE(predicate.isExplosion().has_value());
    EXPECT_TRUE(predicate.isExplosion().value());

    // 注意: EnvironmentalDamage 的 isExplosion() 始终返回 false
    // 爆炸伤害需要使用 EntityDamageSource::setExplosion()，这里只测试 JSON 解析和标志存储

    // 非爆炸伤害不应该匹配
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_FALSE(predicate.test(fire));
}

// ========== isLightning 测试 ==========

TEST_F(DamageSourcePredicateTest, LightningPredicateMatches)
{
    // 解析 JSON: is_lightning = true
    nlohmann::json json = R"({"is_lightning": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 闪电伤害应该匹配
    EnvironmentalDamage lightning(DamageType::LightningBolt);
    EXPECT_TRUE(predicate.test(lightning));

    // 非闪电伤害不应该匹配
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_FALSE(predicate.test(fire));
}

// ========== bypassesArmor 测试 ==========

TEST_F(DamageSourcePredicateTest, BypassesArmorPredicateMatches)
{
    // 解析 JSON: bypasses_armor = true
    nlohmann::json json = R"({"bypasses_armor": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 绕过护甲的伤害应该匹配
    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_TRUE(predicate.test(drown));

    EnvironmentalDamage starve(DamageType::Starve);
    EXPECT_TRUE(predicate.test(starve));

    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_TRUE(predicate.test(fall));

    // 不绕过护甲的伤害不应该匹配
    EnvironmentalDamage cactus(DamageType::Cactus);
    EXPECT_FALSE(predicate.test(cactus));
}

// ========== bypassesInvulnerability 测试 ==========

TEST_F(DamageSourcePredicateTest, BypassesInvulnerabilityPredicateMatches)
{
    // 解析 JSON: bypasses_invulnerability = true
    nlohmann::json json = R"({"bypasses_invulnerability": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 虚空伤害绕过无敌（可以伤害创造模式）
    EnvironmentalDamage outOfWorld(DamageType::OutOfWorld);
    EXPECT_TRUE(predicate.test(outOfWorld));

    // 普通伤害不绕过无敌
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_FALSE(predicate.test(fire));
}

// ========== bypassesMagic 测试 ==========

TEST_F(DamageSourcePredicateTest, BypassesMagicPredicateMatches)
{
    // 解析 JSON: bypasses_magic = true
    nlohmann::json json = R"({"bypasses_magic": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 饥饿伤害绕过魔法保护
    EnvironmentalDamage starve(DamageType::Starve);
    EXPECT_TRUE(predicate.test(starve));

    // 普通伤害不绕过魔法保护
    EnvironmentalDamage fire(DamageType::InFire);
    EXPECT_FALSE(predicate.test(fire));
}

// ========== 组合条件测试 ==========

TEST_F(DamageSourcePredicateTest, MultipleConditionsMatch)
{
    // 解析 JSON: is_fire = true, bypasses_armor = true
    nlohmann::json json = R"({"is_fire": true, "bypasses_armor": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // OnFire: isFire=true, bypassesArmor=true（on_fire 在 BYPASSES_ARMOR 标签内，绕过护甲，
    // 仅由火焰保护附魔减免）。此前 bypassesArmor() 漏 OnFire 返 false，已修复对齐 vanilla。
    EnvironmentalDamage onFire(DamageType::OnFire);
    EXPECT_TRUE(onFire.isFire());
    EXPECT_TRUE(onFire.bypassesArmor());
    EXPECT_TRUE(predicate.test(onFire)); // 满足 is_fire=true 且 bypasses_armor=true

    // 岩浆：是火焰，但不绕过护甲
    EnvironmentalDamage lava(DamageType::Lava);
    EXPECT_TRUE(lava.isFire());
    EXPECT_FALSE(lava.bypassesArmor());
    EXPECT_FALSE(predicate.test(lava));

    // 溺水：绕过护甲，但不是火焰
    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_FALSE(drown.isFire());
    EXPECT_TRUE(drown.bypassesArmor());
    EXPECT_FALSE(predicate.test(drown));

    // 注：EnvironmentalDamage 没有同时满足 isFire=true 和 bypassesArmor=true 的类型
    // 这需要使用 EntityDamageSource 来测试
}

TEST_F(DamageSourcePredicateTest, FireOnlyPredicateMatches)
{
    // 解析 JSON: is_fire = true
    nlohmann::json json = R"({"is_fire": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 火焰伤害应该匹配
    EnvironmentalDamage inFire(DamageType::InFire);
    EXPECT_TRUE(predicate.test(inFire));

    EnvironmentalDamage onFire(DamageType::OnFire);
    EXPECT_TRUE(predicate.test(onFire));

    EnvironmentalDamage lava(DamageType::Lava);
    EXPECT_TRUE(predicate.test(lava));

    // 非火焰伤害不应该匹配
    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_FALSE(predicate.test(drown));
}

// ========== JSON 序列化测试 ==========

TEST_F(DamageSourcePredicateTest, JsonSerialization)
{
    // 创建谓词
    nlohmann::json json = R"({
        "is_projectile": true,
        "is_fire": false,
        "is_explosion": true
    })"_json;

    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // 序列化回 JSON
    nlohmann::json serialized = predicate.toJson();

    // 验证序列化结果
    EXPECT_TRUE(serialized.contains("is_projectile"));
    EXPECT_TRUE(serialized["is_projectile"].get<bool>());
    EXPECT_TRUE(serialized.contains("is_fire"));
    EXPECT_FALSE(serialized["is_fire"].get<bool>());
    EXPECT_TRUE(serialized.contains("is_explosion"));
    EXPECT_TRUE(serialized["is_explosion"].get<bool>());

    // 不应该包含未设置的字段
    EXPECT_FALSE(serialized.contains("is_magic"));
    EXPECT_FALSE(serialized.contains("is_lightning"));
}

TEST_F(DamageSourcePredicateTest, NullJsonReturnsAny)
{
    nlohmann::json json = nullptr;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(DamageSourcePredicateTest, EmptyJsonReturnsAny)
{
    nlohmann::json json = R"({})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

// ========== EntityDamageSource 测试 ==========

TEST_F(DamageSourcePredicateTest, EntityDamageSourceProjectile)
{
    // 创建谓词：匹配投射物
    nlohmann::json json = R"({"is_projectile": true})"_json;
    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    // EntityDamageSource 投射物
    // 注意：这里使用 EnvironmentalDamage 作为占位，因为需要 Entity 参数
    // 实际测试中应该使用真实的实体
}

// ========== Getter 测试 ==========

TEST_F(DamageSourcePredicateTest, GettersReturnCorrectValues)
{
    nlohmann::json json = R"({
        "is_projectile": true,
        "is_fire": false,
        "is_explosion": true,
        "is_magic": true,
        "is_lightning": false,
        "bypasses_armor": true,
        "bypasses_invulnerability": false,
        "bypasses_magic": true
    })"_json;

    auto result = DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    DamageSourcePredicate predicate = result.value();

    EXPECT_TRUE(predicate.isProjectile().has_value());
    EXPECT_TRUE(predicate.isProjectile().value());

    EXPECT_TRUE(predicate.isFire().has_value());
    EXPECT_FALSE(predicate.isFire().value());

    EXPECT_TRUE(predicate.isExplosion().has_value());
    EXPECT_TRUE(predicate.isExplosion().value());

    EXPECT_TRUE(predicate.isMagic().has_value());
    EXPECT_TRUE(predicate.isMagic().value());

    EXPECT_TRUE(predicate.isLightning().has_value());
    EXPECT_FALSE(predicate.isLightning().value());

    EXPECT_TRUE(predicate.bypassesArmor().has_value());
    EXPECT_TRUE(predicate.bypassesArmor().value());

    EXPECT_TRUE(predicate.bypassesInvulnerability().has_value());
    EXPECT_FALSE(predicate.bypassesInvulnerability().value());

    EXPECT_TRUE(predicate.bypassesMagic().has_value());
    EXPECT_TRUE(predicate.bypassesMagic().value());
}
