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

#include "item/loot/LootPredicateManager.hpp"
#include "item/loot/conditions/LootConditions.hpp"

using namespace mc;
using namespace mc::loot;

// ============================================================================
// LootPredicateManager 注册与查找测试
// ============================================================================

class LootPredicateManagerTest : public ::testing::Test {
protected:
    LootPredicateManager manager;
};

TEST_F(LootPredicateManagerTest, RegisterAndGetPredicate)
{
    // 注册一个谓词，然后查找
    auto condition = std::make_unique<RandomChanceCondition>(0.5f);
    manager.registerPredicate("minecraft:gameplay/raid", std::move(condition));

    const LootCondition* result = manager.getPredicate("minecraft:gameplay/raid");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getType(), "random_chance");
}

TEST_F(LootPredicateManagerTest, GetNonExistentPredicate)
{
    // 查找不存在的谓词应返回 nullptr
    const LootCondition* result = manager.getPredicate("minecraft:nonexistent");
    EXPECT_EQ(result, nullptr);
}

TEST_F(LootPredicateManagerTest, HasPredicate)
{
    // 注册前不存在
    EXPECT_FALSE(manager.hasPredicate("minecraft:test/predicate"));

    // 注册后存在
    manager.registerPredicate("minecraft:test/predicate", std::make_unique<RandomChanceCondition>(1.0f));
    EXPECT_TRUE(manager.hasPredicate("minecraft:test/predicate"));
}

TEST_F(LootPredicateManagerTest, HasPredicateNonExistent)
{
    EXPECT_FALSE(manager.hasPredicate("minecraft:does_not_exist"));
}

TEST_F(LootPredicateManagerTest, RegisterNullConditionIsNoOp)
{
    // 注册 nullptr 条件应被忽略
    manager.registerPredicate("minecraft:null_test", nullptr);
    EXPECT_FALSE(manager.hasPredicate("minecraft:null_test"));
    EXPECT_EQ(manager.getPredicate("minecraft:null_test"), nullptr);
}

TEST_F(LootPredicateManagerTest, RegisterOverwritesExisting)
{
    // 注册同名谓词应覆盖
    manager.registerPredicate("minecraft:overwrite_test", std::make_unique<RandomChanceCondition>(0.1f));
    manager.registerPredicate("minecraft:overwrite_test", std::make_unique<RandomChanceCondition>(0.9f));

    const LootCondition* result = manager.getPredicate("minecraft:overwrite_test");
    ASSERT_NE(result, nullptr);

    // 应该是后注册的条件（0.9f 的 RandomChanceCondition）
    auto* rc = dynamic_cast<const RandomChanceCondition*>(result);
    ASSERT_NE(rc, nullptr);
    // 注意：RandomChanceCondition 没有公开 chance 值的 getter，所以我们只验证类型正确
    EXPECT_EQ(result->getType(), "random_chance");
}

TEST_F(LootPredicateManagerTest, Clear)
{
    // 注册多个谓词
    manager.registerPredicate("minecraft:test/a", std::make_unique<RandomChanceCondition>(0.5f));
    manager.registerPredicate("minecraft:test/b", std::make_unique<SurvivesExplosionCondition>());
    manager.registerPredicate("minecraft:test/c", std::make_unique<KilledByPlayerCondition>());

    EXPECT_TRUE(manager.hasPredicate("minecraft:test/a"));
    EXPECT_TRUE(manager.hasPredicate("minecraft:test/b"));
    EXPECT_TRUE(manager.hasPredicate("minecraft:test/c"));

    // 清空后全部不存在
    manager.clear();

    EXPECT_FALSE(manager.hasPredicate("minecraft:test/a"));
    EXPECT_FALSE(manager.hasPredicate("minecraft:test/b"));
    EXPECT_FALSE(manager.hasPredicate("minecraft:test/c"));
}

TEST_F(LootPredicateManagerTest, ClearEmptyManager)
{
    // 对空管理器调用 clear 不应该崩溃
    manager.clear();
    EXPECT_FALSE(manager.hasPredicate("minecraft:any"));
}

TEST_F(LootPredicateManagerTest, GetAllPredicateIds)
{
    // 空管理器应返回空列表
    auto ids = manager.getAllPredicateIds();
    EXPECT_TRUE(ids.empty());

    // 注册多个谓词
    manager.registerPredicate("minecraft:gameplay/raid", std::make_unique<RandomChanceCondition>(0.5f));
    manager.registerPredicate("minecraft:gameplay/fishing", std::make_unique<SurvivesExplosionCondition>());
    manager.registerPredicate("mod_id:custom/predicate", std::make_unique<KilledByPlayerCondition>());

    ids = manager.getAllPredicateIds();
    EXPECT_EQ(ids.size(), 3u);

    // 验证所有 ID 都存在（不保证顺序）
    EXPECT_NE(std::find(ids.begin(), ids.end(), "minecraft:gameplay/raid"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "minecraft:gameplay/fishing"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "mod_id:custom/predicate"), ids.end());
}

TEST_F(LootPredicateManagerTest, RegisterMultipleConditionTypes)
{
    // 注册不同类型的条件
    manager.registerPredicate("minecraft:random_chance", std::make_unique<RandomChanceCondition>(0.5f));
    manager.registerPredicate("minecraft:survives_explosion", std::make_unique<SurvivesExplosionCondition>());
    manager.registerPredicate("minecraft:killed_by_player", std::make_unique<KilledByPlayerCondition>());

    EXPECT_EQ(manager.getPredicate("minecraft:random_chance")->getType(), "random_chance");
    EXPECT_EQ(manager.getPredicate("minecraft:survives_explosion")->getType(), "survives_explosion");
    EXPECT_EQ(manager.getPredicate("minecraft:killed_by_player")->getType(), "killed_by_player");
}

TEST_F(LootPredicateManagerTest, MoveSemantics)
{
    // 注册谓词
    manager.registerPredicate("minecraft:test/move", std::make_unique<RandomChanceCondition>(0.5f));

    // 移动构造
    LootPredicateManager movedManager = std::move(manager);
    EXPECT_TRUE(movedManager.hasPredicate("minecraft:test/move"));

    // 原管理器应处于有效但未指定的状态
    // 不测试原管理器的行为，只验证新管理器正确
    const LootCondition* result = movedManager.getPredicate("minecraft:test/move");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getType(), "random_chance");
}
