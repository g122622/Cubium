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
 * The copyright notice and this permission notice shall be included in all
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

#include "common/util/nbt/Nbt.hpp"
#include <vector>
#include <gtest/gtest.h>

namespace mc {
namespace nbt {
namespace test {

/**
 * @brief NBT 复合标签赋值语义测试
 *
 * 覆盖 compound_tag 的拷贝赋值：目标已有的键必须被源标签的值覆盖（而不是保留旧值），
 * 自赋值与嵌套复合标签的深拷贝同样需要保持正确。
 */
class NbtTagAssignmentTest : public ::testing::Test {
protected:
    static std::string _stringValue(const tags::compound_tag& compound, const std::string& key)
    {
        const auto it = compound.value.find(key);
        if (it == compound.value.end() || it->second == nullptr || it->second->id() != TagId::String) {
            return {};
        }
        return static_cast<const tags::string_tag&>(*it->second).value;
    }
};

TEST_F(NbtTagAssignmentTest, AssignOverwritesExistingKeys)
{
    tags::compound_tag target;
    target.put("id", std::string("diamond"));
    target.put("keep", std::string("old"));

    tags::compound_tag source;
    source.put("id", std::string("iron"));

    target = source;

    EXPECT_EQ(_stringValue(target, "id"), "iron");
    // 源标签没有 keep 键，赋值后 target 应与 source 完全一致
    EXPECT_FALSE(target.value.contains("keep"));
    EXPECT_EQ(target.value.size(), 1u);
}

TEST_F(NbtTagAssignmentTest, AssignDeepCopiesNestedCompounds)
{
    tags::compound_tag nested;
    nested.put("color", std::string("red"));
    tags::compound_tag source;
    source.value["nested"] = std::make_unique<tags::compound_tag>(nested);

    tags::compound_tag target;
    target = source;

    auto& targetNested = static_cast<tags::compound_tag&>(*target.value.at("nested"));
    auto& sourceNested = static_cast<tags::compound_tag&>(*source.value.at("nested"));
    EXPECT_NE(&targetNested, &sourceNested);
    EXPECT_EQ(_stringValue(targetNested, "color"), "red");

    // 修改目标不应影响源（深拷贝）
    targetNested.value["color"] = std::make_unique<tags::string_tag>(std::string("blue"));
    EXPECT_EQ(_stringValue(sourceNested, "color"), "red");
}

TEST_F(NbtTagAssignmentTest, SelfAssignKeepsContent)
{
    tags::compound_tag tag;
    tag.put("id", std::string("stone"));

    const auto* address = &tag;
    tag = static_cast<const tags::compound_tag&>(tag);

    EXPECT_EQ(address, &tag);
    EXPECT_EQ(_stringValue(tag, "id"), "stone");
}

TEST_F(NbtTagAssignmentTest, VectorShiftAfterEraseKeepsValues)
{
    // 复合列表元素原地删除依赖 std::vector 的移动赋值把后一个元素搬移到前一个位置，
    // 若复合标签赋值没有覆盖已有键，搬移后的元素会保留被删除元素的内容
    tags::compound_list_tag list;
    tags::compound_tag first;
    first.put("id", std::string("diamond"));
    list.value.push_back(std::move(first));
    tags::compound_tag second;
    second.put("id", std::string("iron"));
    list.value.push_back(std::move(second));

    list.value.erase(list.value.begin());

    ASSERT_EQ(list.value.size(), 1u);
    EXPECT_EQ(_stringValue(list.value[0], "id"), "iron");
}

} // namespace test
} // namespace nbt
} // namespace mc