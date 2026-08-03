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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "NBTPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

NBTPredicate::NBTPredicate(std::unique_ptr<nbt::tags::compound_tag> tag)
    : m_tag(std::move(tag))
{}

NBTPredicate::NBTPredicate(const NBTPredicate& other)
{
    if (other.m_tag) {
        m_tag = std::unique_ptr<nbt::tags::compound_tag>(
            dynamic_cast<nbt::tags::compound_tag*>(other.m_tag->copy().release()));
    }
}

NBTPredicate& NBTPredicate::operator=(const NBTPredicate& other)
{
    if (this != &other) {
        if (other.m_tag) {
            m_tag = std::unique_ptr<nbt::tags::compound_tag>(
                dynamic_cast<nbt::tags::compound_tag*>(other.m_tag->copy().release()));
        } else {
            m_tag = nullptr;
        }
    }
    return *this;
}

bool NBTPredicate::test(const Entity& entity) const
{
    if (isAny()) {
        return true;
    }

    // 将实体序列化为NBT后进行子集匹配
    nbt::tags::compound_tag entityNbt;
    entity.writeToNBT(entityNbt);
    return test(&entityNbt);
}

bool NBTPredicate::test(const ItemStack& stack) const
{
    if (isAny()) {
        return true;
    }

    if (stack.isEmpty()) {
        return false;
    }

    // 将物品的tag字段序列化为NBT后进行子集匹配
    auto itemTag = _serializeItemStackTag(stack);
    if (itemTag == nullptr) {
        return false;
    }
    return test(itemTag.get());
}

bool NBTPredicate::test(const nbt::tags::compound_tag* tag) const
{
    if (isAny()) {
        return true;
    }

    if (tag == nullptr) {
        return false;
    }

    return matchNBT(*m_tag, *tag);
}

Result<NBTPredicate> NBTPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return NBTPredicate{};
    }

    // 字符串格式：Mojangson 格式的NBT字符串
    if (json.is_string()) {
        std::string nbtString = json.get<std::string>();
        auto parsedTag = nbt::parseMojangson(nbtString);
        if (!parsedTag) {
            return Error(
                ErrorCode::InvalidArgument, fmt::format("Failed to parse Mojangson NBT string: {}", nbtString));
        }
        return NBTPredicate(std::move(parsedTag));
    }

    // 对象格式：JSON对象直接转换为NBT compound tag
    if (json.is_object()) {
        auto nbtTag = nbt::jsonToNbt(json);
        if (!nbtTag) {
            return Error(ErrorCode::InvalidArgument, "Failed to convert JSON object to NBT");
        }
        return NBTPredicate(std::move(nbtTag));
    }

    return NBTPredicate{};
}

nlohmann::json NBTPredicate::toJson() const
{
    if (isAny()) {
        return nullptr;
    }

    // 将NBT序列化为Mojangson字符串
    std::string mojangson = std::to_string(*m_tag);
    return mojangson;
}

bool NBTPredicate::matchNBT(const nbt::tags::compound_tag& expected, const nbt::tags::compound_tag& actual) noexcept
{
    // 遍历期望的所有字段，检查在实际NBT中是否存在且值相等
    for (const auto& [key, value] : expected.value) {
        auto it = actual.value.find(key);
        if (it == actual.value.end()) {
            // 期望的字段在实际NBT中不存在
            return false;
        }

        // 递归比较标签值
        if (!matchTag(*value, *it->second)) {
            return false;
        }
    }

    return true;
}

bool NBTPredicate::matchTag(const nbt::tags::tag& expected, const nbt::tags::tag& actual) noexcept
{
    // 类型不同，不匹配
    if (expected.id() != actual.id()) {
        return false;
    }

    // 根据标签类型比较值
    switch (expected.id()) {
        case nbt::TagId::End:
            return true;

        case nbt::TagId::Byte: {
            const auto& exp = static_cast<const nbt::tags::byte_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::byte_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::Short: {
            const auto& exp = static_cast<const nbt::tags::short_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::short_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::Int: {
            const auto& exp = static_cast<const nbt::tags::int_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::int_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::Long: {
            const auto& exp = static_cast<const nbt::tags::long_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::long_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::Float: {
            const auto& exp = static_cast<const nbt::tags::float_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::float_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::Double: {
            const auto& exp = static_cast<const nbt::tags::double_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::double_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::ByteArray: {
            const auto& exp = static_cast<const nbt::tags::bytearray_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::bytearray_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::String: {
            const auto& exp = static_cast<const nbt::tags::string_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::string_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::List: {
            const auto& exp = static_cast<const nbt::tags::list_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::list_tag&>(actual);

            // 列表元素类型必须相等
            if (exp.element_id() != act.element_id()) {
                return false;
            }

            // 空期望列表匹配任何列表
            if (exp.size() == 0) {
                return true;
            }

            // 实际列表长度不能小于期望列表
            if (act.size() < exp.size()) {
                return false;
            }

            // 无序子集匹配：期望列表中的每个元素必须在实际列表中存在至少一个匹配的元素
            // 这与 MC Java 的 NbtUtils.compareNbt(listCompare=true) 行为一致
            for (size_t i = 0; i < exp.size(); ++i) {
                auto expElem = exp[i];
                bool found = false;
                for (size_t j = 0; j < act.size(); ++j) {
                    auto actElem = act[j];
                    if (matchTag(*expElem, *actElem)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        }

        case nbt::TagId::Compound: {
            const auto& exp = static_cast<const nbt::tags::compound_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::compound_tag&>(actual);
            return matchNBT(exp, act);
        }

        case nbt::TagId::IntArray: {
            const auto& exp = static_cast<const nbt::tags::intarray_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::intarray_tag&>(actual);
            return exp.value == act.value;
        }

        case nbt::TagId::LongArray: {
            const auto& exp = static_cast<const nbt::tags::longarray_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::longarray_tag&>(actual);
            return exp.value == act.value;
        }

        default:
            return false;
    }
}

std::unique_ptr<nbt::tags::compound_tag> NBTPredicate::_serializeItemStackTag(const ItemStack& stack)
{
    // 将物品堆完整序列化为NBT，然后提取tag字段
    nbt::tags::compound_tag rootTag;
    stack.toNbt(rootTag);

    // 检查是否存在tag子标签
    auto it = rootTag.value.find("tag");
    if (it == rootTag.value.end()) {
        return nullptr;
    }

    if (it->second->id() != nbt::TagId::Compound) {
        return nullptr;
    }

    // 复制tag字段的内容
    const auto& tagCompound = static_cast<const nbt::tags::compound_tag&>(*it->second);
    auto result =
        std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(tagCompound.copy().release()));
    return result;
}

} // namespace mc::advancement
