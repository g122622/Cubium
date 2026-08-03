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

#include "TextStyle.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

/**
 * @brief 文本组件接口
 *
 * 表示一段富文本，支持样式继承和嵌套。
 * 参考: net.minecraft.util.text.ITextComponent
 *
 * ## 继承关系
 *
 * ITextComponent 是抽象接口，有以下实现：
 * - StringTextComponent: 纯文本
 * - TranslationTextComponent: 翻译键文本
 *
 * ## 样式继承
 *
 * 子组件会继承父组件的样式，但可以覆盖：
 * ```
 * 父组件: §c红色
 *   子组件: §l粗体  -> 显示为 红色+粗体
 * ```
 *
 * ## JSON 格式
 *
 * 基础格式：
 * ```json
 * {"text": "Hello", "color": "red", "bold": true}
 * ```
 *
 * 嵌套格式：
 * ```json
 * {
 *   "text": "Hello ",
 *   "color": "red",
 *   "extra": [
 *     {"text": "World", "color": "blue"}
 *   ]
 * }
 * ```
 */
class ITextComponent {
public:
    virtual ~ITextComponent() = default;

    // ========== 核心方法 ==========

    /**
     * @brief 获取纯文本内容
     *
     * 递归合并所有子组件的文本，不含样式代码。
     *
     * @return 纯文本字符串
     */
    [[nodiscard]] virtual std::string getUnformattedText() const = 0;

    /**
     * @brief 获取带样式文本
     *
     * 递归合并所有子组件的文本，包含 § 样式代码。
     * 例如: "§cHello §lWorld"
     *
     * @return 带 § 代码的文本
     */
    [[nodiscard]] virtual std::string getFormattedText() const = 0;

    // ========== 样式 ==========

    /**
     * @brief 获取样式
     * @return 当前组件的样式
     */
    [[nodiscard]] virtual const Style& getStyle() const = 0;

    /**
     * @brief 设置样式
     * @param style 样式对象
     */
    virtual void setStyle(const Style& style) = 0;

    /**
     * @brief 获取合并后的样式（包含父样式继承）
     * @param parentStyle 父组件样式
     * @return 合并后的样式
     */
    [[nodiscard]] Style getMergedStyle(const Style& parentStyle) const
    {
        return getStyle().mergeWithParent(parentStyle);
    }

    // ========== 子组件 ==========

    /**
     * @brief 获取子组件列表
     * @return 子组件的常量引用
     */
    [[nodiscard]] virtual const std::vector<std::unique_ptr<ITextComponent>>& getSiblings() const = 0;

    /**
     * @brief 添加子组件
     * @param sibling 子组件（所有权转移）
     */
    virtual void append(std::unique_ptr<ITextComponent> sibling) = 0;

    /**
     * @brief 便捷方法：追加纯文本子组件
     * @param text 文本内容
     */
    void appendText(const std::string& text);

    /**
     * @brief 深拷贝
     *
     * 创建当前组件及其所有子组件的完整副本。
     *
     * @return 新组件的所有权
     */
    [[nodiscard]] virtual std::unique_ptr<ITextComponent> deepCopy() const = 0;

    /**
     * @brief 浅拷贝（仅当前组件，不含子组件）
     * @return 新组件的所有权
     */
    [[nodiscard]] virtual std::unique_ptr<ITextComponent> shallowCopy() const = 0;

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 JSON
     * @return JSON 对象
     */
    [[nodiscard]] virtual nlohmann::json toJson() const = 0;

    /**
     * @brief 从 JSON 反序列化
     *
     * 根据 JSON 内容自动创建正确的子类实例。
     *
     * @param json JSON 对象
     * @return 文本组件的所有权
     */
    static std::unique_ptr<ITextComponent> fromJson(const nlohmann::json& json);

    /**
     * @brief 从 JSON 数组反序列化
     *
     * 将 JSON 数组转换为组件链。
     *
     * @param jsonArray JSON 数组
     * @return 第一个组件（其余组件作为子组件）
     */
    static std::unique_ptr<ITextComponent> fromJsonArray(const nlohmann::json& jsonArray);

    // ========== 比较 ==========

    bool operator==(const ITextComponent& other) const;
    bool operator!=(const ITextComponent& other) const { return !(*this == other); }
};

/**
 * @brief 文本组件基类
 *
 * 提供 ITextComponent 的通用实现，包括样式管理和子组件管理。
 */
class BaseTextComponent : public ITextComponent {
public:
    BaseTextComponent() = default;
    ~BaseTextComponent() override = default;

    // ========== 样式 ==========

    [[nodiscard]] const Style& getStyle() const override { return m_style; }
    void setStyle(const Style& style) override { m_style = style; }

    // ========== 子组件 ==========

    [[nodiscard]] const std::vector<std::unique_ptr<ITextComponent>>& getSiblings() const override
    {
        return m_siblings;
    }

    void append(std::unique_ptr<ITextComponent> sibling) override
    {
        if (sibling) {
            m_siblings.push_back(std::move(sibling));
        }
    }

    // ========== 辅助方法 ==========

protected:
    /**
     * @brief 拷贝基类成员
     * @param target 目标组件
     */
    void copyBaseTo(BaseTextComponent& target) const
    {
        target.m_style = m_style;
        for (const auto& sibling : m_siblings) {
            target.m_siblings.push_back(sibling->deepCopy());
        }
    }

    Style m_style;
    std::vector<std::unique_ptr<ITextComponent>> m_siblings;
};

// ========== 内联实现 ==========

inline bool ITextComponent::operator==(const ITextComponent& other) const
{
    // 比较文本内容和样式
    return getUnformattedText() == other.getUnformattedText() && getStyle() == other.getStyle() &&
        getSiblings().size() == other.getSiblings().size();
}

} // namespace mc::text
