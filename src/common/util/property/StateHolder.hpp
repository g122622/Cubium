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
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace mc {

/**
 * @brief 状态持有者基类模板
 *
 * 不可变状态对象的基类，持有属性值并提供类型安全的状态转换。
 *
 * 参考: net.minecraft.state.StateHolder<O, S>
 *
 * @tparam Owner 拥有此状态的类型（如Block）
 * @tparam State 具体状态类型（如BlockState，CRTP模式）
 *
 * 注意:
 * - 状态是不可变的，with()方法返回新状态引用
 * - 所有状态在StateContainer构建时预计算
 * - 状态转换O(1)时间复杂度
 */
template <typename Owner, typename State>
class StateHolder {
public:
    struct PropertyEntry {
        const IProperty* property = nullptr;
        size_t valueIndex = 0;
    };

    struct PropertyLayout {
        const IProperty* property = nullptr;
        size_t slotIndex = 0;
        size_t stateStride = 0;
    };

    virtual ~StateHolder() = default;

    /**
     * @brief 获取状态的拥有者
     */
    [[nodiscard]] const Owner& owner() const { return *m_owner; }

    /**
     * @brief 获取属性值
     * @note 对于 bool 类型返回值而非引用，因为 std::vector<bool> 特化
     */
    template <typename T>
    [[nodiscard]] typename Property<T>::ValueReturnType get(const Property<T>& prop) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            throw std::invalid_argument(
                "Cannot get property " + prop.name() + " as it does not exist in " + ownerName());
        }
        return static_cast<const Property<T>&>(prop).valueAt(m_valueIndices[slotIndex]);
    }

    /**
     * @brief 尝试获取属性值
     * @note 对于 bool 类型返回值而非引用
     */
    template <typename T>
    [[nodiscard]] std::optional<T> getOptional(const Property<T>& prop) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            return std::nullopt;
        }
        return static_cast<const Property<T>&>(prop).valueAt(m_valueIndices[slotIndex]);
    }

    /**
     * @brief 设置属性值，返回新状态
     */
    template <typename T>
    [[nodiscard]] const State& with(const Property<T>& prop, const T& value) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            throw std::invalid_argument(
                "Cannot set property " + prop.name() + " as it does not exist in " + ownerName());
        }

        auto optIndex = prop.indexOf(value);
        if (!optIndex) {
            throw std::invalid_argument("Invalid value for property " + prop.name());
        }

        if (m_valueIndices[slotIndex] == *optIndex) {
            return static_cast<const State&>(*this);
        }

        const PropertyLayout& layout = propertyLayouts()[slotIndex];
        if (layout.property != &prop) {
            throw std::invalid_argument("Cannot set property " + prop.name() + " to " + prop.valueToString(value) +
                " on " + ownerName() + ", it is not an allowed value");
        }

        const size_t currentIndex = static_cast<size_t>(m_stateIndex);
        const size_t currentValueIndex = m_valueIndices[slotIndex];
        const size_t targetIndex = currentIndex + ((*optIndex - currentValueIndex) * layout.stateStride);
        return *(*m_allStates)[targetIndex];
    }

    /**
     * @brief 循环切换到下一个属性值
     */
    template <typename T>
    [[nodiscard]] const State& cycle(const Property<T>& prop) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            throw std::invalid_argument(
                "Cannot cycle property " + prop.name() + " as it does not exist in " + ownerName());
        }

        const auto& values = prop.allowedValues();
        size_t currentIndex = m_valueIndices[slotIndex];
        size_t nextIndex = (currentIndex + 1) % values.size();

        return with(prop, values[nextIndex]);
    }

    /**
     * @brief 检查是否有此属性
     */
    template <typename T>
    [[nodiscard]] bool hasProperty(const Property<T>& prop) const
    {
        return findPropertySlot(prop) != kInvalidIndex;
    }

    /**
     * @brief 获取所有属性值（内部索引表示）
     */
    [[nodiscard]] std::vector<PropertyEntry> values() const
    {
        std::vector<PropertyEntry> result;
        const auto& layouts = propertyLayouts();
        result.reserve(layouts.size());
        for (size_t i = 0; i < layouts.size(); ++i) {
            result.push_back(PropertyEntry{layouts[i].property, m_valueIndices[i]});
        }
        return result;
    }

    [[nodiscard]] std::optional<size_t> getValueIndex(const IProperty& prop) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            return std::nullopt;
        }
        return m_valueIndices[slotIndex];
    }

    /**
     * @brief 获取状态ID
     */
    [[nodiscard]] u32 stateId() const { return m_stateId; }

    /**
     * @brief 转换为字符串表示
     */
    [[nodiscard]] std::string toString() const
    {
        std::ostringstream ss;
        ss << ownerName();
        const auto& layouts = propertyLayouts();
        if (!layouts.empty()) {
            ss << '[';
            bool first = true;
            for (size_t i = 0; i < layouts.size(); ++i) {
                const IProperty* prop = layouts[i].property;
                const size_t valueIndex = m_valueIndices[i];
                if (!first) ss << ',';
                ss << prop->name() << '=' << prop->valueToString(valueIndex);
                first = false;
            }
            ss << ']';
        }
        return ss.str();
    }

    /**
     * @brief 比较两个状态是否相等
     */
    [[nodiscard]] bool operator==(const StateHolder& other) const { return m_stateId == other.m_stateId; }

    [[nodiscard]] bool operator!=(const StateHolder& other) const { return m_stateId != other.m_stateId; }

protected:
    StateHolder(const Owner* owner,
        std::vector<size_t> valueIndices,
        const std::vector<PropertyLayout>* propertyLayouts,
        const std::vector<State*>* allStates,
        u32 stateId)
        : m_owner(owner)
        , m_valueIndices(std::move(valueIndices))
        , m_propertyLayouts(propertyLayouts != nullptr ? propertyLayouts : &emptyPropertyLayouts())
        , m_allStates(allStates)
        , m_stateIndex(stateId)
        , m_stateId(stateId)
    {}

    /**
     * @brief 设置状态ID（由BlockRegistry调用）
     */
    void setStateId(u32 id) { m_stateId = id; }

    /**
     * @brief 获取拥有者名称（子类可重写）
     */
    [[nodiscard]] virtual std::string ownerName() const { return "Unknown"; }

    [[nodiscard]] const std::vector<PropertyLayout>& propertyLayouts() const { return *m_propertyLayouts; }

    [[nodiscard]] size_t findPropertySlot(const IProperty& prop) const
    {
        const auto& layouts = propertyLayouts();
        for (size_t i = 0; i < layouts.size(); ++i) {
            if (layouts[i].property == &prop) {
                return i;
            }
        }
        return kInvalidIndex;
    }

    [[nodiscard]] static const std::vector<PropertyLayout>& emptyPropertyLayouts()
    {
        static const std::vector<PropertyLayout> layouts;
        return layouts;
    }

    const Owner* m_owner;
    std::vector<size_t> m_valueIndices;
    const std::vector<PropertyLayout>* m_propertyLayouts = nullptr;
    const std::vector<State*>* m_allStates = nullptr;
    u32 m_stateIndex = 0;
    u32 m_stateId;
    static constexpr size_t kInvalidIndex = static_cast<size_t>(-1);

    // 允许StateContainer和BlockRegistry访问
    template <typename O, typename S>
    friend class StateContainer;
    friend class BlockRegistry;
};

} // namespace mc
