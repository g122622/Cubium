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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// Forward declarations
class BlockState;
class IProperty;

/**
 * @brief 方块状态属性匹配谓词
 *
 * 用于检查 BlockState 的属性值是否匹配指定条件。
 * 支持精确匹配和范围匹配。
 *
 * 参考: net.minecraft.advancements.criterion.StatePropertiesPredicate
 *
 * 用法示例:
 * @code
 * StatePropertiesPredicate predicate;
 * predicate.addExactMatch("age", "3");
 * predicate.addRangeMatch("power", "5", "10");
 *
 * if (predicate.matches(*blockState)) {
 *     // 匹配成功
 * }
 * @endcode
 */
class StatePropertiesPredicate {
public:
    /**
     * @brief 属性匹配器基类
     */
    class Matcher {
    public:
        explicit Matcher(std::string propertyName);
        virtual ~Matcher() = default;

        /**
         * @brief 匹配方块状态中的属性值
         * @param state 方块状态
         * @return 如果匹配成功返回 true
         */
        [[nodiscard]] bool match(const BlockState& state) const;

        /**
         * @brief 获取属性名称
         */
        [[nodiscard]] const std::string& propertyName() const { return m_propertyName; }

        /**
         * @brief 序列化为 JSON 格式
         */
        [[nodiscard]] virtual std::string toJson() const = 0;

        /**
         * @brief 克隆匹配器
         */
        [[nodiscard]] virtual std::unique_ptr<Matcher> clone() const = 0;

    protected:
        /**
         * @brief 子类实现的精确匹配逻辑
         * @param state 方块状态
         * @param property 属性指针
         * @return 如果匹配成功返回 true
         */
        [[nodiscard]] virtual bool matchExact(const BlockState& state, const IProperty* property) const = 0;

        std::string m_propertyName;
    };

    /**
     * @brief 精确匹配器
     *
     * 匹配属性值是否等于指定值。
     */
    class ExactMatcher : public Matcher {
    public:
        /**
         * @brief 构造精确匹配器
         * @param propertyName 属性名称
         * @param value 要匹配的值（字符串形式）
         */
        ExactMatcher(const std::string& propertyName, std::string value);

        [[nodiscard]] std::string toJson() const override;
        [[nodiscard]] std::unique_ptr<Matcher> clone() const override;

        [[nodiscard]] const std::string& value() const { return m_value; }

    protected:
        [[nodiscard]] bool matchExact(const BlockState& state, const IProperty* property) const override;

    private:
        std::string m_value;
    };

    /**
     * @brief 范围匹配器
     *
     * 匹配属性值是否在指定范围内（支持整数、枚举等可比较类型）。
     */
    class RangedMatcher : public Matcher {
    public:
        /**
         * @brief 构造范围匹配器
         * @param propertyName 属性名称
         * @param min 最小值（可选，空表示无下界）
         * @param max 最大值（可选，空表示无上界）
         */
        RangedMatcher(const std::string& propertyName, std::optional<std::string> min, std::optional<std::string> max);

        [[nodiscard]] std::string toJson() const override;
        [[nodiscard]] std::unique_ptr<Matcher> clone() const override;

        [[nodiscard]] const std::optional<std::string>& minValue() const { return m_min; }
        [[nodiscard]] const std::optional<std::string>& maxValue() const { return m_max; }

    protected:
        [[nodiscard]] bool matchExact(const BlockState& state, const IProperty* property) const override;

    private:
        std::optional<std::string> m_min;
        std::optional<std::string> m_max;
    };

    /**
     * @brief 空谓词（不匹配任何属性）
     */
    static StatePropertiesPredicate EMPTY;

    /**
     * @brief 默认构造函数
     */
    StatePropertiesPredicate() = default;

    /**
     * @brief 构造函数
     * @param matchers 匹配器列表
     */
    explicit StatePropertiesPredicate(std::vector<std::unique_ptr<Matcher>> matchers);

    /**
     * @brief 拷贝构造函数
     */
    StatePropertiesPredicate(const StatePropertiesPredicate& other);

    /**
     * @brief 拷贝赋值运算符
     */
    StatePropertiesPredicate& operator=(const StatePropertiesPredicate& other);

    /**
     * @brief 移动构造函数
     */
    StatePropertiesPredicate(StatePropertiesPredicate&&) noexcept = default;

    /**
     * @brief 移动赋值运算符
     */
    StatePropertiesPredicate& operator=(StatePropertiesPredicate&&) noexcept = default;

    /**
     * @brief 检查方块状态是否匹配所有属性条件
     * @param state 方块状态
     * @return 如果所有条件都匹配返回 true
     */
    [[nodiscard]] bool matches(const BlockState& state) const;

    /**
     * @brief 检查是否为空谓词
     * @return 如果没有任何匹配器返回 true
     */
    [[nodiscard]] bool isEmpty() const { return m_matchers.empty(); }

    /**
     * @brief 获取匹配器数量
     */
    [[nodiscard]] size_t matcherCount() const { return m_matchers.size(); }

    /**
     * @brief 添加精确匹配器
     * @param propertyName 属性名称
     * @param value 要匹配的值
     */
    void addExactMatch(const std::string& propertyName, const std::string& value);

    /**
     * @brief 添加范围匹配器
     * @param propertyName 属性名称
     * @param min 最小值（可选）
     * @param max 最大值（可选）
     */
    void addRangeMatch(const std::string& propertyName, std::optional<std::string> min, std::optional<std::string> max);

    /**
     * @brief 添加匹配器
     * @param matcher 匹配器
     */
    void addMatcher(std::unique_ptr<Matcher> matcher);

    /**
     * @brief 获取匹配器列表
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Matcher>>& matchers() const { return m_matchers; }

    /**
     * @brief 序列化为 JSON 格式
     * @return JSON 字符串
     */
    [[nodiscard]] std::string toJson() const;

    /**
     * @brief 从 JSON 解析状态属性谓词
     *
     * JSON 格式（MC 1.16.5 StatePropertiesPredicate）：
     * {
     *   "age": "3",                    // 精确匹配
     *   "power": { "min": "5" },       // 范围匹配（仅最小值）
     *   "level": { "max": "10" },      // 范围匹配（仅最大值）
     *   "facing": { "min": "north", "max": "south" }  // 范围匹配
     * }
     *
     * @param json JSON 对象
     * @return 解析结果
     */
    [[nodiscard]] static Result<StatePropertiesPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 转换为 nlohmann::json 对象
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJsonValue() const;

    /**
     * @brief 遍历不存在的属性
     *
     * 检查谓词中的属性是否存在于指定方块的 StateContainer 中，
     * 对不存在的属性调用回调函数。
     *
     * @param stateContainer 方块的状态容器
     * @param callback 不存在属性的回调函数
     */
    template <typename StateContainer>
    void forEachMissingProperty(
        const StateContainer& stateContainer, std::function<void(const std::string&)> callback) const
    {
        for (const auto& matcher : m_matchers) {
            const IProperty* prop = stateContainer.getProperty(matcher->propertyName());
            if (prop == nullptr) {
                callback(matcher->propertyName());
            }
        }
    }

private:
    std::vector<std::unique_ptr<Matcher>> m_matchers;
};

} // namespace mc
