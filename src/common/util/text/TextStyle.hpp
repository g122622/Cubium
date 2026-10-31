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

#include "TextEvents.hpp"
#include "common/core/Types.hpp"
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

/**
 * @brief 文本格式化类型
 *
 * 对应 Minecraft 的 § 代码格式。
 * 参考: net.minecraft.util.text.TextFormatting
 */
enum class TextFormatting : u8 {
    // 颜色代码 (§0-§f)
    Black = 0,        // §0 - 黑色
    DarkBlue = 1,     // §1 - 深蓝
    DarkGreen = 2,    // §2 - 深绿
    DarkAqua = 3,     // §3 - 深青
    DarkRed = 4,      // §4 - 深红
    DarkPurple = 5,   // §5 - 深紫
    Gold = 6,         // §6 - 金色
    Gray = 7,         // §7 - 灰色
    DarkGray = 8,     // §8 - 深灰
    Blue = 9,         // §9 - 蓝色
    Green = 10,       // §a - 绿色
    Aqua = 11,        // §b - 青色
    Red = 12,         // §c - 红色
    LightPurple = 13, // §d - 浅紫
    Yellow = 14,      // §e - 黄色
    White = 15,       // §f - 白色

    // 样式代码
    Obfuscated = 16,    // §k - 混淆（随机字符）
    Bold = 17,          // §l - 粗体
    Strikethrough = 18, // §m - 删除线
    Underline = 19,     // §n - 下划线
    Italic = 20,        // §o - 斜体
    Reset = 21,         // §r - 重置

    // 特殊值
    None = 255 // 无格式
};

/**
 * @brief 获取格式化类型的颜色值（ARGB格式）
 *
 * 仅对颜色类型有效，对样式类型返回白色。
 *
 * @param formatting 格式化类型
 * @return ARGB颜色值
 */
[[nodiscard]] u32 getFormattingColor(TextFormatting formatting) noexcept;

/**
 * @brief 检查格式化类型是否为颜色
 * @param formatting 格式化类型
 * @return 如果是颜色类型返回 true
 */
[[nodiscard]] bool isColor(TextFormatting formatting) noexcept;

/**
 * @brief 检查格式化类型是否为样式
 * @param formatting 格式化类型
 * @return 如果是样式类型返回 true
 */
[[nodiscard]] bool isStyle(TextFormatting formatting) noexcept;

/**
 * @brief 从 § 代码字符获取格式化类型
 * @param code § 后的字符（0-9, a-f, k-o, r）
 * @return 对应的格式化类型，无效字符返回 None
 */
[[nodiscard]] TextFormatting fromCode(char code) noexcept;

/**
 * @brief 获取格式化类型对应的 § 代码字符
 * @param formatting 格式化类型
 * @return § 后的字符，None 返回 '\0'
 */
[[nodiscard]] char toCode(TextFormatting formatting) noexcept;

/**
 * @brief 从名称获取格式化类型
 * @param name 格式名称（如 "red", "bold", "dark_blue"）
 * @return 对应的格式化类型，无效名称返回 None
 */
[[nodiscard]] TextFormatting fromName(const std::string& name) noexcept;

/**
 * @brief 获取格式化类型的名称
 * @param formatting 格式化类型
 * @return 格式名称（如 "red", "bold"）
 */
[[nodiscard]] std::string toName(TextFormatting formatting);

/**
 * @brief 文本样式
 *
 * 包含颜色、样式标志和事件。
 * 参考: net.minecraft.util.text.Style
 */
class Style {
public:
    Style() = default;

    // ========== 颜色 ==========

    /**
     * @brief 获取颜色
     * @return 颜色格式化类型，未设置返回 nullopt
     */
    [[nodiscard]] std::optional<TextFormatting> getColor() const noexcept
    {
        return m_color;
    }

    /**
     * @brief 设置颜色
     * @param color 颜色格式化类型
     */
    void setColor(std::optional<TextFormatting> color) noexcept
    {
        m_color = color;
    }

    /**
     * @brief 获取颜色 ARGB 值
     * @return ARGB 颜色值，未设置颜色返回白色
     */
    [[nodiscard]] u32 getColorARGB() const noexcept;

    // ========== 样式标志 ==========
    //
    // 五个标志都是**三态**（未设置 / true / false），与 vanilla `Style` 的
    // `@Nullable Boolean` 一致。三态是必须的：序列化时「显式写成 false」与「没写」在原版
    // 存档里是两种不同的值（前者要写出 `italic:0b`，后者省略），判定父子样式继承时也
    // 只看子样式是否**显式**给出。用 bool 会把这两者并成一种，界面看不出差别，
    // 存档与网络字节却与原版不同。

    /** 粗体：nullopt = 未设置（继承父样式）。 */
    [[nodiscard]] std::optional<bool> bold() const noexcept
    {
        return m_bold;
    }

    /** 语义同 {@link bold}：是否**确定为**粗体（未设置也算否）。 */
    [[nodiscard]] bool isBold() const noexcept
    {
        return m_bold.value_or(false);
    }

    void setBold(std::optional<bool> bold) noexcept
    {
        m_bold = bold;
    }

    [[nodiscard]] std::optional<bool> italic() const noexcept
    {
        return m_italic;
    }

    [[nodiscard]] bool isItalic() const noexcept
    {
        return m_italic.value_or(false);
    }

    void setItalic(std::optional<bool> italic) noexcept
    {
        m_italic = italic;
    }

    [[nodiscard]] std::optional<bool> underlined() const noexcept
    {
        return m_underlined;
    }

    [[nodiscard]] bool isUnderlined() const noexcept
    {
        return m_underlined.value_or(false);
    }

    void setUnderlined(std::optional<bool> underlined) noexcept
    {
        m_underlined = underlined;
    }

    [[nodiscard]] std::optional<bool> strikethrough() const noexcept
    {
        return m_strikethrough;
    }

    [[nodiscard]] bool isStrikethrough() const noexcept
    {
        return m_strikethrough.value_or(false);
    }

    void setStrikethrough(std::optional<bool> strikethrough) noexcept
    {
        m_strikethrough = strikethrough;
    }

    [[nodiscard]] std::optional<bool> obfuscated() const noexcept
    {
        return m_obfuscated;
    }

    [[nodiscard]] bool isObfuscated() const noexcept
    {
        return m_obfuscated.value_or(false);
    }

    void setObfuscated(std::optional<bool> obfuscated) noexcept
    {
        m_obfuscated = obfuscated;
    }

    // ========== 事件 ==========

    /**
     * @brief 获取点击事件
     * @return 点击事件指针，未设置返回 nullptr
     */
    [[nodiscard]] const ClickEvent* getClickEvent() const noexcept
    {
        return m_clickEvent.has_value() ? &(*m_clickEvent) : nullptr;
    }

    /**
     * @brief 设置点击事件
     * @param event 点击事件
     */
    void setClickEvent(const std::optional<ClickEvent>& event) noexcept
    {
        m_clickEvent = event;
    }

    /**
     * @brief 获取悬停事件
     * @return 悬停事件指针，未设置返回 nullptr
     */
    [[nodiscard]] const HoverEvent* getHoverEvent() const noexcept
    {
        return m_hoverEvent.has_value() ? &(*m_hoverEvent) : nullptr;
    }

    /**
     * @brief 设置悬停事件
     * @param event 悬停事件
     */
    void setHoverEvent(const std::optional<HoverEvent>& event) noexcept
    {
        m_hoverEvent = event;
    }

    // ========== 父样式继承 ==========

    /**
     * @brief 从父样式继承未设置的属性
     * @param parent 父样式
     * @return 合并后的新样式
     */
    [[nodiscard]] Style mergeWithParent(const Style& parent) const noexcept;

    /**
     * @brief 检查样式是否为空（所有属性未设置）
     * @return 如果所有属性都是默认值返回 true
     */
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief 检查样式是否有事件
     * @return 如果有点击或悬停事件返回 true
     */
    [[nodiscard]] bool hasEvents() const noexcept
    {
        return m_clickEvent.has_value() || m_hoverEvent.has_value();
    }

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 JSON
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 对象
     * @return 样式对象
     */
    static Style fromJson(const nlohmann::json& json);

    // ========== 比较 ==========

    bool operator==(const Style& other) const noexcept;

    bool operator!=(const Style& other) const noexcept
    {
        return !(*this == other);
    }

private:
    std::optional<TextFormatting> m_color;
    std::optional<bool> m_bold;
    std::optional<bool> m_italic;
    std::optional<bool> m_underlined;
    std::optional<bool> m_strikethrough;
    std::optional<bool> m_obfuscated;
    std::optional<ClickEvent> m_clickEvent;
    std::optional<HoverEvent> m_hoverEvent;
};

/**
 * @brief 获取样式的 § 代码前缀
 *
 * 例如：红色+粗体 -> "§c§l"
 *
 * @param style 样式
 * @return § 代码字符串
 */
[[nodiscard]] std::string getStyleCodes(const Style& style);

} // namespace mc::text
