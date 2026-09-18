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

#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::nbt;
using namespace mc::nbt::tags;

// ========== nbtToJson 测试 ==========

class NbtToJsonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NbtToJsonTest, ByteTag)
{
    byte_tag tag(static_cast<i8>(42));
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_EQ(json.get<i8>(), 42);
}

TEST_F(NbtToJsonTest, ShortTag)
{
    short_tag tag(static_cast<i16>(1000));
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_EQ(json.get<i16>(), 1000);
}

TEST_F(NbtToJsonTest, IntTag)
{
    int_tag tag(static_cast<i32>(100000));
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_EQ(json.get<i32>(), 100000);
}

TEST_F(NbtToJsonTest, LongTag)
{
    long_tag tag(static_cast<i64>(9999999999LL));
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_EQ(json.get<i64>(), 9999999999LL);
}

TEST_F(NbtToJsonTest, FloatTag)
{
    float_tag tag(3.14f);
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_DOUBLE_EQ(json.get<double>(), static_cast<double>(3.14f));
}

TEST_F(NbtToJsonTest, DoubleTag)
{
    double_tag tag(2.718281828);
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_number());
    EXPECT_DOUBLE_EQ(json.get<double>(), 2.718281828);
}

TEST_F(NbtToJsonTest, StringTag)
{
    string_tag tag("hello world");
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_string());
    EXPECT_EQ(json.get<std::string>(), "hello world");
}

TEST_F(NbtToJsonTest, ByteArrayTag)
{
    bytearray_tag tag;
    tag.value = {1, 2, 3, -1, -128, 127};
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 6u);
    EXPECT_EQ(json[0].get<i8>(), 1);
    EXPECT_EQ(json[5].get<i8>(), 127);
}

TEST_F(NbtToJsonTest, IntArrayTag)
{
    intarray_tag tag;
    tag.value = {100, 200, 300};
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 3u);
    EXPECT_EQ(json[1].get<i32>(), 200);
}

TEST_F(NbtToJsonTest, LongArrayTag)
{
    longarray_tag tag;
    tag.value = {1000LL, 2000LL, 3000LL};
    auto json = nbtToJson(tag);
    ASSERT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 3u);
    EXPECT_EQ(json[2].get<i64>(), 3000LL);
}

TEST_F(NbtToJsonTest, CompoundTag)
{
    auto compound = std::make_unique<compound_tag>();
    compound->value.emplace("name", std::make_unique<string_tag>("test"));
    compound->value.emplace("count", std::make_unique<int_tag>(42));
    compound->value.emplace("active", std::make_unique<byte_tag>(1));

    auto json = nbtToJson(*compound);
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["name"].get<std::string>(), "test");
    EXPECT_EQ(json["count"].get<i32>(), 42);
    EXPECT_EQ(json["active"].get<i8>(), 1);
}

TEST_F(NbtToJsonTest, NestedCompoundTag)
{
    auto inner = std::make_unique<compound_tag>();
    inner->value.emplace("value", std::make_unique<int_tag>(10));

    auto outer = std::make_unique<compound_tag>();
    outer->value.emplace("inner", std::move(inner));

    auto json = nbtToJson(*outer);
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json["inner"].is_object());
    EXPECT_EQ(json["inner"]["value"].get<i32>(), 10);
}

TEST_F(NbtToJsonTest, IntListTag)
{
    auto list = std::make_unique<int_list_tag>();
    list->value = {1, 2, 3, 4, 5};

    auto json = nbtToJson(*list);
    ASSERT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 5u);
    EXPECT_EQ(json[0].get<i32>(), 1);
    EXPECT_EQ(json[4].get<i32>(), 5);
}

TEST_F(NbtToJsonTest, StringListTag)
{
    auto list = std::make_unique<string_list_tag>();
    list->value = {"alpha", "beta", "gamma"};

    auto json = nbtToJson(*list);
    ASSERT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 3u);
    EXPECT_EQ(json[1].get<std::string>(), "beta");
}

TEST_F(NbtToJsonTest, EndTagReturnsNull)
{
    end_tag tag;
    auto json = nbtToJson(tag);
    EXPECT_TRUE(json.is_null());
}

// ========== jsonToNbt 测试 ==========

class JsonToNbtTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(JsonToNbtTest, NullReturnsNullptr)
{
    nlohmann::json json = nullptr;
    auto result = jsonToNbt(json);
    EXPECT_EQ(result, nullptr);
}

TEST_F(JsonToNbtTest, NonObjectReturnsNullptr)
{
    nlohmann::json json = 42;
    auto result = jsonToNbt(json);
    EXPECT_EQ(result, nullptr);
}

TEST_F(JsonToNbtTest, EmptyObjectReturnsEmptyCompound)
{
    nlohmann::json json = nlohmann::json::object();
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->value.empty());
}

TEST_F(JsonToNbtTest, SimpleObjectWithInt)
{
    nlohmann::json json = {{"count", 42}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("count") > 0);
    auto* tag = result->value.at("count").get();
    ASSERT_NE(tag, nullptr);
    // 42 fits in byte range, so it should be byte_tag
    EXPECT_EQ(tag->id(), TagId::Byte);
}

TEST_F(JsonToNbtTest, SimpleObjectWithLargeInt)
{
    nlohmann::json json = {{"value", 100000}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("value") > 0);
    auto* tag = result->value.at("value").get();
    ASSERT_NE(tag, nullptr);
    // 100000 fits in int range
    EXPECT_EQ(tag->id(), TagId::Int);
}

TEST_F(JsonToNbtTest, SimpleObjectWithString)
{
    nlohmann::json json = {{"name", "hello"}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("name") > 0);
    auto* tag = result->value.at("name").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::String);
    auto& strTag = static_cast<const string_tag&>(*tag);
    EXPECT_EQ(strTag.value, "hello");
}

TEST_F(JsonToNbtTest, SimpleObjectWithFloat)
{
    nlohmann::json json = {{"ratio", 3.14}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("ratio") > 0);
    auto* tag = result->value.at("ratio").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->id() == TagId::Float || tag->id() == TagId::Double);
}

TEST_F(JsonToNbtTest, SimpleObjectWithBool)
{
    nlohmann::json json = {{"active", true}, {"disabled", false}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("active") > 0);
    auto* activeTag = result->value.at("active").get();
    ASSERT_NE(activeTag, nullptr);
    EXPECT_EQ(activeTag->id(), TagId::Byte);
    auto& activeByte = static_cast<const byte_tag&>(*activeTag);
    EXPECT_EQ(activeByte.value, 1);

    ASSERT_TRUE(result->value.count("disabled") > 0);
    auto* disabledTag = result->value.at("disabled").get();
    ASSERT_NE(disabledTag, nullptr);
    auto& disabledByte = static_cast<const byte_tag&>(*disabledTag);
    EXPECT_EQ(disabledByte.value, 0);
}

TEST_F(JsonToNbtTest, NestedObject)
{
    nlohmann::json json = {{"outer", {{"inner", {{"value", 42}}}}}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("outer") > 0);
    auto* outerTag = result->value.at("outer").get();
    ASSERT_NE(outerTag, nullptr);
    EXPECT_EQ(outerTag->id(), TagId::Compound);
}

TEST_F(JsonToNbtTest, ArrayOfInts)
{
    nlohmann::json json = {{"numbers", {1, 2, 3, 4, 5}}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("numbers") > 0);
    auto* arrTag = result->value.at("numbers").get();
    ASSERT_NE(arrTag, nullptr);
    EXPECT_EQ(arrTag->id(), TagId::List);
}

TEST_F(JsonToNbtTest, ArrayOfStrings)
{
    nlohmann::json json = {{"names", {"alpha", "beta", "gamma"}}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("names") > 0);
    auto* arrTag = result->value.at("names").get();
    ASSERT_NE(arrTag, nullptr);
    EXPECT_EQ(arrTag->id(), TagId::List);
}

TEST_F(JsonToNbtTest, EmptyArray)
{
    nlohmann::json json = {{"empty", nlohmann::json::array()}};
    auto result = jsonToNbt(json);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("empty") > 0);
    auto* arrTag = result->value.at("empty").get();
    ASSERT_NE(arrTag, nullptr);
    EXPECT_EQ(arrTag->id(), TagId::List);
}

// ========== parseMojangson 测试 ==========

class ParseMojangsonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ParseMojangsonTest, EmptyStringReturnsNullptr)
{
    auto result = parseMojangson("");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ParseMojangsonTest, InvalidStringReturnsNullptr)
{
    auto result = parseMojangson("not valid nbt");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ParseMojangsonTest, SimpleCompound)
{
    auto result = parseMojangson(R"({name:"Test"})");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("name") > 0);
    auto* tag = result->value.at("name").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::String);
    auto& strTag = static_cast<const string_tag&>(*tag);
    EXPECT_EQ(strTag.value, "Test");
}

TEST_F(ParseMojangsonTest, CompoundWithInt)
{
    auto result = parseMojangson(R"({count:42})");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("count") > 0);
}

TEST_F(ParseMojangsonTest, CompoundWithMultipleEntries)
{
    auto result = parseMojangson(R"({name:"Test",count:42,active:1b})");
    ASSERT_NE(result, nullptr);
    EXPECT_GE(result->value.size(), 3u);
    EXPECT_TRUE(result->value.count("name") > 0);
    EXPECT_TRUE(result->value.count("count") > 0);
    EXPECT_TRUE(result->value.count("active") > 0);
}

TEST_F(ParseMojangsonTest, NestedCompound)
{
    auto result = parseMojangson(R"({outer:{inner:10}})");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("outer") > 0);
    auto* outerTag = result->value.at("outer").get();
    ASSERT_NE(outerTag, nullptr);
    EXPECT_EQ(outerTag->id(), TagId::Compound);
}

TEST_F(ParseMojangsonTest, CompoundWithList)
{
    auto result = parseMojangson(R"({items:["a","b","c"]})");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("items") > 0);
    auto* itemsTag = result->value.at("items").get();
    ASSERT_NE(itemsTag, nullptr);
    EXPECT_EQ(itemsTag->id(), TagId::List);
}

TEST_F(ParseMojangsonTest, CompoundWithByteTag)
{
    auto result = parseMojangson("{flag:1b}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("flag") > 0);
    auto* tag = result->value.at("flag").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::Byte);
}

TEST_F(ParseMojangsonTest, CompoundWithShortTag)
{
    auto result = parseMojangson("{shortVal:100s}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("shortVal") > 0);
    auto* tag = result->value.at("shortVal").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::Short);
}

TEST_F(ParseMojangsonTest, CompoundWithLongTag)
{
    auto result = parseMojangson("{longVal:9999999999l}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("longVal") > 0);
    auto* tag = result->value.at("longVal").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::Long);
}

TEST_F(ParseMojangsonTest, CompoundWithFloatTag)
{
    auto result = parseMojangson("{ratio:3.14f}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("ratio") > 0);
    auto* tag = result->value.at("ratio").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::Float);
}

TEST_F(ParseMojangsonTest, CompoundWithDoubleTag)
{
    auto result = parseMojangson("{ratio:3.14d}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("ratio") > 0);
    auto* tag = result->value.at("ratio").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::Double);
}

TEST_F(ParseMojangsonTest, UnquotedStringValue)
{
    auto result = parseMojangson("{name:Test}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->value.count("name") > 0);
    auto* tag = result->value.at("name").get();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), TagId::String);
    auto& strTag = static_cast<const string_tag&>(*tag);
    EXPECT_EQ(strTag.value, "Test");
}

// ========== Round-trip 测试 ==========

class NbtJsonRoundTripTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NbtJsonRoundTripTest, SimpleCompoundRoundTrip)
{
    auto original = std::make_unique<compound_tag>();
    original->value.emplace("name", std::make_unique<string_tag>("hello"));
    original->value.emplace("count", std::make_unique<int_tag>(42));

    nlohmann::json json = nbtToJson(*original);
    EXPECT_TRUE(json.is_object());
    EXPECT_EQ(json["name"].get<std::string>(), "hello");

    auto restored = jsonToNbt(json);
    ASSERT_NE(restored, nullptr);
    ASSERT_TRUE(restored->value.count("name") > 0);
    auto* nameTag = restored->value.at("name").get();
    ASSERT_NE(nameTag, nullptr);
    EXPECT_EQ(nameTag->id(), TagId::String);
    auto& strTag = static_cast<const string_tag&>(*nameTag);
    EXPECT_EQ(strTag.value, "hello");
}

TEST_F(NbtJsonRoundTripTest, NestedCompoundRoundTrip)
{
    auto inner = std::make_unique<compound_tag>();
    inner->value.emplace("value", std::make_unique<int_tag>(99));

    auto outer = std::make_unique<compound_tag>();
    outer->value.emplace("inner", std::move(inner));
    outer->value.emplace("name", std::make_unique<string_tag>("outer"));

    nlohmann::json json = nbtToJson(*outer);
    EXPECT_TRUE(json.is_object());

    auto restored = jsonToNbt(json);
    ASSERT_NE(restored, nullptr);
    ASSERT_TRUE(restored->value.count("name") > 0);
    ASSERT_TRUE(restored->value.count("inner") > 0);
}

TEST_F(NbtJsonRoundTripTest, ListRoundTrip)
{
    auto list = std::make_unique<int_list_tag>();
    list->value = {10, 20, 30};

    nlohmann::json json = nbtToJson(*list);
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 3u);

    // jsonToNbt converts a top-level JSON array, but it needs to be inside an object
    // Let's test it inside a compound
    auto compound = std::make_unique<compound_tag>();
    compound->value.emplace("items", std::move(list));
    nlohmann::json compoundJson = nbtToJson(*compound);

    auto restored = jsonToNbt(compoundJson);
    ASSERT_NE(restored, nullptr);
    ASSERT_TRUE(restored->value.count("items") > 0);
    auto* itemsTag = restored->value.at("items").get();
    ASSERT_NE(itemsTag, nullptr);
    EXPECT_EQ(itemsTag->id(), TagId::List);
}
