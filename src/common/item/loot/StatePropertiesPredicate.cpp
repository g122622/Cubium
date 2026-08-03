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

#include "StatePropertiesPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ============================================================================
// 静态成员
// ============================================================================

StatePropertiesPredicate StatePropertiesPredicate::EMPTY;

// ============================================================================
// Matcher 基类
// ============================================================================

StatePropertiesPredicate::Matcher::Matcher(std::string propertyName)
    : m_propertyName(std::move(propertyName))
{}

bool StatePropertiesPredicate::Matcher::match(const BlockState& state) const
{
    // 获取方块的 StateContainer
    const Block& block = state.getBlock();
    const auto& container = block.stateContainer();

    // 查找属性
    const IProperty* property = container.getProperty(m_propertyName);
    if (property == nullptr) {
        // 属性不存在于此方块
        return false;
    }

    // 检查方块状态是否有此属性
    const auto valueIndex = state.getValueIndex(*property);
    if (!valueIndex.has_value()) {
        // 状态中没有此属性
        return false;
    }

    // 调用子类的精确匹配逻辑
    return matchExact(state, property);
}

// ============================================================================
// ExactMatcher
// ============================================================================

StatePropertiesPredicate::ExactMatcher::ExactMatcher(const std::string& propertyName, std::string value)
    : Matcher(propertyName)
    , m_value(std::move(value))
{}

std::string StatePropertiesPredicate::ExactMatcher::toJson() const
{
    return "\"" + m_value + "\"";
}

std::unique_ptr<StatePropertiesPredicate::Matcher> StatePropertiesPredicate::ExactMatcher::clone() const
{
    return std::make_unique<ExactMatcher>(m_propertyName, m_value);
}

bool StatePropertiesPredicate::ExactMatcher::matchExact(const BlockState& state, const IProperty* property) const
{
    MC_UNUSED(state);

    // 尝试解析字符串值
    auto optIndex = property->parseValue(m_value);
    if (!optIndex.has_value()) {
        // 无法解析值
        return false;
    }

    // 获取当前属性值索引
    const auto valueIndex = state.getValueIndex(*property);
    if (!valueIndex.has_value()) {
        return false;
    }

    // 比较索引值
    return *valueIndex == *optIndex;
}

// ============================================================================
// RangedMatcher
// ============================================================================

StatePropertiesPredicate::RangedMatcher::RangedMatcher(
    const std::string& propertyName, std::optional<std::string> min, std::optional<std::string> max)
    : Matcher(propertyName)
    , m_min(std::move(min))
    , m_max(std::move(max))
{}

std::string StatePropertiesPredicate::RangedMatcher::toJson() const
{
    std::ostringstream ss;
    ss << "{";
    if (m_min.has_value()) {
        ss << "\"min\":\"" << *m_min << "\"";
        if (m_max.has_value()) {
            ss << ",";
        }
    }
    if (m_max.has_value()) {
        ss << "\"max\":\"" << *m_max << "\"";
    }
    ss << "}";
    return ss.str();
}

std::unique_ptr<StatePropertiesPredicate::Matcher> StatePropertiesPredicate::RangedMatcher::clone() const
{
    return std::make_unique<RangedMatcher>(m_propertyName, m_min, m_max);
}

bool StatePropertiesPredicate::RangedMatcher::matchExact(const BlockState& state, const IProperty* property) const
{
    // 获取当前属性值索引
    const auto valueIndex = state.getValueIndex(*property);
    if (!valueIndex.has_value()) {
        return false;
    }

    size_t currentIndex = *valueIndex;
    std::string currentValueStr = property->valueToString(currentIndex);

    // 检查最小值
    if (m_min.has_value()) {
        // 需要比较字符串值
        // 对于整数属性，值字符串是数字，可以按字典序比较
        // 对于枚举属性，按值索引比较
        auto minIndex = property->parseValue(*m_min);
        if (!minIndex.has_value()) {
            return false;
        }
        // 使用索引比较（要求属性值有序）
        // MC 使用 Comparable.compareTo，这里我们用索引来模拟
        if (currentIndex < *minIndex) {
            return false;
        }
    }

    // 检查最大值
    if (m_max.has_value()) {
        auto maxIndex = property->parseValue(*m_max);
        if (!maxIndex.has_value()) {
            return false;
        }
        if (currentIndex > *maxIndex) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// StatePropertiesPredicate
// ============================================================================

StatePropertiesPredicate::StatePropertiesPredicate(std::vector<std::unique_ptr<Matcher>> matchers)
    : m_matchers(std::move(matchers))
{}

StatePropertiesPredicate::StatePropertiesPredicate(const StatePropertiesPredicate& other)
{
    m_matchers.reserve(other.m_matchers.size());
    for (const auto& matcher : other.m_matchers) {
        m_matchers.push_back(matcher->clone());
    }
}

StatePropertiesPredicate& StatePropertiesPredicate::operator=(const StatePropertiesPredicate& other)
{
    if (this != &other) {
        m_matchers.clear();
        m_matchers.reserve(other.m_matchers.size());
        for (const auto& matcher : other.m_matchers) {
            m_matchers.push_back(matcher->clone());
        }
    }
    return *this;
}

bool StatePropertiesPredicate::matches(const BlockState& state) const
{
    for (const auto& matcher : m_matchers) {
        if (!matcher->match(state)) {
            return false;
        }
    }
    return true;
}

void StatePropertiesPredicate::addExactMatch(const std::string& propertyName, const std::string& value)
{
    m_matchers.push_back(std::make_unique<ExactMatcher>(propertyName, value));
}

void StatePropertiesPredicate::addRangeMatch(
    const std::string& propertyName, std::optional<std::string> min, std::optional<std::string> max)
{
    // 如果 min == max，优化为精确匹配
    if (min.has_value() && max.has_value() && *min == *max) {
        addExactMatch(propertyName, *min);
    } else {
        m_matchers.push_back(std::make_unique<RangedMatcher>(propertyName, std::move(min), std::move(max)));
    }
}

void StatePropertiesPredicate::addMatcher(std::unique_ptr<Matcher> matcher)
{
    m_matchers.push_back(std::move(matcher));
}

std::string StatePropertiesPredicate::toJson() const
{
    if (m_matchers.empty()) {
        return "{}";
    }

    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& matcher : m_matchers) {
        if (!first) {
            ss << ",";
        }
        ss << "\"" << matcher->propertyName() << "\":" << matcher->toJson();
        first = false;
    }
    ss << "}";
    return ss.str();
}

nlohmann::json StatePropertiesPredicate::toJsonValue() const
{
    if (m_matchers.empty()) {
        return nlohmann::json::object();
    }

    nlohmann::json json = nlohmann::json::object();
    for (const auto& matcher : m_matchers) {
        const std::string& propName = matcher->propertyName();

        // 使用 dynamic_cast 来判断匹配器类型
        if (const auto* exactMatcher = dynamic_cast<const ExactMatcher*>(matcher.get())) {
            json[propName] = exactMatcher->value();
        } else if (const auto* rangedMatcher = dynamic_cast<const RangedMatcher*>(matcher.get())) {
            nlohmann::json rangeJson = nlohmann::json::object();
            if (rangedMatcher->minValue().has_value()) {
                rangeJson["min"] = *rangedMatcher->minValue();
            }
            if (rangedMatcher->maxValue().has_value()) {
                rangeJson["max"] = *rangedMatcher->maxValue();
            }
            json[propName] = std::move(rangeJson);
        }
    }

    return json;
}

Result<StatePropertiesPredicate> StatePropertiesPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null() || !json.is_object()) {
        return StatePropertiesPredicate{};
    }

    StatePropertiesPredicate predicate;
    for (auto it = json.begin(); it != json.end(); ++it) {
        const std::string& propName = it.key();
        const nlohmann::json& value = it.value();

        // 解析单个属性匹配器
        if (value.is_string()) {
            // 精确匹配: "age": "3"
            predicate.addExactMatch(propName, value.get<std::string>());
        } else if (value.is_object()) {
            // 范围匹配: "power": { "min": "5", "max": "10" }
            std::optional<std::string> min;
            std::optional<std::string> max;

            if (value.contains("min") && !value["min"].is_null()) {
                min = value["min"].get<std::string>();
            }
            if (value.contains("max") && !value["max"].is_null()) {
                max = value["max"].get<std::string>();
            }

            // 如果 min == max，优化为精确匹配
            if (min.has_value() && max.has_value() && *min == *max) {
                predicate.addExactMatch(propName, *min);
            } else if (min.has_value() || max.has_value()) {
                predicate.addRangeMatch(propName, std::move(min), std::move(max));
            }
            // 如果 min 和 max 都没有，跳过这个属性
        }
        // 其他类型忽略
    }

    return predicate;
}

} // namespace mc
