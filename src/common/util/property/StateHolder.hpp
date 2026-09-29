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
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/IProperty.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace mc {

/**
 * @brief 单个状态对象允许的属性数上限
 *
 * 属性值索引改为内联存储（见 StateHolder::m_valueIndices）后，数组必须有编译期上界。
 * 取 8 的依据：全项目 174 个 `StateContainer::Builder` 的 `.add` 链长度实测分布为
 * 1 个属性 65 个、2 个 40、3 个 32、4 个 7、5 个 9、6 个 4、7 个 3，**最长 7**
 * （`TripWireBlock` / `MultifaceBlock` / `TrailsBlocks`）。
 *
 * 超出时 `StateContainer::generateStates` 会当场断言（而非静默截断）：属性被丢掉会让
 * `with()` 算出错误的 targetIndex，进而让方块切到不相干的状态，且不产生任何报错。
 *
 * 这个上限同时约束了热路径：`findPropertySlot` 的线性扫描最长 8 次指针比较。
 */
inline constexpr size_t MAX_STATE_PROPERTIES = 8;

/// 单个属性值索引。属性值数量实测上限 26（`age` [0,25]），u8 足够。
using StateValueIndex = u8;

/// 一个状态对象各属性的值索引数组（按 PropertyLayout::slotIndex 对齐）
using StateValueIndices = std::array<StateValueIndex, MAX_STATE_PROPERTIES>;

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

    /// 单个属性值索引的类型别名（等价于 mc::StateValueIndex）
    using ValueIndex = StateValueIndex;

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
     * @brief 通过 IProperty 接口和值索引设置属性值，返回新状态
     *
     * 类型擦除版本的 with() 方法，允许在编译期不知道属性具体类型时设置属性值。
     * 典型用途：从 BlockStateTag JSON/NBT 数据中解析属性名和字符串值后设置方块状态。
     *
     * @param prop 属性指针（通过 StateContainer::getProperty(name) 获取）
     * @param valueIndex 属性值索引（通过 IProperty::parseValue(string) 获取）
     * @return 设置属性值后的新状态引用；如果属性不存在或值索引无效则返回当前状态
     */
    [[nodiscard]] const State& withValueIndex(const IProperty& prop, size_t valueIndex) const
    {
        const size_t slotIndex = findPropertySlot(prop);
        if (slotIndex == kInvalidIndex) {
            return static_cast<const State&>(*this);
        }

        if (valueIndex >= prop.valueCount()) {
            return static_cast<const State&>(*this);
        }

        if (m_valueIndices[slotIndex] == valueIndex) {
            return static_cast<const State&>(*this);
        }

        const PropertyLayout& layout = propertyLayouts()[slotIndex];
        const size_t currentIndex = static_cast<size_t>(m_stateIndex);
        const size_t currentValueIndex = m_valueIndices[slotIndex];
        const size_t targetIndex = currentIndex + ((valueIndex - currentValueIndex) * layout.stateStride);
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
        for (size_t i = 0; i < m_propertyCount; ++i) {
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
        if (m_propertyCount > 0) {
            ss << '[';
            bool first = true;
            for (size_t i = 0; i < m_propertyCount; ++i) {
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
     * @brief 从源状态复制所有共有属性到当前状态
     *
     * 遍历源状态的所有属性，对于当前状态也拥有的同名属性（通过属性指针匹配），
     * 将源状态的属性值复制过来。返回复制后的新状态引用。
     *
     * 典型用途：铜方块氧化时，将当前方块的状态属性（如FACING、HALF等）
     * 复制到下一氧化等级方块的默认状态上。
     *
     * @param source 源状态
     * @return 复制共有属性后的新状态引用
     */
    [[nodiscard]] const State& withPropertiesOf(const StateHolder& source) const
    {
        const State* result = &static_cast<const State&>(*this);
        const auto& sourceLayouts = source.propertyLayouts();
        for (size_t i = 0; i < source.m_propertyCount; ++i) {
            const IProperty* sourceProp = sourceLayouts[i].property;
            // 通过属性指针匹配检查当前状态是否也有此属性
            size_t targetSlot = findPropertySlot(*sourceProp);
            if (targetSlot != kInvalidIndex && propertyLayouts()[targetSlot].property == sourceProp) {
                size_t sourceValueIndex = source.m_valueIndices[i];
                // 使用result的当前值索引（而非this的），因为前面的属性复制可能已经改变了result
                size_t currentValueIndex = result->m_valueIndices[targetSlot];
                if (sourceValueIndex != currentValueIndex) {
                    const PropertyLayout& targetLayout = propertyLayouts()[targetSlot];
                    size_t currentIdx = static_cast<size_t>(result->m_stateIndex);
                    size_t targetIdx = currentIdx + ((sourceValueIndex - currentValueIndex) * targetLayout.stateStride);
                    result = &(*(*m_allStates)[targetIdx]);
                }
            }
        }
        return *result;
    }

    /**
     * @brief 比较两个状态是否相等
     */
    [[nodiscard]] bool operator==(const StateHolder& other) const { return m_stateId == other.m_stateId; }

    [[nodiscard]] bool operator!=(const StateHolder& other) const { return m_stateId != other.m_stateId; }

protected:
    StateHolder(const Owner* owner,
        std::array<ValueIndex, MAX_STATE_PROPERTIES> valueIndices,
        size_t propertyCount,
        const std::vector<PropertyLayout>* propertyLayouts,
        const std::vector<State*>* allStates,
        u32 stateId)
        : m_owner(owner)
        , m_valueIndices(valueIndices)
        , m_propertyCount(static_cast<u8>(propertyCount))
        , m_propertyLayouts(propertyLayouts != nullptr ? propertyLayouts : &emptyPropertyLayouts())
        , m_allStates(allStates)
        , m_stateIndex(stateId)
        , m_stateId(stateId)
    {
        MC_ASSERT_RELEASE_MSG(
            propertyCount <= MAX_STATE_PROPERTIES, "StateHolder: property count exceeds MAX_STATE_PROPERTIES");
    }

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
        const size_t count = m_propertyCount < layouts.size() ? m_propertyCount : layouts.size();
        for (size_t i = 0; i < count; ++i) {
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

    /// 各属性的值索引，与 `*m_propertyLayouts` 的 slotIndex 一一对应。
    ///
    /// **内联定长数组而非 `std::vector<size_t>`**：一个 `BlockState` 是一个 value
    /// 对象、全局有 29 294 个，若每个都持一个 vector，就要为每个状态对象单独申请一块
    /// 堆内存（实测 `StateContainer::generateStates` 分配 31 925 次 × 64 B = 1.95 MB），
    /// 外加 vector 头 24 B × 29 294 = 0.67 MB。两项都不随状态内容变化，是纯粹的
    /// "容器开销"。改用 `array<u8, 8>` 后：
    ///   - 堆分配整块消失（−1.95 MB，且堆节点数 −31 925，约 −12%）
    ///   - 对象内从 24 B 降到 9 B（8 个 u8 + 1 个计数），BlockState 112 → 96 B
    /// u8 足够：属性值数量实测上限 26（`age` [0,25]），存量类型也要求更大时
    /// 会由 `static_assert` 或断言当场暴露，不会静默截断。
    std::array<ValueIndex, MAX_STATE_PROPERTIES> m_valueIndices{};

    /// 实际使用的属性槽位数（<= MAX_STATE_PROPERTIES）。
    /// 与 `m_propertyLayouts->size()` 冗余，此处单存一份是为了让属性数量可内联读取
    /// （`values()` / `toString()` 的循环上界不再依赖指针解引用）。
    u8 m_propertyCount = 0;

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
