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

#include "NBTPredicate.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <sstream>

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

    // 将实体序列化为NBT后进行比较
    // 由于当前项目实体NBT序列化尚未完全实现，暂时返回 true
    // TODO: 实现实体NBT序列化后启用完整匹配
    // nbt::tags::compound_tag entityNbt;
    // entity.serializeNBT(entityNbt);
    // return test(&entityNbt);

    MC_UNUSED(entity);
    return true;
}

bool NBTPredicate::test(const ItemStack& stack) const
{
    if (isAny()) {
        return true;
    }

    // 检查物品的NBT标签
    // 由于当前项目物品NBT序列化尚未完全实现，暂时返回 true
    // TODO: 实现物品NBT序列化后启用完整匹配
    // const nbt::tags::compound_tag* itemNbt = stack.getTag();
    // return test(itemNbt);

    MC_UNUSED(stack);
    return true;
}

bool NBTPredicate::test(const nbt::tags::compound_tag* tag) const
{
    if (isAny()) {
        return true;
    }

    if (tag == nullptr) {
        return false;
    }

    return _matchNBT(*m_tag, *tag);
}

Result<NBTPredicate> NBTPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return NBTPredicate{};
    }

    // 解析 JSON 格式的 NBT 数据
    // 使用字符串格式的 NBT（Mojangson 格式）
    if (json.is_string()) {
        std::string nbtString = json.get<std::string>();
        // TODO: 实现 Mojangson 解析器
        // 暂时返回空的 NBTPredicate
        MC_UNUSED(nbtString);
        return NBTPredicate{};
    }

    // 解析对象格式的 NBT 数据
    if (json.is_object()) {
        // TODO: 实现 JSON 到 NBT 的转换
        // 暂时返回空的 NBTPredicate
        return NBTPredicate{};
    }

    return NBTPredicate{};
}

nlohmann::json NBTPredicate::toJson() const
{
    if (isAny()) {
        return nullptr;
    }

    // TODO: 实现 NBT 到 JSON 的转换
    // 暂时返回 null
    return nullptr;
}

bool NBTPredicate::_matchNBT(const nbt::tags::compound_tag& expected, const nbt::tags::compound_tag& actual) noexcept
{
    // 遍历期望的所有字段，检查在实际NBT中是否存在且值相等
    for (const auto& [key, value] : expected.value) {
        auto it = actual.value.find(key);
        if (it == actual.value.end()) {
            // 期望的字段在实际NBT中不存在
            return false;
        }

        // 递归比较标签值
        if (!_matchTag(*value, *it->second)) {
            return false;
        }
    }

    return true;
}

bool NBTPredicate::_matchTag(const nbt::tags::tag& expected, const nbt::tags::tag& actual) noexcept
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

            // 列表长度必须相等
            if (exp.size() != act.size()) {
                return false;
            }

            // 列表元素类型必须相等
            if (exp.element_id() != act.element_id()) {
                return false;
            }

            // 逐个比较元素
            for (size_t i = 0; i < exp.size(); ++i) {
                auto expElem = exp[i];
                auto actElem = act[i];
                if (!_matchTag(*expElem, *actElem)) {
                    return false;
                }
            }
            return true;
        }

        case nbt::TagId::Compound: {
            const auto& exp = static_cast<const nbt::tags::compound_tag&>(expected);
            const auto& act = static_cast<const nbt::tags::compound_tag&>(actual);
            return _matchNBT(exp, act);
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

} // namespace mc::advancement
