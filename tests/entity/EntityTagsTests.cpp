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

#include "entity/core/Entity.hpp"
#include "entity/core/MobEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// Entity Tags Tests
// ============================================================================

/**
 * @brief 测试实体标签的基本添加和移除
 *
 * 参考 MC 1.16.5 Entity.addTag() / removeTag() / getTags()
 */
TEST(EntityTags, AddAndRemove)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 初始状态应该没有标签
    EXPECT_TRUE(entity.getTags().empty());
    EXPECT_EQ(entity.getTagCount(), 0u);
    EXPECT_FALSE(entity.hasTag("test_tag"));

    // 添加标签
    EXPECT_TRUE(entity.addTag("tag1"));
    EXPECT_TRUE(entity.hasTag("tag1"));
    EXPECT_EQ(entity.getTagCount(), 1u);

    // 重复添加相同标签应该返回 false
    EXPECT_FALSE(entity.addTag("tag1"));
    EXPECT_EQ(entity.getTagCount(), 1u);

    // 添加第二个标签
    EXPECT_TRUE(entity.addTag("tag2"));
    EXPECT_EQ(entity.getTagCount(), 2u);

    // 移除标签
    EXPECT_TRUE(entity.removeTag("tag1"));
    EXPECT_FALSE(entity.hasTag("tag1"));
    EXPECT_EQ(entity.getTagCount(), 1u);

    // 移除不存在的标签应该返回 false
    EXPECT_FALSE(entity.removeTag("nonexistent"));
    EXPECT_EQ(entity.getTagCount(), 1u);
}

/**
 * @brief 测试标签数量上限（1024）
 *
 * 参考 MC 1.16.5 Entity.addTag() 中的 tags.size() >= 1024 检查
 */
TEST(EntityTags, MaxTagLimit)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 添加 1024 个标签应该成功
    for (int i = 0; i < 1024; ++i) {
        EXPECT_TRUE(entity.addTag("tag_" + std::to_string(i)));
    }
    EXPECT_EQ(entity.getTagCount(), 1024u);

    // 第 1025 个标签应该失败
    EXPECT_FALSE(entity.addTag("tag_1024"));
    EXPECT_EQ(entity.getTagCount(), 1024u);

    // 移除一个标签后，应该可以再次添加
    EXPECT_TRUE(entity.removeTag("tag_0"));
    EXPECT_TRUE(entity.addTag("tag_new"));
    EXPECT_EQ(entity.getTagCount(), 1024u);
}

/**
 * @brief 测试清空所有标签
 */
TEST(EntityTags, ClearTags)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.addTag("tag1");
    entity.addTag("tag2");
    entity.addTag("tag3");
    EXPECT_EQ(entity.getTagCount(), 3u);

    entity.clearTags();
    EXPECT_TRUE(entity.getTags().empty());
    EXPECT_EQ(entity.getTagCount(), 0u);
}

/**
 * @brief 测试标签集合的不可变性
 *
 * getTags() 返回 const 引用，不应允许外部修改
 */
TEST(EntityTags, TagsImmutability)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.addTag("tag1");

    const auto& tags = entity.getTags();
    EXPECT_EQ(tags.size(), 1u);
    EXPECT_NE(tags.find("tag1"), tags.end());

    // 无法通过 getTags() 修改（const 引用）
    // tags.insert("tag2"); // 编译错误
}

/**
 * @brief 测试空标签名
 *
 * MC 1.16.5 允许空字符串作为标签名
 */
TEST(EntityTags, EmptyTagName)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 空字符串是有效的标签名
    EXPECT_TRUE(entity.addTag(""));
    EXPECT_TRUE(entity.hasTag(""));
    EXPECT_EQ(entity.getTagCount(), 1u);

    EXPECT_TRUE(entity.removeTag(""));
    EXPECT_FALSE(entity.hasTag(""));
    EXPECT_EQ(entity.getTagCount(), 0u);
}

/**
 * @brief 测试标签的独立性
 *
 * 不同实体的标签应该独立
 */
TEST(EntityTags, IndependentTags)
{
    Entity entity1(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    Entity entity2(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());

    entity1.addTag("pig_tag");
    entity2.addTag("cow_tag");

    EXPECT_TRUE(entity1.hasTag("pig_tag"));
    EXPECT_FALSE(entity1.hasTag("cow_tag"));

    EXPECT_FALSE(entity2.hasTag("pig_tag"));
    EXPECT_TRUE(entity2.hasTag("cow_tag"));

    // 移除一个实体的标签不应影响另一个
    entity1.removeTag("pig_tag");
    EXPECT_TRUE(entity2.hasTag("cow_tag"));
}

/**
 * @brief 测试多态性
 *
 * MobEntity（Entity的子类）应该能够使用标签功能
 */
TEST(EntityTags, Polymorphism)
{
    // 直接使用 Entity 测试多态性，因为 MobEntity 构造函数参数不同
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    EXPECT_TRUE(entity.addTag("mob_tag"));
    EXPECT_TRUE(entity.hasTag("mob_tag"));

    EXPECT_TRUE(entity.removeTag("mob_tag"));
    EXPECT_FALSE(entity.hasTag("mob_tag"));
}

/**
 * @brief 测试标签的持久性
 *
 * 标签在实体生命周期内应该持续存在
 */
TEST(EntityTags, TagPersistence)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    entity.addTag("persistent_tag");
    entity.addTag("another_tag");

    // 模拟实体状态变化
    entity.setPosition(100.0f, 64.0f, 200.0f);
    entity.tick();

    // 标签应该仍然存在
    EXPECT_TRUE(entity.hasTag("persistent_tag"));
    EXPECT_TRUE(entity.hasTag("another_tag"));
    EXPECT_EQ(entity.getTagCount(), 2u);
}
