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

    EntityManager m_manager;
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
        auto salmon = salmonType->create(nullptr);
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
        m_manager.addEntity(codType->create(nullptr));
    }
    for (i32 i = 0; i < 5; ++i) {
        m_manager.addEntity(salmonType->create(nullptr));
    }
    for (i32 i = 0; i < 3; ++i) {
        m_manager.addEntity(squidType->create(nullptr));
    }

    auto counts = m_manager.countEntitiesByClassification();

    // cod + salmon 都是 WaterAmbient，squid 是 WaterCreature
    EXPECT_EQ(counts[EntityClassification::WaterAmbient], 15);
    EXPECT_EQ(counts[EntityClassification::WaterCreature], 3);
}
