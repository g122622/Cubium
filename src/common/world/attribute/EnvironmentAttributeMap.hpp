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

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 环境属性表
 *
 * MC 1.21.11 final class EnvironmentAttributeMap。
 * 保存"属性 → (修改器, 参数)"的映射，用于维度类型（`dimension_type.attributes`）
 * 与群系（`biome.attributes`）声明的常量属性。
 *
 * 条目以属性地址为键；参数类型由修改器决定，而 apply 的返回值始终是属性值类型，
 * 因此每个条目都能以类型擦除的形式（`void* value` 进、出）服务属性系统的常量层。
 */
class EnvironmentAttributeMap {
public:
    /// 按值应用一个属性条目的函数：value 指向属性值，attribute 指向属性对象
    using AttributeApplier = std::function<void(void* value, const void* attribute)>;

    /// 一个条目
    class Entry {
    public:
        /**
         * @brief 构造条目
         *
         * @param operation 修改器操作标识
         * @param apply 应用函数（读取参数、调用修改器并写回 value）
         */
        Entry(AttributeModifierOperation operation, AttributeApplier apply)
            : m_operation(operation)
            , m_apply(std::move(apply))
        {}

        [[nodiscard]] AttributeModifierOperation operation() const noexcept { return m_operation; }

        void applyModifier(void* value, const void* attribute) const { m_apply(value, attribute); }

    private:
        AttributeModifierOperation m_operation;
        AttributeApplier m_apply;
    };

    EnvironmentAttributeMap() = default;
    EnvironmentAttributeMap(const EnvironmentAttributeMap&) = default;
    EnvironmentAttributeMap(EnvironmentAttributeMap&&) noexcept = default;
    EnvironmentAttributeMap& operator=(const EnvironmentAttributeMap&) = default;
    EnvironmentAttributeMap& operator=(EnvironmentAttributeMap&&) noexcept = default;
    ~EnvironmentAttributeMap();

    /// 判断是否包含某属性
    [[nodiscard]] bool contains(const void* attribute) const { return m_entries.contains(attribute); }

    /**
     * @brief 若包含该属性，按其修改器与参数改写 value
     *
     * @param attribute 属性地址
     * @param value 属性值（按属性的实际类型解释）
     */
    void applyModifier(const void* attribute, void* value) const
    {
        const auto it = m_entries.find(attribute);
        if (it != m_entries.end()) {
            it->second->applyModifier(value, attribute);
        }
    }

    [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_entries.size(); }

    /// 全部属性地址
    [[nodiscard]] std::vector<const void*> attributes() const
    {
        std::vector<const void*> result;
        result.reserve(m_entries.size());
        for (const auto& [attribute, entry] : m_entries) {
            result.push_back(attribute);
        }
        return result;
    }

    /**
     * @brief 属性表构建器
     */
    class Builder {
    public:
        /**
         * @brief 以覆写方式设置属性值
         *
         * @tparam Value 属性值类型
         * @param attribute 环境属性
         * @param value 值
         * @return 构建器自身
         */
        template <typename Value>
        Builder& set(const EnvironmentAttribute<Value>& attribute, Value value)
        {
            return modify(attribute, AttributeModifierOperation::Override, std::move(value));
        }

        /**
         * @brief 以指定修改器设置属性参数
         *
         * 覆写修改器直接取参数作为新值；其余修改器以属性默认值为基础值应用。
         *
         * @tparam Value 属性值类型
         * @param attribute 环境属性
         * @param operation 修改器操作标识（必须被属性类型允许）
         * @param argument 修改器参数（覆写修改器下即属性值）
         * @return 构建器自身
         */
        template <typename Value>
        Builder& modify(
            const EnvironmentAttribute<Value>& attribute, AttributeModifierOperation operation, Value argument)
        {
            MC_ASSERT_RELEASE_MSG(
                attribute.type().isAllowedModifier(operation), "EnvironmentAttributeMap: modifier not allowed");
            m_entries[&attribute] = std::make_shared<const Entry>(
                operation, [&attribute, operation, argument = std::move(argument)](void* value, const void*) {
                    auto* typedValue = static_cast<Value*>(value);
                    *typedValue = _apply(attribute, operation, *typedValue, argument);
                });
            return *this;
        }

        /**
         * @brief 合并另一张属性表（同名属性以参数表为准）
         */
        Builder& putAll(const EnvironmentAttributeMap& other)
        {
            for (const auto& [attribute, entry] : other.m_entries) {
                m_entries[attribute] = entry;
            }
            return *this;
        }

        [[nodiscard]] EnvironmentAttributeMap build() { return EnvironmentAttributeMap(m_entries); }

    private:
        template <typename Value>
        static Value _apply(const EnvironmentAttribute<Value>& attribute,
            AttributeModifierOperation operation,
            Value subject,
            const Value& argument)
        {
            if (operation == AttributeModifierOperation::Override) {
                return argument;
            }

            const auto& library = attribute.type().modifierLibrary();
            const auto it = library.find(operation);
            MC_ASSERT_RELEASE_MSG(it != library.end(), "EnvironmentAttributeMap: modifier missing from library");
            // 条目参数类型与属性值类型一致：直接以 Value 作为参数调用
            return it->second->apply(std::move(subject), argument);
        }

        std::map<const void*, std::shared_ptr<const Entry>> m_entries;
    };

    /// 属性地址 → 条目
    [[nodiscard]] const std::map<const void*, std::shared_ptr<const Entry>>& entries() const noexcept
    {
        return m_entries;
    }

private:
    explicit EnvironmentAttributeMap(std::map<const void*, std::shared_ptr<const Entry>> entries)
        : m_entries(std::move(entries))
    {}

    std::map<const void*, std::shared_ptr<const Entry>> m_entries;
};

inline EnvironmentAttributeMap::~EnvironmentAttributeMap() = default;

} // namespace attribute
} // namespace world
} // namespace mc