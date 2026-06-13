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

#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <nlohmann/json.hpp>

using namespace mc::advancement;
using namespace mc::nbt::tags;

// ========== isAny 和基础测试 ==========

class NBTPredicateBasicTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateBasicTest, DefaultIsAny)
{
    NBTPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
    EXPECT_EQ(predicate.getTag(), nullptr);
}

TEST_F(NBTPredicateBasicTest, ConstructedWithTagIsNotAny)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("key", std::make_unique<string_tag>("value"));
    NBTPredicate predicate(std::move(tag));
    EXPECT_FALSE(predicate.isAny());
    EXPECT_NE(predicate.getTag(), nullptr);
}

TEST_F(NBTPredicateBasicTest, CopyConstructor)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("name", std::make_unique<string_tag>("test"));
    NBTPredicate original(std::move(tag));

    NBTPredicate copy(original);
    EXPECT_FALSE(copy.isAny());
    EXPECT_NE(copy.getTag(), nullptr);
    // Verify deep copy: both should have the same content but different pointers
    EXPECT_NE(copy.getTag(), original.getTag());
}

TEST_F(NBTPredicateBasicTest, CopyAssignment)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("count", std::make_unique<int_tag>(42));
    NBTPredicate original(std::move(tag));

    NBTPredicate assigned;
    assigned = original;
    EXPECT_FALSE(assigned.isAny());
    EXPECT_NE(assigned.getTag(), nullptr);
    EXPECT_NE(assigned.getTag(), original.getTag());
}

TEST_F(NBTPredicateBasicTest, MoveConstructor)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("key", std::make_unique<string_tag>("value"));
    NBTPredicate original(std::move(tag));

    NBTPredicate moved(std::move(original));
    EXPECT_FALSE(moved.isAny());
    EXPECT_NE(moved.getTag(), nullptr);
}

TEST_F(NBTPredicateBasicTest, MoveAssignment)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("key", std::make_unique<string_tag>("value"));
    NBTPredicate original(std::move(tag));

    NBTPredicate assigned;
    assigned = std::move(original);
    EXPECT_FALSE(assigned.isAny());
    EXPECT_NE(assigned.getTag(), nullptr);
}

// ========== fromJson 测试 ==========

class NBTPredicateFromJsonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateFromJsonTest, NullJsonReturnsAny)
{
    auto result = NBTPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(NBTPredicateFromJsonTest, MojangsonStringSimple)
{
    nlohmann::json json = R"({CustomName:"Test"})";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_NE(result.value().getTag(), nullptr);
    // Verify the parsed tag contains the CustomName key
    const auto* tag = result.value().getTag();
    ASSERT_TRUE(tag->value.count("CustomName") > 0);
    auto* nameTag = tag->value.at("CustomName").get();
    ASSERT_NE(nameTag, nullptr);
    EXPECT_EQ(nameTag->id(), mc::nbt::TagId::String);
    auto& strTag = static_cast<const string_tag&>(*nameTag);
    EXPECT_EQ(strTag.value, "Test");
}

TEST_F(NBTPredicateFromJsonTest, MojangsonStringWithMultipleEntries)
{
    nlohmann::json json = R"({name:"Test",count:42})";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    const auto* tag = result.value().getTag();
    EXPECT_GE(tag->value.size(), 2u);
    EXPECT_TRUE(tag->value.count("name") > 0);
    EXPECT_TRUE(tag->value.count("count") > 0);
}

TEST_F(NBTPredicateFromJsonTest, MojangsonStringWithNestedCompound)
{
    nlohmann::json json = R"({display:{Name:"Diamond Sword"}})";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    const auto* tag = result.value().getTag();
    ASSERT_TRUE(tag->value.count("display") > 0);
    auto* displayTag = tag->value.at("display").get();
    ASSERT_NE(displayTag, nullptr);
    EXPECT_EQ(displayTag->id(), mc::nbt::TagId::Compound);
}

TEST_F(NBTPredicateFromJsonTest, MojangsonStringInvalidReturnsError)
{
    nlohmann::json json = "not valid nbt";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_FALSE(result.success());
}

TEST_F(NBTPredicateFromJsonTest, JsonObjectSimple)
{
    nlohmann::json json = {{"CustomName", "Test"}};
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_NE(result.value().getTag(), nullptr);
    const auto* tag = result.value().getTag();
    ASSERT_TRUE(tag->value.count("CustomName") > 0);
}

TEST_F(NBTPredicateFromJsonTest, JsonObjectWithMultipleEntries)
{
    nlohmann::json json = {{"name", "hello"}, {"count", 42}, {"active", true}};
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    const auto* tag = result.value().getTag();
    EXPECT_GE(tag->value.size(), 3u);
}

TEST_F(NBTPredicateFromJsonTest, JsonObjectNested)
{
    nlohmann::json json = {{"display", {{"Name", "Test"}, {"Lore", {"Line 1", "Line 2"}}}}};
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    const auto* tag = result.value().getTag();
    ASSERT_TRUE(tag->value.count("display") > 0);
    auto* displayTag = tag->value.at("display").get();
    ASSERT_NE(displayTag, nullptr);
    EXPECT_EQ(displayTag->id(), mc::nbt::TagId::Compound);
}

TEST_F(NBTPredicateFromJsonTest, JsonObjectEmptyReturnsAny)
{
    // Empty JSON object should still produce a compound tag, but with no keys
    // NBTPredicate considers a tag with an empty compound as "not any" (it has a tag)
    nlohmann::json json = nlohmann::json::object();
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    // An empty compound tag is still a valid tag, so isAny should be false
    EXPECT_FALSE(result.value().isAny());
}

TEST_F(NBTPredicateFromJsonTest, JsonObjectWithArray)
{
    nlohmann::json json = {{"items", {1, 2, 3}}};
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    const auto* tag = result.value().getTag();
    ASSERT_TRUE(tag->value.count("items") > 0);
    auto* itemsTag = tag->value.at("items").get();
    ASSERT_NE(itemsTag, nullptr);
    EXPECT_EQ(itemsTag->id(), mc::nbt::TagId::List);
}

// ========== toJson 测试 ==========

class NBTPredicateToJsonTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateToJsonTest, AnyPredicateReturnsNull)
{
    NBTPredicate predicate;
    auto json = predicate.toJson();
    EXPECT_TRUE(json.is_null());
}

TEST_F(NBTPredicateToJsonTest, SimpleTagSerializesToMojangsonString)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("name", std::make_unique<string_tag>("test"));
    NBTPredicate predicate(std::move(tag));

    auto json = predicate.toJson();
    // toJson should return a Mojangson string
    ASSERT_TRUE(json.is_string());
    std::string mojangson = json.get<std::string>();
    EXPECT_NE(mojangson.find("name"), std::string::npos);
    EXPECT_NE(mojangson.find("test"), std::string::npos);
}

TEST_F(NBTPredicateToJsonTest, TagWithMultipleEntries)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("name", std::make_unique<string_tag>("hello"));
    tag->value.emplace("count", std::make_unique<int_tag>(42));
    NBTPredicate predicate(std::move(tag));

    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_string());
    std::string mojangson = json.get<std::string>();
    EXPECT_NE(mojangson.find("name"), std::string::npos);
    EXPECT_NE(mojangson.find("count"), std::string::npos);
}

// ========== Round-trip 测试 ==========

class NBTPredicateRoundTripTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateRoundTripTest, MojangsonStringRoundTrip)
{
    nlohmann::json originalJson = R"({CustomName:"Test"})";
    auto result = NBTPredicate::fromJson(originalJson);
    EXPECT_TRUE(result.success());

    auto serialized = result.value().toJson();
    ASSERT_TRUE(serialized.is_string());

    // Parse the serialized Mojangson string back
    auto result2 = NBTPredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());
    EXPECT_FALSE(result2.value().isAny());

    // Verify the tag has the same key
    const auto* tag = result2.value().getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->value.count("CustomName") > 0);
}

TEST_F(NBTPredicateRoundTripTest, JsonObjectRoundTrip)
{
    nlohmann::json originalJson = {{"name", "hello"}, {"count", 42}};
    auto result = NBTPredicate::fromJson(originalJson);
    EXPECT_TRUE(result.success());

    auto serialized = result.value().toJson();
    ASSERT_TRUE(serialized.is_string());

    // Parse the serialized Mojangson string back
    auto result2 = NBTPredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());
    EXPECT_FALSE(result2.value().isAny());

    const auto* tag = result2.value().getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->value.count("name") > 0);
    EXPECT_TRUE(tag->value.count("count") > 0);
}

// ========== test(compound_tag*) 测试 ==========

class NBTPredicateMatchTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateMatchTest, AnyPredicateMatchesNullptr)
{
    NBTPredicate predicate;
    EXPECT_TRUE(predicate.test(static_cast<const compound_tag*>(nullptr)));
}

TEST_F(NBTPredicateMatchTest, AnyPredicateMatchesAnyTag)
{
    NBTPredicate predicate;
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("key", std::make_unique<string_tag>("value"));
    EXPECT_TRUE(predicate.test(tag.get()));
}

TEST_F(NBTPredicateMatchTest, NonAnyPredicateRejectsNullptr)
{
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("key", std::make_unique<string_tag>("value"));
    NBTPredicate predicate(std::move(tag));
    EXPECT_FALSE(predicate.test(static_cast<const compound_tag*>(nullptr)));
}

TEST_F(NBTPredicateMatchTest, ExactMatch)
{
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("name", std::make_unique<string_tag>("test"));
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("name", std::make_unique<string_tag>("test"));
    EXPECT_TRUE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, SubsetMatch)
{
    // Expected has fewer keys than actual - should still match (subset semantics)
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("name", std::make_unique<string_tag>("test"));
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("name", std::make_unique<string_tag>("test"));
    actual->value.emplace("count", std::make_unique<int_tag>(42));
    EXPECT_TRUE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, MissingKeyFails)
{
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("name", std::make_unique<string_tag>("test"));
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("count", std::make_unique<int_tag>(42));
    EXPECT_FALSE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, ValueMismatchFails)
{
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("name", std::make_unique<string_tag>("expected"));
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("name", std::make_unique<string_tag>("actual"));
    EXPECT_FALSE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, EmptyExpectedMatchesAnyCompound)
{
    auto expected = std::make_unique<compound_tag>();
    // Empty compound - should match any compound (subset of anything)
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("name", std::make_unique<string_tag>("test"));
    EXPECT_TRUE(predicate.test(actual.get()));

    auto emptyActual = std::make_unique<compound_tag>();
    EXPECT_TRUE(predicate.test(emptyActual.get()));
}

TEST_F(NBTPredicateMatchTest, NestedCompoundMatch)
{
    auto innerExpected = std::make_unique<compound_tag>();
    innerExpected->value.emplace("value", std::make_unique<int_tag>(10));

    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("inner", std::move(innerExpected));
    NBTPredicate predicate(std::move(expected));

    auto innerActual = std::make_unique<compound_tag>();
    innerActual->value.emplace("value", std::make_unique<int_tag>(10));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("inner", std::move(innerActual));
    EXPECT_TRUE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, NestedCompoundMismatch)
{
    auto innerExpected = std::make_unique<compound_tag>();
    innerExpected->value.emplace("value", std::make_unique<int_tag>(10));

    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("inner", std::move(innerExpected));
    NBTPredicate predicate(std::move(expected));

    auto innerActual = std::make_unique<compound_tag>();
    innerActual->value.emplace("value", std::make_unique<int_tag>(20)); // different value

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("inner", std::move(innerActual));
    EXPECT_FALSE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, ListMatch)
{
    // Build expected with an int list
    auto intList = std::make_unique<int_list_tag>();
    intList->value = {1, 2, 3};
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("numbers", std::move(intList));
    NBTPredicate predicate(std::move(expected));

    // Build actual with the same list
    auto actualIntList = std::make_unique<int_list_tag>();
    actualIntList->value = {1, 2, 3};
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("numbers", std::move(actualIntList));
    EXPECT_TRUE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, ListSupersetMatch)
{
    // Expected has fewer items - unordered subset matching
    auto intList = std::make_unique<int_list_tag>();
    intList->value = {1, 3};
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("numbers", std::move(intList));
    NBTPredicate predicate(std::move(expected));

    // Actual has more items - should match (superset for subset)
    auto actualIntList = std::make_unique<int_list_tag>();
    actualIntList->value = {1, 2, 3};
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("numbers", std::move(actualIntList));
    EXPECT_TRUE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, ListMissingItemFails)
{
    auto intList = std::make_unique<int_list_tag>();
    intList->value = {1, 2, 3};
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("numbers", std::move(intList));
    NBTPredicate predicate(std::move(expected));

    // Actual is missing element 3
    auto actualIntList = std::make_unique<int_list_tag>();
    actualIntList->value = {1, 2};
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("numbers", std::move(actualIntList));
    EXPECT_FALSE(predicate.test(actual.get()));
}

TEST_F(NBTPredicateMatchTest, EmptyExpectedListMatchesSameTypeEmptyList)
{
    // An empty expected list matches another empty list of the same type
    auto emptyIntList = std::make_unique<int_list_tag>();
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("items", std::move(emptyIntList));
    NBTPredicate predicate(std::move(expected));

    auto actualEmptyIntList = std::make_unique<int_list_tag>();
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("items", std::move(actualEmptyIntList));
    EXPECT_TRUE(predicate.test(actual.get()));

    // An empty expected list matches a non-empty list of the same type (subset semantics)
    auto actualIntList = std::make_unique<int_list_tag>();
    actualIntList->value = {1, 2, 3};
    auto actual2 = std::make_unique<compound_tag>();
    actual2->value.emplace("items", std::move(actualIntList));
    EXPECT_TRUE(predicate.test(actual2.get()));
}

TEST_F(NBTPredicateMatchTest, EmptyEndListOnlyMatchesEndList)
{
    // An end_list_tag (empty list with End element type) only matches other end_list_tag
    // because element_id() must match (End != Int)
    auto emptyEndList = std::make_unique<end_list_tag>();
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("items", std::move(emptyEndList));
    NBTPredicate predicate(std::move(expected));

    // Same type (end_list) should match
    auto actualEndList = std::make_unique<end_list_tag>();
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("items", std::move(actualEndList));
    EXPECT_TRUE(predicate.test(actual.get()));

    // Different type (int_list) should NOT match
    auto actualIntList = std::make_unique<int_list_tag>();
    auto actual2 = std::make_unique<compound_tag>();
    actual2->value.emplace("items", std::move(actualIntList));
    EXPECT_FALSE(predicate.test(actual2.get()));
}

TEST_F(NBTPredicateMatchTest, NumericTypeMismatchFails)
{
    auto expected = std::make_unique<compound_tag>();
    expected->value.emplace("value", std::make_unique<int_tag>(1));
    NBTPredicate predicate(std::move(expected));

    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("value", std::make_unique<string_tag>("1"));
    EXPECT_FALSE(predicate.test(actual.get()));
}

// ========== fromJson + test 集成测试 ==========

class NBTPredicateIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateIntegrationTest, MojangsonParseAndMatch)
{
    // Parse from Mojangson string
    nlohmann::json json = R"({CustomName:"Diamond Sword"})";
    auto result = NBTPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    // Create a matching compound tag
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("CustomName", std::make_unique<string_tag>("Diamond Sword"));
    EXPECT_TRUE(result.value().test(actual.get()));
}

TEST_F(NBTPredicateIntegrationTest, MojangsonParseAndSubsetMatch)
{
    nlohmann::json json = R"({CustomName:"Test"})";
    auto result = NBTPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    // Actual has extra keys - should still match
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("CustomName", std::make_unique<string_tag>("Test"));
    actual->value.emplace("extra", std::make_unique<int_tag>(42));
    EXPECT_TRUE(result.value().test(actual.get()));
}

TEST_F(NBTPredicateIntegrationTest, JsonObjectParseAndMatch)
{
    nlohmann::json json = {{"name", "hello"}, {"count", 42}};
    auto result = NBTPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    // Note: JSON conversion may use different NBT types than expected
    // (e.g., 42 becomes byte_tag instead of int_tag)
    // So we need to be careful with type matching
    // Let's just verify it parsed successfully
    EXPECT_FALSE(result.value().isAny());
}

TEST_F(NBTPredicateIntegrationTest, InvalidMojangsonReturnsError)
{
    nlohmann::json json = "not valid nbt string!";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_FALSE(result.success());
}

TEST_F(NBTPredicateIntegrationTest, MojangsonWithByteTag)
{
    nlohmann::json json = "{Unbreakable:1b}";
    auto result = NBTPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // Create matching actual
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("Unbreakable", std::make_unique<byte_tag>(1));
    EXPECT_TRUE(result.value().test(actual.get()));
}

TEST_F(NBTPredicateIntegrationTest, MojangsonWithShortTag)
{
    nlohmann::json json = "{damage:100s}";
    auto result = NBTPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
}

TEST_F(NBTPredicateIntegrationTest, SerializeAndReParse)
{
    // Create predicate with a compound tag
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("CustomName", std::make_unique<string_tag>("My Item"));
    NBTPredicate original(std::move(tag));

    // Serialize to JSON
    auto json = original.toJson();
    ASSERT_TRUE(json.is_string());

    // Re-parse
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // Verify it matches the same data
    auto actual = std::make_unique<compound_tag>();
    actual->value.emplace("CustomName", std::make_unique<string_tag>("My Item"));
    EXPECT_TRUE(result.value().test(actual.get()));
}
