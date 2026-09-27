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

// 实体分类与自然生成容量的核心对齐测试。
// 这些测试验证一个曾经导致实体无限累积的根因：
//   鱼类（cod/salmon/pufferfish/tropical_fish）在生成配置里属于 water_ambient
//   分类（数据包 JSON 把它们放在 water_ambient 下），但实体注册时却被错配到
//   water_creature 分类。生成循环每 tick 从 EntityManager 重新统计真实分类计数，
//   错配导致 water_ambient 计数永远为 0，容量上限形同虚设，鱼类无限生成。
//
// 同一根因后来在敌对生物上复发：minecraft:zombie_horse（僵尸马）在数据包中属于 monster
// 生成列表，但实体注册时被错配到 Creature。后果是光照门槛反转（转为要求明亮，只在白天
// 露天生成）叠加怪物容量上限失效（计数落到生物配额），表现为"白天草原上大量生成僵尸马"。
// minecraft:hoglin（疣猪兽）在下界存在完全相同的错配。

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

class NaturalSpawnerClassificationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    EntityManager m_manager{mc::test::testEcsRegistry()};
};

// 断言实体类型注册的分类。辅助函数避免重复样板。
void expectClassification(const char* entityTypeId, EntityClassification expected)
{
    const EntityType* type = EntityRegistry::instance().getType(entityTypeId);
    ASSERT_NE(type, nullptr) << "实体类型未注册: " << entityTypeId;
    EXPECT_EQ(type->classification(), expected) << "实体 " << entityTypeId << " 分类错配";
}

} // namespace

// ========== 鱼类应注册为 WaterAmbient（与数据包 water_ambient 分类的生成配置一致） ==========

TEST_F(NaturalSpawnerClassificationTest, SalmonIsWaterAmbient)
{
    expectClassification(EntityTypeKeys::SALMON, EntityClassification::WaterAmbient);
}

TEST_F(NaturalSpawnerClassificationTest, CodIsWaterAmbient)
{
    expectClassification(EntityTypeKeys::COD, EntityClassification::WaterAmbient);
}

TEST_F(NaturalSpawnerClassificationTest, PufferfishIsWaterAmbient)
{
    expectClassification(EntityTypeKeys::PUFFERFISH, EntityClassification::WaterAmbient);
}

TEST_F(NaturalSpawnerClassificationTest, TropicalFishIsWaterAmbient)
{
    expectClassification(EntityTypeKeys::TROPICAL_FISH, EntityClassification::WaterAmbient);
}

// ========== 鱿鱼/海豚等应保持 WaterCreature ==========

TEST_F(NaturalSpawnerClassificationTest, SquidIsWaterCreature)
{
    expectClassification(EntityTypeKeys::SQUID, EntityClassification::WaterCreature);
}

TEST_F(NaturalSpawnerClassificationTest, DolphinIsWaterCreature)
{
    expectClassification(EntityTypeKeys::DOLPHIN, EntityClassification::WaterCreature);
}

// ========== 容量计数的分类必须与生成分类一致（这是无限生成根因的关键测试） ==========
//
// 如果 salmon 注册成 WaterCreature，那么向世界添加 salmon 后，
// countEntitiesByClassification()[WaterAmbient] 仍然是 0。
// 生成循环据此判断 water_ambient 未满 -> 永远生成 salmon -> 无限累积。
// 此测试直接复现该失效：添加 N 条 salmon 后，water_ambient 计数必须等于 N。

TEST_F(NaturalSpawnerClassificationTest, CountByClassificationMatchesSpawnCategoryForSalmon)
{
    const EntityType* salmonType = EntityRegistry::instance().getType(EntityTypeKeys::SALMON);
    ASSERT_NE(salmonType, nullptr);

    constexpr i32 kSalmonCount = 30;
    for (i32 i = 0; i < kSalmonCount; ++i) {
        auto salmon = salmonType->create(nullptr, mc::test::testEcsRegistry());
        ASSERT_NE(salmon, nullptr);
        m_manager.addEntity(std::move(salmon));
    }

    auto counts = m_manager.countEntitiesByClassification();

    // salmon 在数据包里属于 water_ambient，所以计数必须落到 WaterAmbient
    EXPECT_EQ(counts[EntityClassification::WaterAmbient], kSalmonCount)
        << "salmon 计数未落到 WaterAmbient，容量上限会因此失效";
    // 不应错误计入 WaterCreature
    EXPECT_EQ(counts[EntityClassification::WaterCreature], 0) << "salmon 不应计入 WaterCreature";
}

TEST_F(NaturalSpawnerClassificationTest, CountByClassificationMixedFish)
{
    const EntityType* codType = EntityRegistry::instance().getType(EntityTypeKeys::COD);
    const EntityType* salmonType = EntityRegistry::instance().getType(EntityTypeKeys::SALMON);
    const EntityType* squidType = EntityRegistry::instance().getType(EntityTypeKeys::SQUID);
    ASSERT_NE(codType, nullptr);
    ASSERT_NE(salmonType, nullptr);
    ASSERT_NE(squidType, nullptr);

    for (i32 i = 0; i < 10; ++i) {
        m_manager.addEntity(codType->create(nullptr, mc::test::testEcsRegistry()));
    }
    for (i32 i = 0; i < 5; ++i) {
        m_manager.addEntity(salmonType->create(nullptr, mc::test::testEcsRegistry()));
    }
    for (i32 i = 0; i < 3; ++i) {
        m_manager.addEntity(squidType->create(nullptr, mc::test::testEcsRegistry()));
    }

    auto counts = m_manager.countEntitiesByClassification();

    // cod + salmon 都是 WaterAmbient，squid 是 WaterCreature
    EXPECT_EQ(counts[EntityClassification::WaterAmbient], 15);
    EXPECT_EQ(counts[EntityClassification::WaterCreature], 3);
}

// ========== 敌对生物的同类错配（僵尸马/疣猪兽） ==========
//
// 数据包把僵尸马放进平原、向日葵平原、热带草原、热带高原、风袭热带草原、雪原的
// monster 生成列表，疣猪兽放进下界生物群系的 monster 列表。若注册为 Creature：
//   - NaturalSpawner::_canSpawnAt 走 Creature 分支要求明亮光照 → 只在白天露天生成；
//   - 生成时向怪物配额累加，但 countEntitiesByClassification 把它们算进生物配额，
//     于是怪物计数永不增长 → 上限失效 → 无限堆积。

TEST_F(NaturalSpawnerClassificationTest, ZombieHorseIsMonster)
{
    expectClassification(EntityTypeKeys::ZOMBIE_HORSE, EntityClassification::Monster);
}

TEST_F(NaturalSpawnerClassificationTest, HoglinIsMonster)
{
    expectClassification(EntityTypeKeys::HOGLIN, EntityClassification::Monster);
}

TEST_F(NaturalSpawnerClassificationTest, CountByClassificationMatchesSpawnCategoryForZombieHorse)
{
    const EntityType* zombieHorseType = EntityRegistry::instance().getType(EntityTypeKeys::ZOMBIE_HORSE);
    ASSERT_NE(zombieHorseType, nullptr);

    constexpr i32 kZombieHorseCount = 30;
    for (i32 i = 0; i < kZombieHorseCount; ++i) {
        auto horse = zombieHorseType->create(nullptr, mc::test::testEcsRegistry());
        ASSERT_NE(horse, nullptr);
        m_manager.addEntity(std::move(horse));
    }

    auto counts = m_manager.countEntitiesByClassification();

    // 僵尸马在数据包里属于 monster，所以计数必须落到 Monster；否则怪物容量上限失效，
    // 僵尸马会无视上限持续堆积（这正是"白天草原大量生成僵尸马"的直接原因之一）。
    EXPECT_EQ(counts[EntityClassification::Monster], kZombieHorseCount)
        << "僵尸马计数未落到 Monster，怪物容量上限会因此失效";
    EXPECT_EQ(counts[EntityClassification::Creature], 0) << "僵尸马不应计入 Creature";
}
