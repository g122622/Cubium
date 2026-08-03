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

#pragma once

#include "Property.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mc {

/**
 * @brief 布尔属性
 *
 * 表示一个布尔类型的方块状态属性，如 lit, powered, open 等。
 *
 * 参考: net.minecraft.state.BooleanProperty
 *
 * 用法示例:
 * @code
 * // 创建属性
 * auto lit = BooleanProperty::create("lit");
 *
 * // 获取值
 * bool isLit = state.get(*lit);
 *
 * // 设置值
 * const BlockState& newState = state.with(*lit, true);
 * @endcode
 */
class BooleanProperty : public Property<bool> {
public:
    /**
     * @brief 创建布尔属性
     * @param name 属性名称
     * @return 属性实例
     *
     * 注意: 属性名称应该遵循MC命名约定，使用小写字母和下划线
     */
    [[nodiscard]] static std::unique_ptr<BooleanProperty> create(const std::string& name)
    {
        return std::unique_ptr<BooleanProperty>(new BooleanProperty(name));
    }

    /**
     * @brief 获取允许的值（true和false）
     */
    [[nodiscard]] const std::vector<bool>& allowedValues() const { return m_values; }

    /**
     * @brief 将布尔值转换为字符串
     */
    [[nodiscard]] std::string valueToString(const bool& value) const override { return value ? "true" : "false"; }

    /**
     * @brief 解析字符串为布尔值
     */
    [[nodiscard]] std::optional<bool> parse(std::string_view str) const override
    {
        if (str == "true") return true;
        if (str == "false") return false;
        return std::nullopt;
    }

    /**
     * @brief 计算哈希值
     */
    [[nodiscard]] size_t hashCode() const override
    {
        return std::hash<std::string>{}(m_name) ^ (std::hash<std::string>{}("BooleanProperty") << 1);
    }

    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] const char* typeName() const override { return "BooleanProperty"; }

private:
    explicit BooleanProperty(const std::string& name)
        : Property<bool>(name, {false, true})
    {}
};

} // namespace mc
