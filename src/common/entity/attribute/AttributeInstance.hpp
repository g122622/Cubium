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

#include "Attribute.hpp"
#include "AttributeModifier.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性实例
 *
 * 管理单个属性的基础值和所有修改器。
 * 负责计算最终属性值。
 */
class AttributeInstance {
public:
    /**
     * @brief 构造属性实例
     * @param attribute 属性定义
     */
    explicit AttributeInstance(const Attribute& attribute)
        : m_attribute(attribute)
        , m_baseValue(attribute.defaultValue())
        , m_dirty(true)
        , m_cachedValue(0.0)
    {}

    /**
     * @brief 获取属性定义
     */
    [[nodiscard]] const Attribute& attribute() const noexcept { return m_attribute; }

    /**
     * @brief 获取基础值
     */
    [[nodiscard]] f64 baseValue() const noexcept { return m_baseValue; }

    /**
     * @brief 设置基础值
     */
    void setBaseValue(f64 value)
    {
        m_baseValue = _clamp(value);
        m_dirty = true;
    }

    /**
     * @brief 获取计算后的值
     *
     * 计算流程：
     * 1. 从基础值开始
     * 2. 应用所有 Addition 操作
     * 3. 应用所有 MultiplyBase 操作
     * 4. 应用所有 MultiplyTotal 操作
     */
    [[nodiscard]] f64 getValue() const
    {
        if (m_dirty) {
            m_cachedValue = _computeValue();
            m_dirty = false;
        }
        return m_cachedValue;
    }

    /**
     * @brief 添加修改器
     * @param modifier 修改器
     */
    void addModifier(const AttributeModifier& modifier)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_modifiers.push_back(modifier);
        m_dirty = true;
    }

    /**
     * @brief 移除修改器
     * @param id 修改器ID
     * @return 是否成功移除
     */
    bool removeModifier(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(
            m_modifiers.begin(), m_modifiers.end(), [&id](const AttributeModifier& m) { return m.id() == id; });
        if (it != m_modifiers.end()) {
            m_modifiers.erase(it);
            m_dirty = true;
            return true;
        }
        return false;
    }

    /**
     * @brief 移除所有修改器
     */
    void clearModifiers()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_modifiers.clear();
        m_dirty = true;
    }

    /**
     * @brief 获取所有修改器
     */
    [[nodiscard]] const std::vector<AttributeModifier>& modifiers() const noexcept { return m_modifiers; }

    /**
     * @brief 检查是否有修改器
     * @param id 修改器ID
     */
    [[nodiscard]] bool hasModifier(const std::string& id) const
    {
        return std::any_of(
            m_modifiers.begin(), m_modifiers.end(), [&id](const AttributeModifier& m) { return m.id() == id; });
    }

    /**
     * @brief 获取修改器
     * @param id 修改器ID
     * @return 修改器指针，不存在返回nullptr
     */
    [[nodiscard]] const AttributeModifier* getModifier(const std::string& id) const
    {
        auto it = std::find_if(
            m_modifiers.begin(), m_modifiers.end(), [&id](const AttributeModifier& m) { return m.id() == id; });
        return it != m_modifiers.end() ? &(*it) : nullptr;
    }

    /**
     * @brief 是否需要同步
     */
    [[nodiscard]] bool isDirty() const noexcept { return m_dirty; }

    /**
     * @brief 标记为已同步
     */
    void markSynced() noexcept { m_dirty = false; }

private:
    /**
     * @brief 计算最终值
     */
    [[nodiscard]] f64 _computeValue() const
    {
        f64 value = m_baseValue;

        // 第一阶段：加法操作
        for (const auto& modifier : m_modifiers) {
            if (modifier.operation() == Operation::Addition) {
                value += modifier.amount();
            }
        }

        // 第二阶段：基础乘法
        for (const auto& modifier : m_modifiers) {
            if (modifier.operation() == Operation::MultiplyBase) {
                value += m_baseValue * modifier.amount();
            }
        }

        // 第三阶段：总计乘法
        for (const auto& modifier : m_modifiers) {
            if (modifier.operation() == Operation::MultiplyTotal) {
                value *= (1.0 + modifier.amount());
            }
        }

        return _clamp(value);
    }

    /**
     * @brief 将值限制在属性范围内
     */
    [[nodiscard]] f64 _clamp(f64 value) const noexcept
    {
        return std::max(m_attribute.minValue(), std::min(m_attribute.maxValue(), value));
    }

    Attribute m_attribute;
    f64 m_baseValue;
    std::vector<AttributeModifier> m_modifiers;
    mutable bool m_dirty;
    mutable f64 m_cachedValue;
    mutable std::mutex m_mutex;
};

} // namespace attribute
} // namespace entity
} // namespace mc
