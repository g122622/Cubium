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
 * LIABILITY,WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// EntityType::isAllowedInPeaceful()（NotInPeaceful 标志位取反）逐实体标注正确性测试。
//
// 背景：对齐 Java 1.21.11 EntityType.notInPeaceful 字段（Builder.notInPeaceful() 置位，
// 默认 false）。和平难度下 /summon、刷怪蛋、刷怪箱等生成路径用 isAllowedInPeaceful() 守卫
// 拦截 notInPeaceful 实体（对齐 SummonCommand.java:86、SpawnEggItem.isAllowedInPeaceful）。
//
// 精确化要点（区别于旧的 classification 派生）：
// 旧实现用 entity::isPeaceful(classification)（= classification != Monster）派生和平可召性，
// 把 Monster 分类等价于和平禁召。但 vanilla 有 6 个 Monster 类实体（Shulker/Hoglin/Piglin/
// EnderDragon/ZombieHorse/ZombieNautilus）未调 notInPeaceful 故和平可召——派生逻辑误拒这些。
// 精确化改为独立 NotInPeaceful 标志位 + 逐实体标注（35 个 vanilla 调 notInPeaceful 的实体显式
// 标注，6 个对照实体保持默认未标），isAllowedInPeaceful() 严格按标志位判定。
//
// 本测试直接断言注册表中各实体的 isAllowedInPeaceful() 字段，验证：
//   1. 标注了 notInPeaceful 的代表性怪物（zombie/skeleton/creeper/...）和平禁召（false）
//   2. 未标注的 Monster 类对照实体（shulker/piglin/hoglin/...）和平可召（true）——精确化核心
//   3. 动物类（cow/pig）和平可召（true）
//   4. canSummon() 与 isAllowedInPeaceful() 独立性（giant 不可召唤但仍可和平判定）
// 单元测试直接读注册表字段，不受 DespawnManager/世界状态干扰（集成测试无法验证 Monster 类
// 对照实体，因 Cubium DespawnManager 和平清除分支会清掉所有 isDespawnPeaceful 怪物）。
//
// 注：EntityFlags 有两个同名枚举——mc::EntityFlags（u8，实体共享标志位 OnFire/Crouching 等，
// 见 EntityFlags.hpp）与 mc::entity::EntityFlags（u32，EntityType 类型级标志 ImmuneToFire/
// CanSummon/NotInPeaceful 等，见 EntityType.hpp）。本测试用后者，故 EntityFlags 显式限定
// entity::EntityFlags 消歧（对齐 EntityCoreTests 的 EntityType.Flags 测试范式）。

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <vector>

using namespace mc;
using namespace mc::entity;

class EntityPeacefulFlagTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // VanillaEntities::registerAll 注册全部实体类型（含逐实体 notInPeaceful 标注）。
        // VanillaBlocks::initialize 是 registerAll 的前置依赖（部分实体注册引用方块）。
        // registerAll 幂等（m_registered 标志 + clear 重注册），多 fixture 共享安全。
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }

    // 按 typeId 查询 EntityType，断言存在并返回。
    static const EntityType* requireType(const char* typeId)
    {
        const EntityType* type = EntityRegistry::instance().getType(typeId);
        EXPECT_NE(type, nullptr) << "EntityType not registered: " << typeId;
        return type;
    }
};

// ============================================================================
// 标注了 notInPeaceful 的代表性怪物：和平难度禁召（isAllowedInPeaceful==false）
// 对齐 Java EntityType 注册链中调用 .notInPeaceful() 的实体（VanillaEntities.cpp 35 处）。
// 选取各类别代表性实体覆盖：亡灵（zombie/skeleton/husk/drowned/stray/wither_skeleton/
// zombie_villager/endermite/silverfish/zombified_piglin）、节肢（spider/cave_spider）、
// 灾厄（witch/vindicator/evoker/illusioner/pillager/ravager/vex）、下界（blaze/ghast/
// magma_cube/piglin_brute/zoglin）、特殊（creeper/enderman/slime/guardian/elder_guardian/
// phantom/wither/warden/giant/breeze/bogged）。
// ============================================================================
TEST_F(EntityPeacefulFlagTest, NotInPeacefulMonstersAreBlockedOnPeaceful)
{
    const std::vector<const char*> blocked = {"minecraft:zombie",
        "minecraft:skeleton",
        "minecraft:creeper",
        "minecraft:spider",
        "minecraft:enderman",
        "minecraft:blaze",
        "minecraft:witch",
        "minecraft:slime",
        "minecraft:guardian",
        "minecraft:elder_guardian",
        "minecraft:husk",
        "minecraft:drowned",
        "minecraft:stray",
        "minecraft:bogged",
        "minecraft:wither_skeleton",
        "minecraft:cave_spider",
        "minecraft:wither",
        "minecraft:warden",
        "minecraft:phantom",
        "minecraft:zombie_villager",
        "minecraft:endermite",
        "minecraft:silverfish",
        "minecraft:ghast",
        "minecraft:magma_cube",
        "minecraft:piglin_brute",
        "minecraft:zoglin",
        "minecraft:zombified_piglin",
        "minecraft:vindicator",
        "minecraft:evoker",
        "minecraft:illusioner",
        "minecraft:pillager",
        "minecraft:ravager",
        "minecraft:vex",
        "minecraft:breeze",
        "minecraft:giant"};

    for (const char* id : blocked) {
        const EntityType* type = requireType(id);
        if (type == nullptr) {
            continue;
        }
        EXPECT_FALSE(type->isAllowedInPeaceful()) << id << " should be blocked on peaceful (notInPeaceful flag set)";
        EXPECT_TRUE(type->hasFlag(entity::EntityFlags::NotInPeaceful)) << id << " should have NotInPeaceful flag";
    }
}

// ============================================================================
// 未标注 notInPeaceful 的 Monster 类对照实体：和平可召（isAllowedInPeaceful==true）
// 这是精确化的核心验证点——这些实体 classification==Monster 但 vanilla 未调 notInPeaceful，
// 旧 classification 派生逻辑会误拒（isPeaceful(Monster)=false），精确化标志位正确放行。
// 对照实体：shulker/piglin/hoglin/ender_dragon/zombie_horse/zombie_nautilus。
// ============================================================================
TEST_F(EntityPeacefulFlagTest, MonsterClassificationButAllowedInPeaceful)
{
    const std::vector<const char*> allowed = {"minecraft:shulker",
        "minecraft:piglin",
        "minecraft:hoglin",
        "minecraft:ender_dragon",
        "minecraft:zombie_horse",
        "minecraft:zombie_nautilus"};

    for (const char* id : allowed) {
        const EntityType* type = requireType(id);
        if (type == nullptr) {
            continue;
        }
        EXPECT_TRUE(type->isAllowedInPeaceful())
            << id << " should be allowed in peaceful (Monster class but notInPeaceful not set)";
        EXPECT_FALSE(type->hasFlag(entity::EntityFlags::NotInPeaceful)) << id << " should NOT have NotInPeaceful flag";
        // 注：ender_dragon 在 Cubium 注册为 Monster 分类但未标 notInPeaceful（和平可召）；
        // zombie_horse/zombie_nautilus 实际非 Monster 分类（Creature/WaterCreature），
        // 但仍属"vanilla 未标 notInPeaceful 的可和平召实体"对照集——此处只断言
        // isAllowedInPeaceful==true 与无 NotInPeaceful 标志，不强制分类。
    }
}

// ============================================================================
// 动物/被动类：和平可召（isAllowedInPeaceful==true，默认无标志）
// ============================================================================
TEST_F(EntityPeacefulFlagTest, PassiveMobsAllowedInPeaceful)
{
    const std::vector<const char*> allowed = {"minecraft:cow",
        "minecraft:pig",
        "minecraft:sheep",
        "minecraft:chicken",
        "minecraft:villager",
        "minecraft:iron_golem",
        "minecraft:snow_golem",
        "minecraft:bat",
        "minecraft:squid",
        "minecraft:horse"};

    for (const char* id : allowed) {
        const EntityType* type = requireType(id);
        if (type == nullptr) {
            continue;
        }
        EXPECT_TRUE(type->isAllowedInPeaceful()) << id << " passive mob should be allowed in peaceful";
        EXPECT_FALSE(type->hasFlag(entity::EntityFlags::NotInPeaceful))
            << id << " passive mob should NOT have NotInPeaceful flag";
    }
}

// ============================================================================
// isAllowedInPeaceful 与 canSummon 独立性：
// giant 不可召唤（canSummon==false）但仍标 notInPeaceful（和平禁召语义独立于可召唤性）。
// 验证 notInPeaceful 标注不依赖 canSummon（giant 特殊：无 canSummon(true) 但有 notInPeaceful）。
// ============================================================================
TEST_F(EntityPeacefulFlagTest, NotInPeacefulIndependentFromCanSummon)
{
    const EntityType* giant = requireType("minecraft:giant");
    if (giant == nullptr) {
        GTEST_SKIP();
    }
    EXPECT_FALSE(giant->canSummon()) << "giant should not be summonable";
    EXPECT_FALSE(giant->isAllowedInPeaceful())
        << "giant should be blocked on peaceful (notInPeaceful set independent of canSummon)";
    EXPECT_TRUE(giant->hasFlag(entity::EntityFlags::NotInPeaceful));
}

// ============================================================================
// EntityType::Builder.notInPeaceful() + isAllowedInPeaceful() 机制层验证（不依赖 vanilla 注册表）：
// 默认（不调 notInPeaceful）isAllowedInPeaceful==true；调后==false。
// 对齐 Java Builder.notInPeaceful() 默认 false 语义。
// ============================================================================
TEST_F(EntityPeacefulFlagTest, BuilderNotInPeacefulFlagSemantics)
{
    auto factory = [](IWorld*, ecs::EntityRegistry&) -> std::unique_ptr<Entity> { return nullptr; };

    // 默认无标志 → 和平可召。
    EntityType defaultType = EntityType::Builder(factory, EntityClassification::Monster).build();
    EXPECT_TRUE(defaultType.isAllowedInPeaceful());
    EXPECT_FALSE(defaultType.hasFlag(entity::EntityFlags::NotInPeaceful));

    // 调 notInPeaceful → 和平禁召。
    EntityType blockedType = EntityType::Builder(factory, EntityClassification::Monster).notInPeaceful().build();
    EXPECT_FALSE(blockedType.isAllowedInPeaceful());
    EXPECT_TRUE(blockedType.hasFlag(entity::EntityFlags::NotInPeaceful));

    // notInPeaceful 与其他标志正交（可同时 fireImmune + notInPeaceful）。
    EntityType combinedType =
        EntityType::Builder(factory, EntityClassification::Monster).immuneToFire().notInPeaceful().build();
    EXPECT_FALSE(combinedType.isAllowedInPeaceful());
    EXPECT_TRUE(combinedType.immuneToFire());
}
