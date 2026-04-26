#pragma once

#include "Property.hpp"
#include <unordered_map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>

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
template<typename Owner, typename State>
class StateHolder {
public:
    virtual ~StateHolder() = default;

    /**
     * @brief 获取状态的拥有者
     */
    [[nodiscard]] const Owner& owner() const {
        return *m_owner;
    }

    /**
     * @brief 获取属性值
     * @note 对于 bool 类型返回值而非引用，因为 std::vector<bool> 特化
     */
    template<typename T>
    [[nodiscard]] typename Property<T>::ValueReturnType get(const Property<T>& prop) const {
        auto it = m_values.find(&prop);
        if (it == m_values.end()) {
            throw std::invalid_argument(
                "Cannot get property " + prop.name() + " as it does not exist in " + ownerName()
            );
        }
        return static_cast<const Property<T>&>(prop).valueAt(it->second);
    }

    /**
     * @brief 尝试获取属性值
     * @note 对于 bool 类型返回值而非引用
     */
    template<typename T>
    [[nodiscard]] std::optional<T> getOptional(const Property<T>& prop) const {
        auto it = m_values.find(&prop);
        if (it == m_values.end()) {
            return std::nullopt;
        }
        return static_cast<const Property<T>&>(prop).valueAt(it->second);
    }

    /**
     * @brief 设置属性值，返回新状态
     */
    template<typename T>
    [[nodiscard]] const State& with(const Property<T>& prop, const T& value) const {
        auto it = m_values.find(&prop);
        if (it == m_values.end()) {
            throw std::invalid_argument(
                "Cannot set property " + prop.name() + " as it does not exist in " + ownerName()
            );
        }

        auto optIndex = prop.indexOf(value);
        if (!optIndex) {
            throw std::invalid_argument("Invalid value for property " + prop.name());
        }

        if (it->second == *optIndex) {
            return static_cast<const State&>(*this);
        }

        auto transIt = m_transitions.find(&prop);
        if (transIt == m_transitions.end() || transIt->second.size() <= *optIndex) {
            throw std::invalid_argument(
                "Cannot set property " + prop.name() + " to " + prop.valueToString(value) +
                " on " + ownerName() + ", it is not an allowed value"
            );
        }

        return *transIt->second[*optIndex];
    }

    /**
     * @brief 循环切换到下一个属性值
     */
    template<typename T>
    [[nodiscard]] const State& cycle(const Property<T>& prop) const {
        auto it = m_values.find(&prop);
        if (it == m_values.end()) {
            throw std::invalid_argument(
                "Cannot cycle property " + prop.name() + " as it does not exist in " + ownerName()
            );
        }

        const auto& values = prop.allowedValues();
        size_t currentIndex = it->second;
        size_t nextIndex = (currentIndex + 1) % values.size();

        return with(prop, values[nextIndex]);
    }

    /**
     * @brief 检查是否有此属性
     */
    template<typename T>
    [[nodiscard]] bool hasProperty(const Property<T>& prop) const {
        return m_values.find(&prop) != m_values.end();
    }

    /**
     * @brief 获取所有属性值（内部索引表示）
     */
    [[nodiscard]] const std::unordered_map<const IProperty*, size_t>& values() const {
        return m_values;
    }

    /**
     * @brief 获取状态ID
     */
    [[nodiscard]] u32 stateId() const {
        return m_stateId;
    }

    /**
     * @brief 转换为字符串表示
     */
    [[nodiscard]] String toString() const {
        std::ostringstream ss;
        ss << ownerName();
        if (!m_values.empty()) {
            ss << '[';
            bool first = true;
            for (const auto& [prop, valueIndex] : m_values) {
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
    [[nodiscard]] bool operator==(const StateHolder& other) const {
        return m_stateId == other.m_stateId;
    }

    [[nodiscard]] bool operator!=(const StateHolder& other) const {
        return m_stateId != other.m_stateId;
    }

protected:
    StateHolder(const Owner* owner,
                std::unordered_map<const IProperty*, size_t> values,
                u32 stateId)
        : m_owner(owner)
        , m_values(std::move(values))
        , m_stateId(stateId) {
    }

    /**
     * @brief 初始化转换表（由StateContainer调用）
     */
    void initTransitions(std::unordered_map<const IProperty*, std::vector<const State*>> transitions) {
        m_transitions = std::move(transitions);
    }

    /**
     * @brief 设置状态ID（由BlockRegistry调用）
     */
    void setStateId(u32 id) {
        m_stateId = id;
    }

    /**
     * @brief 获取拥有者名称（子类可重写）
     */
    [[nodiscard]] virtual String ownerName() const {
        return "Unknown";
    }

    const Owner* m_owner;
    std::unordered_map<const IProperty*, size_t> m_values;
    u32 m_stateId;
    std::unordered_map<const IProperty*, std::vector<const State*>> m_transitions;

    // 允许StateContainer和BlockRegistry访问
    template<typename O, typename S>
    friend class StateContainer;
    friend class BlockRegistry;
};

} // namespace mc
