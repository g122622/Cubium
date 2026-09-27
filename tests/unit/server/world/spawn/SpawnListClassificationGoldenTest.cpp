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
 * @file SpawnListClassificationGoldenTest.cpp
 * @brief 数据包生成列表实体的「注册分类 vs 原版 MobCategory」黄金表测试。
 *
 * 缺陷模式（本测试要永久锁定的根因）：
 *   生物群系的 spawners 列表由数据包提供，而自然生成流程按「实体注册分类」判定
 *   光照门槛（NaturalSpawner::_canSpawnAt）与容量统计（EntityManager::countEntitiesByClassification）。
 *   一旦某实体的注册分类与其在数据包列表中所属分类错配，就会同时产生两类错误后果：
 *     1) 光照门槛反转：注册为 Creature 的敌对生物会要求"明亮"光照，只在白天露天生成；
 *     2) 容量上限失效：生成时消耗 A 分类配额，但计数落到 B 分类，A 的计数永不增长，
 *        上限永不触顶而导致实体无限堆积。
 *
 * 实际案例："白天草原上大量生成僵尸马"——minecraft:zombie_horse 在数据包中属于 monster
 * 列表，但项目曾将其注册为 Creature，因而白天露天疯狂生成且怪物上限失效。
 * 同一错配还存在于 hoglin（下界）、zombie_nautilus、villager。
 *
 * 维护约定：
 *   - 新增生物群系生成列表条目时，若该实体尚未注册，请加入 kKnownUnregisteredSpawnTypes，
 *     并在实现该实体后把它移入 kSpawnListClassifications。
 *   - 修改任何实体的注册分类时，若该实体出现在数据包生成列表中，本测试会立即失败。
 */

#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include <string_view>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

/**
 * @brief 数据包引用类型 → 期望分类的黄金表条目
 */
struct SpawnListClassificationCase {
    std::string_view entityTypeId; ///< 实体类型资源位置
    EntityClassification expected; ///< 期望的注册分类（取自原版该实体的 MobCategory）
};

/// 数据包生物群系 spawners 列表引用的全部实体，及其在原版中的生成分类。
/// 该表是"注册分类必须与数据包列表分类口径一致"这一不变量的唯一事实来源。
constexpr SpawnListClassificationCase kSpawnListClassifications[] = {
    {"minecraft:axolotl", EntityClassification::Axolotls},
    {"minecraft:bat", EntityClassification::Ambient},
    {"minecraft:bogged", EntityClassification::Monster},
    {"minecraft:chicken", EntityClassification::Creature},
    {"minecraft:cod", EntityClassification::WaterAmbient},
    {"minecraft:cow", EntityClassification::Creature},
    {"minecraft:creeper", EntityClassification::Monster},
    {"minecraft:dolphin", EntityClassification::WaterCreature},
    {"minecraft:donkey", EntityClassification::Creature},
    {"minecraft:drowned", EntityClassification::Monster},
    {"minecraft:enderman", EntityClassification::Monster},
    {"minecraft:fox", EntityClassification::Creature},
    {"minecraft:ghast", EntityClassification::Monster},
    {"minecraft:glow_squid", EntityClassification::UndergroundWaterCreature},
    {"minecraft:hoglin", EntityClassification::Monster},
    {"minecraft:horse", EntityClassification::Creature},
    {"minecraft:husk", EntityClassification::Monster},
    {"minecraft:llama", EntityClassification::Creature},
    {"minecraft:magma_cube", EntityClassification::Monster},
    {"minecraft:mooshroom", EntityClassification::Creature},
    {"minecraft:nautilus", EntityClassification::WaterCreature},
    // 豹猫例外：数据包把它放进 monster 列表，但原版实体自身的 MobCategory 是 CREATURE。
    // 该差异属原版既有设计（豹猫按敌对逻辑参与丛林刷怪），故期望值取实体自身的 CREATURE。
    {"minecraft:ocelot", EntityClassification::Creature},
    {"minecraft:panda", EntityClassification::Creature},
    {"minecraft:parrot", EntityClassification::Creature},
    {"minecraft:pig", EntityClassification::Creature},
    {"minecraft:piglin", EntityClassification::Monster},
    {"minecraft:polar_bear", EntityClassification::Creature},
    {"minecraft:pufferfish", EntityClassification::WaterAmbient},
    {"minecraft:rabbit", EntityClassification::Creature},
    {"minecraft:salmon", EntityClassification::WaterAmbient},
    {"minecraft:sheep", EntityClassification::Creature},
    {"minecraft:skeleton", EntityClassification::Monster},
    {"minecraft:slime", EntityClassification::Monster},
    {"minecraft:spider", EntityClassification::Monster},
    {"minecraft:squid", EntityClassification::WaterCreature},
    {"minecraft:stray", EntityClassification::Monster},
    {"minecraft:strider", EntityClassification::Creature},
    {"minecraft:tropical_fish", EntityClassification::WaterAmbient},
    {"minecraft:turtle", EntityClassification::Creature},
    {"minecraft:witch", EntityClassification::Monster},
    {"minecraft:wolf", EntityClassification::Creature},
    {"minecraft:zombie", EntityClassification::Monster},
    {"minecraft:zombie_horse", EntityClassification::Monster},
    {"minecraft:zombie_villager", EntityClassification::Monster},
    {"minecraft:zombified_piglin", EntityClassification::Monster},
};

/**
 * @brief 数据包生成列表引用、但项目尚未注册的实体类型
 *
 * 这些条目在自然生成时会被跳过（NaturalSpawner 会打印去重后的 warn 日志）。
 * 它们不参与分类断言，但本测试会断言其"仍未注册"，以便在实现它们时强制更新本文件，
 * 避免实现后遗忘分类口径核对。
 */
constexpr std::string_view kKnownUnregisteredSpawnTypes[] = {
    "minecraft:armadillo",
    "minecraft:camel",
    "minecraft:frog",
    "minecraft:goat",
    "minecraft:parched",
};

} // namespace

// ============================================================================
// 数据包生成列表实体的分类黄金表
// ============================================================================

// 数据包生成列表引用的每个已注册实体，其注册分类必须与该实体在原版的 MobCategory 一致。
// 分类错配会导致光照门槛反转与容量上限失效（见文件头说明）。
TEST(SpawnListClassificationGoldenTest, SpawnListEntitiesUseVanillaClassification)
{
    VanillaEntities::registerAll();

    for (const auto& testCase : kSpawnListClassifications) {
        const EntityType* type = EntityRegistry::instance().getType(std::string(testCase.entityTypeId));
        ASSERT_NE(type, nullptr) << "数据包生成列表引用的实体未注册: " << testCase.entityTypeId;
        EXPECT_EQ(type->classification(), testCase.expected)
            << "实体 " << testCase.entityTypeId << " 的注册分类与数据包生成列表口径不一致；"
            << "该错配会导致光照门槛反转（白天生成敌对生物）或容量上限失效（无限堆积）";
    }
}

// 已知未注册类型必须保持未注册：一旦某个实体被实现，本断言失败，提示把它移入
// kSpawnListClassifications 并核对分类口径。
TEST(SpawnListClassificationGoldenTest, KnownUnregisteredSpawnTypesRemainUnregistered)
{
    VanillaEntities::registerAll();

    for (const std::string_view entityTypeId : kKnownUnregisteredSpawnTypes) {
        const EntityType* type = EntityRegistry::instance().getType(std::string(entityTypeId));
        EXPECT_EQ(type, nullptr) << "实体 " << entityTypeId << " 已被注册，请将其从 kKnownUnregisteredSpawnTypes 移入 "
                                 << "kSpawnListClassifications 并补充期望分类断言";
    }
}

// ============================================================================
// 非生成列表实体的分类口径（不参与自然生成，但容量统计与和平判定仍按分类进行）
// ============================================================================

// 僵尸鹦鹉螺与村民不出现在任何生物群系生成列表中，但分类口径同样须与实体自身性质一致：
//   - 僵尸鹦鹉螺是亡灵敌对生物，注册为 Monster，否则会被错算进水生生物配额；
//   - 村民不参与自然生成，注册为 Misc。
// 骷髅马为生物分类（仅经命令/刷怪蛋获得，无自然生成途径）。
TEST(SpawnListClassificationGoldenTest, NonSpawnListEntityClassificationAlignment)
{
    VanillaEntities::registerAll();

    const EntityType* zombieNautilus = EntityRegistry::instance().getType("minecraft:zombie_nautilus");
    ASSERT_NE(zombieNautilus, nullptr);
    EXPECT_EQ(zombieNautilus->classification(), EntityClassification::Monster);

    const EntityType* villager = EntityRegistry::instance().getType("minecraft:villager");
    ASSERT_NE(villager, nullptr);
    EXPECT_EQ(villager->classification(), EntityClassification::Misc);

    const EntityType* skeletonHorse = EntityRegistry::instance().getType("minecraft:skeleton_horse");
    ASSERT_NE(skeletonHorse, nullptr);
    EXPECT_EQ(skeletonHorse->classification(), EntityClassification::Creature);
}
