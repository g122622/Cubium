#pragma once

#include "Attribute.hpp"
#include "AttributeModifier.hpp"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性实例
 *
 * 管理单个属性的基础值和所有修改器。
 * 负责计算最终属性值。
 *
 * 参考 MC 1.16.5 AttributeInstance / ModifiableAttributeInstance
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
    [[nodiscard]] const Attribute& attribute() const { return m_attribute; }

    /**
     * @brief 获取基础值
     */
    [[nodiscard]] f64 baseValue() const { return m_baseValue; }

    /**
     * @brief 设置基础值
     */
    void setBaseValue(f64 value)
    {
        m_baseValue = clamp(value);
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
            m_cachedValue = computeValue();
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
    [[nodiscard]] const std::vector<AttributeModifier>& modifiers() const { return m_modifiers; }

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
    [[nodiscard]] bool isDirty() const { return m_dirty; }

    /**
     * @brief 标记为已同步
     */
    void markSynced() { m_dirty = false; }

private:
    /**
     * @brief 计算最终值
     */
    [[nodiscard]] f64 computeValue() const
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

        return clamp(value);
    }

    /**
     * @brief 将值限制在属性范围内
     */
    [[nodiscard]] f64 clamp(f64 value) const
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
