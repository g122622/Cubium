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

#include "SetNbtFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/core/Types.hpp"
#include <sstream>

namespace mc {
namespace loot {

// ============================================================================
// NBT 转 JSON 辅助函数
// ============================================================================

namespace {

/**
 * @brief 将 NBT 标签转换为 nlohmann::json
 *
 * 参考 MC 1.16.5 的 NBT 到 JSON 转换规则：
 * - 数值类型直接转换
 * - 字符串直接转换
 * - 数组转换为 JSON 数组
 * - 列表转换为 JSON 数组
 * - 复合标签转换为 JSON 对象
 *
 * @param tag NBT 标签
 * @return 转换后的 JSON 值
 */
nlohmann::json nbtToJson(const nbt::tags::tag& tag);

nlohmann::json nbtListToJson(const nbt::tags::list_tag& list)
{
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        auto elem = list[i];
        if (elem) {
            result.push_back(nbtToJson(*elem));
        }
    }
    return result;
}

nlohmann::json nbtToJson(const nbt::tags::tag& tag)
{
    using namespace nbt::tags;

    switch (tag.id()) {
        case nbt::TagId::Byte: {
            const auto& t = dynamic_cast<const byte_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Short: {
            const auto& t = dynamic_cast<const short_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Int: {
            const auto& t = dynamic_cast<const int_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Long: {
            const auto& t = dynamic_cast<const long_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Float: {
            const auto& t = dynamic_cast<const float_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Double: {
            const auto& t = dynamic_cast<const double_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::ByteArray: {
            const auto& t = dynamic_cast<const bytearray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::String: {
            const auto& t = dynamic_cast<const string_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::List: {
            const auto& t = dynamic_cast<const list_tag&>(tag);
            return nbtListToJson(t);
        }
        case nbt::TagId::Compound: {
            const auto& t = dynamic_cast<const compound_tag&>(tag);
            nlohmann::json result = nlohmann::json::object();
            for (const auto& [key, value] : t.value) {
                if (value) {
                    result[key] = nbtToJson(*value);
                }
            }
            return result;
        }
        case nbt::TagId::IntArray: {
            const auto& t = dynamic_cast<const intarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::LongArray: {
            const auto& t = dynamic_cast<const longarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::End:
        default:
            return nullptr;
    }
}

} // anonymous namespace

SetNbtFunction::SetNbtFunction(const std::string& nbtString)
    : m_nbtString(nbtString)
{}

ItemStack SetNbtFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);

    if (stack.isEmpty() || m_nbtString.empty()) {
        return stack;
    }

    // 参考 MC 1.16.5 net.minecraft.loot.functions.SetNBT.doApply
    // 1. 解析 NBT 字符串（Mojangson 格式）
    // 2. 将解析的 NBT 合并到 ItemStack 的现有标签中

    try {
        // 使用 Mojangson 格式解析 NBT 字符串
        std::istringstream iss(m_nbtString);

        // 设置 Mojangson 上下文
        iss >> nbt::contexts::mojangson;

        // 使用 compound_tag::read 静态方法解析
        auto parsedTagPtr = nbt::tags::compound_tag::read(iss);
        if (!parsedTagPtr || iss.fail()) {
            // 解析失败，返回原始物品
            return stack;
        }

        nbt::tags::compound_tag& parsedTag = *parsedTagPtr;

        // 将 NBT 转换为 JSON 并合并到 ItemStack
        nlohmann::json jsonTag = nbtToJson(parsedTag);
        if (jsonTag.is_object() && !jsonTag.empty()) {
            stack.mergeTag(jsonTag);
        }
    }
    catch (const std::exception& e) {
        // 解析异常，返回原始物品
        // 在实际游戏中可能需要记录日志
        MC_UNUSED(e);
        return stack;
    }

    return stack;
}

std::unique_ptr<LootFunction> SetNbtFunction::clone() const
{
    auto func = std::make_unique<SetNbtFunction>(m_nbtString);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
