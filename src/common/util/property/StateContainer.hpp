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

#include "BooleanProperty.hpp"
#include "DirectionProperty.hpp"
#include "IProperty.hpp"
#include "IntegerProperty.hpp"
#include "Property.hpp"
#include "StateHolder.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fmt/format.h>

namespace mc {

/**
 * @brief 状态容器模板
 *
 * 预计算并管理所有可能的状态组合。
 *
 *
 * @tparam Owner 拥有者类型
 * @tparam State 状态类型
 *
 * 注意:
 * - 状态数量 = 各属性值数量的乘积，可能非常大
 * - 属性名称必须符合 [a-z0-9_]+ 格式
 * - 每个属性至少需要2个值
 */
template <typename Owner, typename State>
class StateContainer {
public:
    using StateFactory = std::function<std::unique_ptr<State>(const Owner&,
        StateValueIndices,
        size_t,
        const std::vector<typename StateHolder<Owner, State>::PropertyLayout>*,
        const std::vector<State*>*,
        u32)>;

    /**
     * @brief 构建器
     */
    class Builder {
    public:
        explicit Builder(Owner& owner)
            : m_owner(owner)
        {}

        /**
         * @brief 添加属性（拥有所有权）
         */
        Builder& add(std::unique_ptr<IProperty> prop)
        {
            MC_ASSERT_RELEASE_MSG(prop != nullptr, "StateContainer::Builder::add: property cannot be null");
            validateProperty(*prop);
            const std::string name = prop->name();
            const IProperty* rawProp = prop.get();
            m_ownedProperties.push_back(std::move(prop));
            m_properties[name] = rawProp;
            m_declarationOrder.push_back(rawProp);
            return *this;
        }

        /**
         * @brief 添加属性（借用引用，不转移所有权）
         */
        Builder& add(const IProperty* prop)
        {
            MC_ASSERT_RELEASE_MSG(prop != nullptr, "StateContainer::Builder::add: property cannot be null");
            validateProperty(*prop);
            m_properties[prop->name()] = prop;
            m_declarationOrder.push_back(prop);
            return *this;
        }

        /**
         * @brief 添加属性（引用版本）
         */
        template <typename T>
        Builder& add(const Property<T>& prop)
        {
            return add(static_cast<const IProperty*>(&prop));
        }

        /**
         * @brief 添加布尔属性
         */
        Builder& addBoolean(const std::string& name)
        {
            return add(BooleanProperty::create(name));
        }

        /**
         * @brief 添加整数属性
         */
        Builder& addInteger(const std::string& name, i32 min, i32 max)
        {
            return add(IntegerProperty::create(name, min, max));
        }

        /**
         * @brief 添加方向属性（所有方向）
         */
        Builder& addDirection(const std::string& name)
        {
            return add(DirectionProperty::create(name));
        }

        /**
         * @brief 添加方向属性（仅水平方向）
         */
        Builder& addHorizontalDirection(const std::string& name)
        {
            return add(DirectionProperty::createHorizontal(name));
        }

        /**
         * @brief 添加坐标轴属性
         */
        Builder& addAxis(const std::string& name)
        {
            return add(AxisProperty::create(name));
        }

        /**
         * @brief 构建状态容器
         */
        std::unique_ptr<StateContainer> create(StateFactory factory)
        {
            return std::unique_ptr<StateContainer>(new StateContainer(m_owner,
                std::move(m_properties),
                std::move(m_declarationOrder),
                std::move(m_ownedProperties),
                factory));
        }

    private:
        Owner& m_owner;
        std::unordered_map<std::string, const IProperty*> m_properties;
        /// 属性声明顺序：决定状态表的 stride，必须稳定。
        std::vector<const IProperty*> m_declarationOrder;
        std::vector<std::unique_ptr<IProperty>> m_ownedProperties;

        void validateProperty(const IProperty& prop)
        {
            // 属性名/值名格式与重复注册都属于注册期的编程错误，一旦出错必须当场崩溃，
            // 否则会生成错误的属性布局，后续 with() 会把方块切到不相干的状态。
            static const std::regex NAME_PATTERN("^[a-z0-9_]+$");
            MC_ASSERT_RELEASE_MSG(std::regex_match(prop.name(), NAME_PATTERN),
                fmt::format("StateContainer::Builder: invalid property name '{}'", prop.name()).c_str());
            MC_ASSERT_RELEASE_MSG(prop.valueCount() > 1,
                fmt::format("StateContainer::Builder: property '{}' must have more than 1 possible value", prop.name())
                    .c_str());
            for (size_t i = 0; i < prop.valueCount(); ++i) {
                MC_ASSERT_RELEASE_MSG(std::regex_match(prop.valueToString(i), NAME_PATTERN),
                    fmt::format("StateContainer::Builder: property '{}' has invalid value name '{}'",
                        prop.name(),
                        prop.valueToString(i))
                        .c_str());
            }
            MC_ASSERT_RELEASE_MSG(m_properties.find(prop.name()) == m_properties.end(),
                fmt::format("StateContainer::Builder: duplicate property '{}'", prop.name()).c_str());
        }
    };

    [[nodiscard]] const State& baseState() const
    {
        return *m_states[0];
    }

    [[nodiscard]] const std::vector<std::unique_ptr<State>>& validStates() const
    {
        return m_states;
    }

    [[nodiscard]] size_t stateCount() const
    {
        return m_states.size();
    }

    [[nodiscard]] const Owner& owner() const
    {
        return m_owner;
    }

    [[nodiscard]] const std::unordered_map<std::string, const IProperty*>& properties() const
    {
        return m_properties;
    }

    [[nodiscard]] const IProperty* getProperty(std::string_view name) const
    {
        auto it = m_properties.find(std::string(name));
        return it != m_properties.end() ? it->second : nullptr;
    }

    [[nodiscard]] std::string toString() const
    {
        std::ostringstream ss;
        ss << "StateContainer{owner=" << typeid(Owner).name();
        if (!m_properties.empty()) {
            ss << ", properties=[";
            bool first = true;
            for (const auto& [name, prop] : m_properties) {
                if (!first) {
                    ss << ", ";
                }
                ss << name;
                first = false;
            }
            ss << "]";
        }
        ss << ", states=" << m_states.size() << "}";
        return ss.str();
    }

private:
    using PropertyLayout = typename StateHolder<Owner, State>::PropertyLayout;

    StateContainer(Owner& owner,
        std::unordered_map<std::string, const IProperty*> propertiesToTransfer,
        std::vector<const IProperty*> declarationOrder,
        std::vector<std::unique_ptr<IProperty>> ownedProperties,
        StateFactory factory)
        : m_owner(owner)
        , m_properties(std::move(propertiesToTransfer))
        , m_ownedProperties(std::move(ownedProperties))
    {
        // 属性必须按**声明顺序**展开：状态值索引的 stride 由这个顺序决定，而 `with()`
        // 正是用 `(目标值索引 - 当前值索引) * stride` 计算目标状态的。顺序一旦不可确定，
        // `with()` 就会落到另一个属性组合上——例如给 oak_slab 设 `type=bottom` 反而把
        // `waterlogged` 翻成 true。`std::unordered_map` 的遍历顺序没有保证，故不能直接
        // 用它。
        generateStates(declarationOrder, factory);
    }

    void generateStates(const std::vector<const IProperty*>& props, StateFactory factory)
    {
        // 内联值索引数组的容量上限。属性被静默丢掉会让 with() 算出错误的 targetIndex、
        // 把方块切换到不相干的状态，且不产生任何报错，故在此当场拦下。
        MC_ASSERT_RELEASE_MSG(props.size() <= MAX_STATE_PROPERTIES,
            "StateContainer::generateStates: property count exceeds MAX_STATE_PROPERTIES; "
            "raise the constant (and re-check hot-path assumptions) rather than dropping properties");

        size_t totalStates = 1;
        for (const auto* prop : props) {
            totalStates *= prop->valueCount();
        }

        m_propertyLayouts.clear();
        m_propertyLayouts.reserve(props.size());

        size_t stride = 1;
        for (const auto* prop : props) {
            MC_ASSERT_RELEASE_MSG(
                prop->valueCount() <= static_cast<size_t>(std::numeric_limits<StateValueIndex>::max()),
                "StateContainer::generateStates: property has more values than StateValueIndex can hold");
            m_propertyLayouts.push_back(PropertyLayout{prop, m_propertyLayouts.size(), stride});
            stride *= prop->valueCount();
        }

        m_states.reserve(totalStates);
        m_statePointers.clear();
        m_statePointers.reserve(totalStates);

        u32 stateId = 0;
        for (size_t flatIndex = 0; flatIndex < totalStates; ++flatIndex) {
            StateValueIndices valueIndices{};
            size_t remaining = flatIndex;
            for (size_t propIndex = 0; propIndex < props.size(); ++propIndex) {
                const size_t valueCount = props[propIndex]->valueCount();
                valueIndices[propIndex] = static_cast<StateValueIndex>(remaining % valueCount);
                remaining /= valueCount;
            }

            auto state = factory(m_owner, valueIndices, props.size(), &m_propertyLayouts, &m_statePointers, stateId);
            m_statePointers.push_back(state.get());
            m_states.push_back(std::move(state));
            stateId++;
        }
    }

    Owner& m_owner;
    std::unordered_map<std::string, const IProperty*> m_properties;
    std::vector<std::unique_ptr<IProperty>> m_ownedProperties;
    std::vector<std::unique_ptr<State>> m_states;
    std::vector<State*> m_statePointers;
    std::vector<PropertyLayout> m_propertyLayouts;
};

} // namespace mc
