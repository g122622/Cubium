/**
 * @file GameRule.cpp
 * @brief 游戏规则类型实现
 */

#include "GameRule.hpp"

namespace mc::world::gamerule {

// ============================================================================
// BooleanGameRuleValue 特化
// ============================================================================

template <>
[[nodiscard]] std::string GameRuleValue<bool>::toString() const {
    return m_value ? "true" : "false";
}

template <>
bool GameRuleValue<bool>::fromString(const std::string& value) {
    // MC 1.16.5 使用 Boolean.parseBoolean，支持 "true"（大小写不敏感）
    if (value == "true" || value == "TRUE" || value == "1") {
        m_value = true;
        return true;
    } else if (value == "false" || value == "FALSE" || value == "0") {
        m_value = false;
        return true;
    }
    // 其他情况默认为 false（与 Java Boolean.parseBoolean 行为一致）
    m_value = false;
    return false;
}

// ============================================================================
// IntegerGameRuleValue 特化
// ============================================================================

template <>
[[nodiscard]] std::string GameRuleValue<i32>::toString() const {
    return std::to_string(m_value);
}

template <>
bool GameRuleValue<i32>::fromString(const std::string& value) {
    if (value.empty()) {
        m_value = 0;
        return false;
    }

    try {
        m_value = std::stoi(value);
        return true;
    } catch (const std::exception&) {
        // 解析失败，保持原值不变
        return false;
    }
}

} // namespace mc::world::gamerule
