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

#include "common/util/LinkedHashSet.hpp"
#include <gtest/gtest.h>

using namespace mc;

TEST(LinkedHashSetTest, EmptySet)
{
    LinkedHashSet<int> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0u);
    EXPECT_FALSE(set.contains(1));
    EXPECT_EQ(set.find(1), set.end());
    EXPECT_EQ(set.count(1), 0u);
}

TEST(LinkedHashSetTest, InsertAndContains)
{
    LinkedHashSet<int> set;
    auto [it1, inserted1] = set.insert(10);
    EXPECT_TRUE(inserted1);
    EXPECT_EQ(*it1, 10);
    EXPECT_EQ(set.size(), 1u);

    auto [it2, inserted2] = set.insert(20);
    EXPECT_TRUE(inserted2);
    EXPECT_EQ(*it2, 20);
    EXPECT_EQ(set.size(), 2u);

    auto [it3, inserted3] = set.insert(10);
    EXPECT_FALSE(inserted3);
    EXPECT_EQ(*it3, 10);
    EXPECT_EQ(set.size(), 2u);

    EXPECT_TRUE(set.contains(10));
    EXPECT_TRUE(set.contains(20));
    EXPECT_FALSE(set.contains(30));
}

TEST(LinkedHashSetTest, InsertionOrderPreserved)
{
    LinkedHashSet<std::string> set;
    set.insert("alpha");
    set.insert("beta");
    set.insert("gamma");
    set.insert("delta");

    // 遍历顺序应与插入顺序一致
    auto it = set.begin();
    EXPECT_EQ(*it++, "alpha");
    EXPECT_EQ(*it++, "beta");
    EXPECT_EQ(*it++, "gamma");
    EXPECT_EQ(*it++, "delta");
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, DuplicateInsertDoesNotChangeOrder)
{
    LinkedHashSet<std::string> set;
    set.insert("first");
    set.insert("second");
    set.insert("third");

    // 重复插入不改变顺序
    set.insert("second");

    auto it = set.begin();
    EXPECT_EQ(*it++, "first");
    EXPECT_EQ(*it++, "second");
    EXPECT_EQ(*it++, "third");
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, EraseByIterator)
{
    LinkedHashSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);

    // 删除第二个元素
    auto it = set.begin();
    ++it; // 指向 2
    auto next = set.erase(it);
    EXPECT_EQ(*next, 3);
    EXPECT_EQ(set.size(), 2u);
    EXPECT_FALSE(set.contains(2));
    EXPECT_TRUE(set.contains(1));
    EXPECT_TRUE(set.contains(3));

    // 顺序应保持
    it = set.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, EraseByValue)
{
    LinkedHashSet<int> set;
    set.insert(10);
    set.insert(20);
    set.insert(30);

    EXPECT_TRUE(set.erase(20));
    EXPECT_FALSE(set.erase(99));
    EXPECT_EQ(set.size(), 2u);
    EXPECT_FALSE(set.contains(20));

    auto it = set.begin();
    EXPECT_EQ(*it++, 10);
    EXPECT_EQ(*it++, 30);
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, EraseFirstElement_FIFO)
{
    // 模拟 VaultBlockEntity 的淘汰逻辑：超上限时删除最早插入的
    LinkedHashSet<std::string> set;
    constexpr int MAX = 5;

    for (int i = 0; i < MAX + 3; ++i) {
        set.insert("player-" + std::to_string(i));
        if (static_cast<int>(set.size()) > MAX) {
            set.erase(set.begin()); // 淘汰最早插入的
        }
    }

    // 最终应只保留最后 MAX 个
    EXPECT_EQ(set.size(), static_cast<size_t>(MAX));
    EXPECT_FALSE(set.contains("player-0"));
    EXPECT_FALSE(set.contains("player-1"));
    EXPECT_FALSE(set.contains("player-2"));
    EXPECT_TRUE(set.contains("player-3"));
    EXPECT_TRUE(set.contains("player-4"));
    EXPECT_TRUE(set.contains("player-5"));
    EXPECT_TRUE(set.contains("player-6"));
    EXPECT_TRUE(set.contains("player-7"));

    // 顺序应与插入顺序一致
    auto it = set.begin();
    EXPECT_EQ(*it++, "player-3");
    EXPECT_EQ(*it++, "player-4");
    EXPECT_EQ(*it++, "player-5");
    EXPECT_EQ(*it++, "player-6");
    EXPECT_EQ(*it++, "player-7");
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, Clear)
{
    LinkedHashSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    set.clear();
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0u);
    EXPECT_FALSE(set.contains(1));
}

TEST(LinkedHashSetTest, CopyConstructor)
{
    LinkedHashSet<int> original;
    original.insert(1);
    original.insert(2);
    original.insert(3);

    LinkedHashSet<int> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_TRUE(copy.contains(1));
    EXPECT_TRUE(copy.contains(2));
    EXPECT_TRUE(copy.contains(3));

    // 顺序应一致
    auto origIt = original.begin();
    auto copyIt = copy.begin();
    while (origIt != original.end()) {
        EXPECT_EQ(*origIt, *copyIt);
        ++origIt;
        ++copyIt;
    }
}

TEST(LinkedHashSetTest, MoveConstructor)
{
    LinkedHashSet<int> original;
    original.insert(10);
    original.insert(20);

    LinkedHashSet<int> moved(std::move(original));
    EXPECT_EQ(moved.size(), 2u);
    EXPECT_TRUE(moved.contains(10));
    EXPECT_TRUE(moved.contains(20));
}

TEST(LinkedHashSetTest, CopyAssignment)
{
    LinkedHashSet<int> original;
    original.insert(1);
    original.insert(2);

    LinkedHashSet<int> assigned;
    assigned.insert(99);
    assigned = original;

    EXPECT_EQ(assigned.size(), 2u);
    EXPECT_TRUE(assigned.contains(1));
    EXPECT_TRUE(assigned.contains(2));
    EXPECT_FALSE(assigned.contains(99));
}

TEST(LinkedHashSetTest, InitializerList)
{
    LinkedHashSet<int> set{1, 2, 3, 2, 1};
    EXPECT_EQ(set.size(), 3u);
    auto it = set.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
}

TEST(LinkedHashSetTest, RangeConstructor)
{
    std::vector<int> vec{5, 3, 1, 3, 5};
    LinkedHashSet<int> set(vec.begin(), vec.end());
    EXPECT_EQ(set.size(), 3u);
    auto it = set.begin();
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 1);
}

TEST(LinkedHashSetTest, Find)
{
    LinkedHashSet<std::string> set;
    set.insert("hello");
    set.insert("world");

    auto it = set.find("hello");
    EXPECT_NE(it, set.end());
    EXPECT_EQ(*it, "hello");

    it = set.find("missing");
    EXPECT_EQ(it, set.end());
}

TEST(LinkedHashSetTest, Count)
{
    LinkedHashSet<int> set;
    set.insert(42);
    EXPECT_EQ(set.count(42), 1u);
    EXPECT_EQ(set.count(99), 0u);
}

TEST(LinkedHashSetTest, SerializationOrder)
{
    // 模拟 VaultBlockEntity 的序列化/反序列化场景
    LinkedHashSet<std::string> original;
    original.insert("uuid-001");
    original.insert("uuid-002");
    original.insert("uuid-003");

    // 序列化
    std::vector<std::string> serialized;
    for (const auto& uuid : original) {
        serialized.push_back(uuid);
    }

    // 反序列化到新集合
    LinkedHashSet<std::string> loaded;
    for (const auto& uuid : serialized) {
        loaded.insert(uuid);
    }

    // 顺序应一致
    auto origIt = original.begin();
    auto loadIt = loaded.begin();
    while (origIt != original.end()) {
        EXPECT_EQ(*origIt, *loadIt);
        ++origIt;
        ++loadIt;
    }
}
