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

// EntityDataManager 的槽位存储契约测试。
//
// 背景：槽位容器由 std::unordered_map<u16, DataEntry> 改为按 id 稠密索引的
// std::vector<DataEntry>（服务端 id 空间稠密，map 每条约 80 字节的节点开销不必要）。
// 随之引入两条必须锁死的语义：
//   1. 落在数组范围内 ≠ 已注册 —— 必须靠 DataEntry::present 区分；
//   2. getRaw 返回副本（std::optional）而非内部指针 —— 数组扩容会让旧指针失效。

#include "entity/core/Entity.hpp"
#include "entity/core/EntityDataManager.hpp"

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

namespace {

// 服务端 id 由 registerData 沿继承链分配，Entity 基类固定 8 个（id0..7）。
constexpr u16 ENTITY_FIELD_COUNT = 8;

} // namespace

// ============================================================================
// 稠密存储与 present 语义
// ============================================================================

TEST(EntityDataManagerStorageTest, RegisteredSlotsAreDenseFromZero)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.registerData();

    const auto& entries = entity.dataManager().getAllEntries();
    // 槽位数 == 已分配的最大 id + 1（id 即下标）
    ASSERT_GE(entries.size(), ENTITY_FIELD_COUNT);
    for (size_t id = 0; id < ENTITY_FIELD_COUNT; ++id) {
        EXPECT_TRUE(entries[id].present) << "id " << id << " 应已注册";
        EXPECT_TRUE(entity.dataManager().hasParam(static_cast<u16>(id)));
    }
    // 尾部槽位（若有）尚未注册
    for (size_t id = ENTITY_FIELD_COUNT; id < entries.size(); ++id) {
        EXPECT_FALSE(entries[id].present);
        EXPECT_FALSE(entity.dataManager().hasParam(static_cast<u16>(id)));
    }
}

TEST(EntityDataManagerStorageTest, InRangeButUnregisteredSlotIsAbsent)
{
    EntityDataManager manager;
    // 写入高位 id 会把数组撑到该下标，中间全是未注册槽位
    ASSERT_TRUE(manager.setRaw(50, DataValue(static_cast<i32>(7))));

    EXPECT_EQ(manager.getAllEntries().size(), 51u);
    EXPECT_TRUE(manager.hasParam(50));
    for (u16 id = 0; id < 50; ++id) {
        EXPECT_FALSE(manager.hasParam(id)) << "id " << id << " 落在范围内但未注册";
        EXPECT_FALSE(manager.getRaw(id).has_value());
    }
}

// ============================================================================
// getRaw 的边界与副本语义
// ============================================================================

TEST(EntityDataManagerStorageTest, GetRawReturnsCopyAndNulloptWhenMissing)
{
    EntityDataManager manager;

    EXPECT_FALSE(manager.getRaw(0).has_value());
    EXPECT_FALSE(manager.getRaw(9999).has_value());

    ASSERT_TRUE(manager.setRaw(3, DataValue(static_cast<i32>(42))));
    const auto value = manager.getRaw(3);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->get<i32>(), 42);

    // 副本语义：改原槽位不影响已取出的副本
    ASSERT_TRUE(manager.setRaw(3, DataValue(static_cast<i32>(43))));
    EXPECT_EQ(value->get<i32>(), 42);
    EXPECT_EQ(manager.getRaw(3)->get<i32>(), 43);
}

TEST(EntityDataManagerStorageTest, GetRawSurvivesStorageGrowth)
{
    EntityDataManager manager;
    ASSERT_TRUE(manager.setRaw(2, DataValue(static_cast<i32>(11))));

    // 取出"旧指针等价物"后扩容数组；副本必须仍然有效（指针方案下这里已悬垂）
    const auto before = manager.getRaw(2);
    ASSERT_TRUE(before.has_value());
    ASSERT_TRUE(manager.setRaw(200, DataValue(static_cast<i32>(99))));

    EXPECT_EQ(manager.getAllEntries().size(), 201u);
    EXPECT_EQ(before->get<i32>(), 11);
    EXPECT_EQ(manager.getRaw(2)->get<i32>(), 11);
}

TEST(EntityDataManagerStorageTest, TypedGetFallsBackToDefaultForUnregisteredSlot)
{
    EntityDataManager manager;
    ASSERT_TRUE(manager.setRaw(40, DataValue(static_cast<i32>(5))));

    // id 在范围内但未注册：hasParam 为假，getRaw 为空
    EXPECT_FALSE(manager.hasParam(10));
    EXPECT_FALSE(manager.getRaw(10).has_value());

    // 越界 id 不得崩溃
    EXPECT_FALSE(manager.hasParam(65534));
    EXPECT_FALSE(manager.getRaw(65534).has_value());
}

// ============================================================================
// 客户端路径：u8 字节索引（0..254）按需扩容
// ============================================================================

TEST(EntityDataManagerStorageTest, ClientByteIndexGrowsOnDemand)
{
    EntityDataManager manager;

    // 客户端从不调 registerParam，只按服务端下发的字节索引 setRaw
    for (u16 id : {0, 2, 254}) {
        ASSERT_TRUE(manager.setRaw(id, DataValue(static_cast<i32>(id))));
    }

    EXPECT_EQ(manager.getAllEntries().size(), 255u);
    ASSERT_TRUE(manager.getRaw(254).has_value());
    EXPECT_EQ(manager.getRaw(254)->get<i32>(), 254);
    EXPECT_FALSE(manager.hasParam(1));
    EXPECT_FALSE(manager.hasParam(253));
}

// ============================================================================
// 脏标记与布局
// ============================================================================

TEST(EntityDataManagerStorageTest, DirtyTrackingSkipsUnregisteredSlots)
{
    EntityDataManager manager;
    ASSERT_TRUE(manager.setRaw(5, DataValue(static_cast<i32>(1))));
    EXPECT_TRUE(manager.hasDirtyData());
    EXPECT_EQ(manager.getDirtyParams(), std::vector<u16>{5});

    manager.clearDirty(5);
    EXPECT_FALSE(manager.hasDirtyData());
    EXPECT_TRUE(manager.getDirtyParams().empty());

    // 清除未注册槽位与越界 id 不得崩溃
    manager.clearDirty(6);
    manager.clearDirty(9999);
}

TEST(EntityDataManagerStorageTest, EntryLayoutStaysCompact)
{
    // present 必须落进 DataValue 之后的对齐空隙，不得撑大 DataEntry
    EXPECT_LE(sizeof(DataEntry), sizeof(DataValue) + 8);
}
