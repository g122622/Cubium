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

#include <memory>
#include <string>
#include <vector>

#include "common/util/nbt/Nbt.hpp"

using namespace mc;

namespace {

using nbt::tags::byte_tag;
using nbt::tags::compound_list_tag;
using nbt::tags::compound_tag;
using nbt::tags::end_list_tag;
using nbt::tags::int_tag;
using nbt::tags::list_list_tag;
using nbt::tags::list_tag;
using nbt::tags::string_list_tag;
using nbt::tags::string_tag;
using nbt::tags::tag;
using nbt::tags::tag_list_tag;

} // namespace

/**
 * @brief `list_tag` 按下标增删改虚接口的测试
 *
 * 这些操作必须对所有元素存储形态一致可用：定长数值列表、数组列表、字符串列表、
 * 复合列表、嵌套列表、任意标签列表、空列表。调用方（NbtPath 等）只持有 `list_tag*`，
 * 不允许再 `dynamic_cast` 到具体类型。
 */
class ListTagIndexOpsTest : public ::testing::Test {
protected:
    /// 用统一的只读视图比较元素内容，不依赖具体容器的元素类型。
    static std::vector<std::string> intValues(const list_tag& list)
    {
        std::vector<std::string> values;
        for (size_t i = 0; i < list.size(); ++i) {
            values.push_back(std::to_string(dynamic_cast<const int_tag&>(*list[i]).value));
        }
        return values;
    }
};

// ============================================================================
// removeAt
// ============================================================================

TEST_F(ListTagIndexOpsTest, RemoveAtErasesElementForNumericList)
{
    nbt::tags::int_list_tag list(std::vector<std::int32_t>{10, 20, 30});

    EXPECT_TRUE(list.removeAt(1));
    EXPECT_EQ(intValues(list), (std::vector<std::string>{"10", "30"}));
    EXPECT_EQ(list.element_id(), nbt::TagId::Int);
}

TEST_F(ListTagIndexOpsTest, RemoveAtRejectsOutOfRange)
{
    nbt::tags::int_list_tag list(std::vector<std::int32_t>{10});
    EXPECT_FALSE(list.removeAt(1));
    EXPECT_EQ(list.size(), 1u);
}

TEST_F(ListTagIndexOpsTest, RemoveAtWorksForTagList)
{
    tag_list_tag list;
    list.appendCopy(int_tag(1));
    list.appendCopy(string_tag(std::string("two")));

    ASSERT_EQ(list.size(), 2u);
    EXPECT_TRUE(list.removeAt(0));
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->id(), nbt::TagId::String);
}

TEST_F(ListTagIndexOpsTest, RemoveAtWorksForNestedAndCompoundLists)
{
    list_list_tag nested;
    nested.value.push_back(std::make_unique<nbt::tags::int_list_tag>(std::vector<std::int32_t>{1}));
    nested.value.push_back(std::make_unique<nbt::tags::int_list_tag>(std::vector<std::int32_t>{2}));
    ASSERT_TRUE(nested.removeAt(0));
    EXPECT_EQ(nested.size(), 1u);

    compound_list_tag compounds;
    compound_tag first;
    first.put("k", std::string("v"));
    compounds.appendCopy(first);
    ASSERT_EQ(compounds.size(), 1u);
    EXPECT_TRUE(compounds.removeAt(0));
    EXPECT_TRUE(compounds.value.empty());
}

// ============================================================================
// clearAll
// ============================================================================

TEST_F(ListTagIndexOpsTest, ClearAllReportsWhetherAnythingRemoved)
{
    nbt::tags::int_list_tag list(std::vector<std::int32_t>{1, 2, 3});
    EXPECT_TRUE(list.clearAll());
    EXPECT_EQ(list.size(), 0u);
    // 已经为空时不再报告"发生了删除"
    EXPECT_FALSE(list.clearAll());
}

TEST_F(ListTagIndexOpsTest, ClearAllOnEmptyEndListDoesNothing)
{
    end_list_tag empty;
    EXPECT_FALSE(empty.clearAll());
    EXPECT_EQ(empty.size(), 0u);
}

// ============================================================================
// replaceAt
// ============================================================================

TEST_F(ListTagIndexOpsTest, ReplaceAtRequiresMatchingElementType)
{
    nbt::tags::int_list_tag list(std::vector<std::int32_t>{1, 2, 3});

    EXPECT_TRUE(list.replaceAt(1, int_tag(42)));
    EXPECT_EQ(intValues(list), (std::vector<std::string>{"1", "42", "3"}));

    // 定长列表拒绝其它类型：写入后 element_id 会与实际内容不符，序列化即非法 NBT。
    EXPECT_FALSE(list.replaceAt(0, string_tag(std::string("nope"))));
    EXPECT_EQ(intValues(list), (std::vector<std::string>{"1", "42", "3"}));

    EXPECT_FALSE(list.replaceAt(9, int_tag(0)));
}

TEST_F(ListTagIndexOpsTest, ReplaceAtOnTagListAcceptsAnyType)
{
    tag_list_tag list; // eid 为 End：元素类型未固定
    list.appendCopy(int_tag(1));

    ASSERT_TRUE(list.replaceAt(0, string_tag(std::string("now a string"))));
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->id(), nbt::TagId::String);
}

TEST_F(ListTagIndexOpsTest, ReplaceAtOnFixedTagListRejectsOtherTypes)
{
    tag_list_tag list(nbt::TagId::Int);
    list.appendCopy(int_tag(1));

    EXPECT_FALSE(list.replaceAt(0, string_tag(std::string("nope"))));
    EXPECT_TRUE(list.replaceAt(0, int_tag(7)));
}

TEST_F(ListTagIndexOpsTest, ReplaceAtOnStringAndArrayLists)
{
    string_list_tag strings(std::vector<std::string>{"a", "b"});
    EXPECT_TRUE(strings.replaceAt(0, string_tag(std::string("z"))));
    EXPECT_EQ(strings.value[0], "z");
    EXPECT_FALSE(strings.replaceAt(0, int_tag(1)));

    nbt::tags::bytearray_list_tag arrays;
    arrays.value.push_back(std::vector<std::int8_t>{1, 2});
    arrays.value.push_back(std::vector<std::int8_t>{3});
    EXPECT_TRUE(arrays.replaceAt(1, nbt::tags::bytearray_tag(std::vector<std::int8_t>{9, 9, 9})));
    EXPECT_EQ(arrays.value[1].size(), 3u);
}

TEST_F(ListTagIndexOpsTest, ReplaceAtOnNestedListRequiresListTag)
{
    list_list_tag nested;
    nested.value.push_back(std::make_unique<nbt::tags::int_list_tag>(std::vector<std::int32_t>{1}));

    EXPECT_FALSE(nested.replaceAt(0, int_tag(5)));
    EXPECT_TRUE(nested.replaceAt(0, nbt::tags::string_list_tag(std::vector<std::string>{"x"})));
    ASSERT_EQ(nested.size(), 1u);
    ASSERT_EQ(nested[0]->id(), nbt::TagId::List);
    EXPECT_EQ(dynamic_cast<const list_tag&>(*nested[0]).element_id(), nbt::TagId::String);
}

// ============================================================================
// appendCopy
// ============================================================================

TEST_F(ListTagIndexOpsTest, AppendCopyRespectsElementType)
{
    nbt::tags::int_list_tag numbers;
    EXPECT_TRUE(numbers.appendCopy(int_tag(5)));
    EXPECT_FALSE(numbers.appendCopy(string_tag(std::string("no"))));
    EXPECT_EQ(numbers.size(), 1u);

    compound_list_tag compounds;
    compound_tag element;
    element.put("id", std::string("x"));
    EXPECT_TRUE(compounds.appendCopy(element));
    EXPECT_FALSE(compounds.appendCopy(int_tag(1)));
    ASSERT_EQ(compounds.size(), 1u);
    EXPECT_EQ(compounds.value[0].get<nbt::tags::string_tag>("id"), "x");
}

TEST_F(ListTagIndexOpsTest, AppendCopyOnEmptyEndListIsRejected)
{
    // end_list_tag 的 size 恒为 0 且元素类型是 TAG_End，任何元素都放不进去。
    end_list_tag empty;
    EXPECT_FALSE(empty.appendCopy(int_tag(1)));
    EXPECT_EQ(empty.size(), 0u);
}

TEST_F(ListTagIndexOpsTest, AppendCopyCopiesInsteadOfTakingOwnership)
{
    tag_list_tag list;
    int_tag source(3);
    ASSERT_TRUE(list.appendCopy(source));

    // 修改源标签不应影响已追加的副本。
    source.value = 99;
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(dynamic_cast<const int_tag&>(*list[0]).value, 3);
}