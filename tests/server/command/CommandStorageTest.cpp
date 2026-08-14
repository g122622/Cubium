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

#include "common/BaseTestServer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/data/DataAccessor.hpp"

namespace mc::command {
namespace {

// ============================================================================
// CommandStorage 单元测试
// ============================================================================

TEST(CommandStorageTest, GetNonexistentReturnsEmpty)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:test");

    auto data = storage.get(id);
    ASSERT_NE(data, nullptr);
    EXPECT_TRUE(data->value.empty());
}

TEST(CommandStorageTest, SetAndGet)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:my_storage");

    nbt::tags::compound_tag data;
    data.put("key1", std::string("value1"));
    data.put("key2", 42);

    storage.set(id, data);

    auto result = storage.get(id);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->value.size(), 2u);

    auto it1 = result->value.find("key1");
    ASSERT_NE(it1, result->value.end());
    auto* str = dynamic_cast<const nbt::tags::string_tag*>(it1->second.get());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "value1");

    auto it2 = result->value.find("key2");
    ASSERT_NE(it2, result->value.end());
    auto* num = dynamic_cast<const nbt::tags::int_tag*>(it2->second.get());
    ASSERT_NE(num, nullptr);
    EXPECT_EQ(num->value, 42);
}

TEST(CommandStorageTest, SetOverwritesExisting)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:storage");

    nbt::tags::compound_tag data1;
    data1.put("version", 1);
    storage.set(id, data1);

    nbt::tags::compound_tag data2;
    data2.put("version", 2);
    storage.set(id, data2);

    auto result = storage.get(id);
    ASSERT_NE(result, nullptr);
    auto it = result->value.find("version");
    ASSERT_NE(it, result->value.end());
    auto* num = dynamic_cast<const nbt::tags::int_tag*>(it->second.get());
    ASSERT_NE(num, nullptr);
    EXPECT_EQ(num->value, 2);
}

TEST(CommandStorageTest, Exists)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:exists_test");

    EXPECT_FALSE(storage.exists(id));

    nbt::tags::compound_tag data;
    data.put("key", std::string("value"));
    storage.set(id, data);

    EXPECT_TRUE(storage.exists(id));
}

TEST(CommandStorageTest, Clear)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:clear_test");

    nbt::tags::compound_tag data;
    data.put("key", std::string("value"));
    storage.set(id, data);
    EXPECT_TRUE(storage.exists(id));

    storage.clear(id);
    EXPECT_FALSE(storage.exists(id));
}

TEST(CommandStorageTest, ListAll)
{
    CommandStorage storage;

    nbt::tags::compound_tag data;
    data.put("key", std::string("value"));

    storage.set(ResourceLocation("minecraft:a"), data);
    storage.set(ResourceLocation("minecraft:b"), data);
    storage.set(ResourceLocation("othermod:c"), data);

    auto list = storage.listAll();
    EXPECT_EQ(list.size(), 3u);
}

TEST(CommandStorageTest, DirtyFlag)
{
    CommandStorage storage;
    EXPECT_FALSE(storage.isDirty());

    ResourceLocation id("minecraft:dirty_test");
    nbt::tags::compound_tag data;
    data.put("key", std::string("value"));

    storage.set(id, data);
    EXPECT_TRUE(storage.isDirty());

    storage.markDirty(); // 手动标记
    EXPECT_TRUE(storage.isDirty());
}

TEST(CommandStorageTest, GetReturnsDeepCopy)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:copy_test");

    nbt::tags::compound_tag data;
    data.put("key", std::string("original"));
    storage.set(id, data);

    auto copy1 = storage.get(id);
    ASSERT_NE(copy1, nullptr);

    // 修改深拷贝不影响原始数据
    copy1->put("key", std::string("modified"));

    auto copy2 = storage.get(id);
    ASSERT_NE(copy2, nullptr);
    auto it = copy2->value.find("key");
    ASSERT_NE(it, copy2->value.end());
    auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "original");
}

TEST(CommandStorageTest, MultipleStoragesAreIndependent)
{
    CommandStorage storage1;
    CommandStorage storage2;

    ResourceLocation id("minecraft:shared_id");

    nbt::tags::compound_tag data;
    data.put("value", 100);
    storage1.set(id, data);

    // storage2 中不存在该 id
    EXPECT_TRUE(storage1.exists(id));
    EXPECT_FALSE(storage2.exists(id));
}

TEST(CommandStorageTest, SaveAndLoad)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:persist_test");

    nbt::tags::compound_tag data;
    data.put("name", std::string("test_storage"));
    data.put("count", 42);
    storage.set(id, data);

    // 保存到 JSON
    nlohmann::json json;
    storage.save(json);
    EXPECT_TRUE(storage.isDirty());

    // 加载到新的 storage
    CommandStorage loaded;
    loaded.load(json);
    EXPECT_FALSE(loaded.isDirty());

    auto result = loaded.get(id);
    ASSERT_NE(result, nullptr);
    auto it = result->value.find("name");
    ASSERT_NE(it, result->value.end());
    auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "test_storage");
}

// ============================================================================
// IServer::commandStorage() 集成测试
// ============================================================================

TEST(CommandStorageTest, BaseTestServerProvidesCommandStorage)
{
    mc::test::BaseTestServer server;
    auto& storage = server.commandStorage();
    const auto& constStorage = static_cast<const mc::test::BaseTestServer&>(server).commandStorage();

    // 验证非常量和常量版本引用同一对象
    EXPECT_EQ(&storage, &constStorage);

    // 验证基本功能可用
    ResourceLocation id("test:integration");
    nbt::tags::compound_tag data;
    data.put("test_key", std::string("test_value"));
    storage.set(id, data);

    auto result = storage.get(id);
    ASSERT_NE(result, nullptr);
    auto it = result->value.find("test_key");
    ASSERT_NE(it, result->value.end());
    auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "test_value");
}

TEST(CommandStorageTest, StorageDataAccessorWithCommandStorage)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:accessor_test");

    // 先写入数据
    nbt::tags::compound_tag data;
    data.put("name", std::string("hello"));
    storage.set(id, data);

    // 通过 StorageDataAccessor 读取
    StorageDataAccessor accessor(&storage, id);

    auto readData = accessor.getData();
    ASSERT_NE(readData, nullptr);
    auto it = readData->value.find("name");
    ASSERT_NE(it, readData->value.end());
    auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str->value, "hello");
}

TEST(CommandStorageTest, StorageDataAccessorMergeData)
{
    CommandStorage storage;
    ResourceLocation id("minecraft:merge_test");

    // 写入初始数据
    nbt::tags::compound_tag data;
    data.put("key1", std::string("value1"));
    storage.set(id, data);

    // 通过 StorageDataAccessor 合并数据
    StorageDataAccessor accessor(&storage, id);
    nbt::tags::compound_tag mergeData;
    mergeData.put("key2", std::string("value2"));
    accessor.mergeData(mergeData);

    // 验证合并结果
    auto result = storage.get(id);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->value.size(), 2u);

    auto it1 = result->value.find("key1");
    ASSERT_NE(it1, result->value.end());
    auto* key1 = dynamic_cast<const nbt::tags::string_tag*>(it1->second.get());
    ASSERT_NE(key1, nullptr);
    EXPECT_EQ(key1->value, "value1");

    auto it2 = result->value.find("key2");
    ASSERT_NE(it2, result->value.end());
    auto* key2 = dynamic_cast<const nbt::tags::string_tag*>(it2->second.get());
    ASSERT_NE(key2, nullptr);
    EXPECT_EQ(key2->value, "value2");
}

} // namespace
} // namespace mc::command
