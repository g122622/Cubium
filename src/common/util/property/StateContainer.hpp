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
#include <algorithm>
#include <functional>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

/**
 * @brief 状态容器模板
 *
 * 预计算并管理所有可能的状态组合。
 *
 * 参考: net.minecraft.state.StateContainer<O, S>
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
        std::vector<size_t>,
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
            if (!prop) {
                throw std::invalid_argument("Property cannot be null");
            }
            validateProperty(*prop);
            const std::string name = prop->name();
            const IProperty* rawProp = prop.get();
            m_ownedProperties.push_back(std::move(prop));
            m_properties[name] = rawProp;
            return *this;
        }

        /**
         * @brief 添加属性（借用引用，不转移所有权）
         */
        Builder& add(const IProperty* prop)
        {
            if (!prop) {
                throw std::invalid_argument("Property cannot be null");
            }
            validateProperty(*prop);
            m_properties[prop->name()] = prop;
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
        Builder& addBoolean(const std::string& name) { return add(BooleanProperty::create(name)); }

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
        Builder& addDirection(const std::string& name) { return add(DirectionProperty::create(name)); }

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
        Builder& addAxis(const std::string& name) { return add(AxisProperty::create(name)); }

        /**
         * @brief 构建状态容器
         */
        std::unique_ptr<StateContainer> create(StateFactory factory)
        {
            return std::unique_ptr<StateContainer>(
                new StateContainer(m_owner, std::move(m_properties), std::move(m_ownedProperties), factory));
        }

    private:
        Owner& m_owner;
        std::unordered_map<std::string, const IProperty*> m_properties;
        std::vector<std::unique_ptr<IProperty>> m_ownedProperties;

        void validateProperty(const IProperty& prop)
        {
            static const std::regex NAME_PATTERN("^[a-z0-9_]+$");
            if (!std::regex_match(prop.name(), NAME_PATTERN)) {
                throw std::invalid_argument("Invalid property name: " + prop.name());
            }
            if (prop.valueCount() <= 1) {
                throw std::invalid_argument("Property " + prop.name() + " must have more than 1 possible value");
            }
            for (size_t i = 0; i < prop.valueCount(); ++i) {
                if (!std::regex_match(prop.valueToString(i), NAME_PATTERN)) {
                    throw std::invalid_argument(
                        "Property " + prop.name() + " has invalid value name: " + prop.valueToString(i));
                }
            }
            if (m_properties.find(prop.name()) != m_properties.end()) {
                throw std::invalid_argument("Duplicate property: " + prop.name());
            }
        }
    };

    [[nodiscard]] const State& baseState() const { return *m_states[0]; }
    [[nodiscard]] const std::vector<std::unique_ptr<State>>& validStates() const { return m_states; }
    [[nodiscard]] size_t stateCount() const { return m_states.size(); }
    [[nodiscard]] const Owner& owner() const { return m_owner; }
    [[nodiscard]] const std::unordered_map<std::string, const IProperty*>& properties() const { return m_properties; }

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
                if (!first) ss << ", ";
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
        std::vector<std::unique_ptr<IProperty>> ownedProperties,
        StateFactory factory)
        : m_owner(owner)
        , m_properties(std::move(propertiesToTransfer))
        , m_ownedProperties(std::move(ownedProperties))
    {
        std::vector<const IProperty*> props;
        for (const auto& [name, prop] : m_properties) {
            props.push_back(prop);
        }
        generateStates(props, factory);
    }

    void generateStates(const std::vector<const IProperty*>& props, StateFactory factory)
    {
        size_t totalStates = 1;
        for (const auto* prop : props) {
            totalStates *= prop->valueCount();
        }

        m_propertyLayouts.clear();
        m_propertyLayouts.reserve(props.size());

        size_t stride = 1;
        for (const auto* prop : props) {
            m_propertyLayouts.push_back(PropertyLayout{prop, m_propertyLayouts.size(), stride});
            stride *= prop->valueCount();
        }

        m_states.reserve(totalStates);
        m_statePointers.clear();
        m_statePointers.reserve(totalStates);

        u32 stateId = 0;
        for (size_t flatIndex = 0; flatIndex < totalStates; ++flatIndex) {
            std::vector<size_t> valueIndices(props.size(), 0);
            size_t remaining = flatIndex;
            for (size_t propIndex = 0; propIndex < props.size(); ++propIndex) {
                const size_t valueCount = props[propIndex]->valueCount();
                valueIndices[propIndex] = remaining % valueCount;
                remaining /= valueCount;
            }

            auto state = factory(m_owner, std::move(valueIndices), &m_propertyLayouts, &m_statePointers, stateId);
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
