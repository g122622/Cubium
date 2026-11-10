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
 * @brief 文本颜色，对应原版 `net.minecraft.network.chat.TextColor`
 *
 * 同时覆盖两种来源：
 * - **具名颜色**：由 `TextFormatting` 的 16 个颜色码构造，序列化时写**名字**（`"red"`）；
 * - **自定义 RGB**：由 `fromRgb` 构造，序列化时写 `"#RRGGBB"`。
 *
 * 两者的 JSON 文本不同（`"red"` vs `"#FF5555"`），即使 RGB 值相同——存档与网络
 * 逐字节比对时这是必须区分的，所以内部保留 `name` 字段而不是只存 RGB。
 */
class TextColor {
public:
    /**
     * @brief 由 RGB 构造自定义颜色，对应原版 `TextColor.fromRgb`
     *
     * 只保留低 24 位；传入 `0xFF5555` 与 `0x00FF5555` 等价。
     */
    [[nodiscard]] static TextColor fromRgb(i32 rgb);

    /**
     * @brief 由格式化枚举构造具名颜色，对应原版 `TextColor.fromLegacyFormat`
     *
     * 传入样式码（加粗、斜体等）或 `Reset`/`None` 时返回 `std::nullopt`——
     * 它们不对应任何颜色。
     */
    [[nodiscard]] static std::optional<TextColor> fromLegacyFormat(TextFormatting formatting);

    /**
     * @brief 解析 `"red"` 或 `"#RRGGBB"`，对应原版 `TextColor.parseColor`
     *
     * 具名颜色必须与 16 个颜色名**完全匹配**（区分大小写、不接受样式名）；
     * 十六进制必须是 `#` 加**恰好 6 位**十六进制数字。任何不符返回 `std::nullopt`。
     */
    [[nodiscard]] static std::optional<TextColor> parseColor(std::string_view text);

    /**
     * @brief 序列化成 JSON 字符串形式
     *
     * 具名颜色返回名字，自定义颜色返回 `"#RRGGBB"`（**大写十六进制**）。
     */
    [[nodiscard]] std::string serialize() const;

    /**
     * @brief RGB 值（不含 alpha）
     */
    [[nodiscard]] i32 value() const noexcept
    {
        return m_value;
    }

    /**
     * @brief 具名颜色的名字；自定义颜色为空
     */
    [[nodiscard]] const std::optional<std::string>& name() const noexcept
    {
        return m_name;
    }

    bool operator==(const TextColor& other) const noexcept
    {
        return m_value == other.m_value && m_name == other.m_name;
    }

private:
    TextColor(i32 value, std::optional<std::string> name);

    i32 m_value = 0;
    std::optional<std::string> m_name;
};

/**
 * @brief 获取格式化类型的颜色值（ARGB格式），样式类型返回 std::nullopt
 *
 * 对应原版 `ChatFormatting#getColor()`：**只有 16 个颜色码有颜色**，
 * 6 个样式码（obfuscated / bold / strikethrough / underline / italic / reset）
 * 与原版一致返回「无颜色」。
 *
 * 需要「无颜色即白色」的渲染场景请用 getFormattingColor，但**判断某类型是否
 * 真的带颜色必须用本函数或 isColor**，不能用 `getFormattingColor(x) != 0xFFFFFFFF`
 * ——因为白色本身就是一个合法颜色（`TextFormatting::White`）。
 *
 * @param formatting 格式化类型
 * @return 有颜色时返回 ARGB 值，样式类型返回 std::nullopt
 */
[[nodiscard]] std::optional<u32> findFormattingARGB(TextFormatting formatting) noexcept;

/**
 * @brief 获取格式化类型的颜色值（ARGB格式）
 *
 * 仅对颜色类型有效，对样式类型返回白色（`0xFFFFFFFF`）。
 *
 * **这是渲染便捷入口，不表达能力语义**：如需区分「无颜色」与「白色」，
 * 请改用 findFormattingARGB。
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
     * @brief 获取颜色（`TextColor` 形式）
     *
     * 这是完整形式，能同时承载具名色与自定义 RGB。
     */
    [[nodiscard]] const std::optional<TextColor>& getTextColor() const noexcept
    {
        return m_color;
    }

    /**
     * @brief 获取具名颜色
     *
     * 颜色是自定义 RGB（非具名）时返回 `std::nullopt`——那些值没有对应的
     * `TextFormatting`。需要拿到任意颜色请用 `getTextColor()`。
     */
    [[nodiscard]] std::optional<TextFormatting> getColor() const noexcept;

    /**
     * @brief 设置颜色（`TextColor` 形式）
     */
    void setTextColor(std::optional<TextColor> color) noexcept
    {
        m_color = std::move(color);
    }

    /**
     * @brief 设置具名颜色
     *
     * 传 `std::nullopt` 表示清除颜色。
     */
    void setColor(std::optional<TextFormatting> color) noexcept
    {
        if (!color.has_value()) {
            m_color = std::nullopt;
            return;
        }
        m_color = TextColor::fromLegacyFormat(*color);
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

    /**
     * @brief 获取插入文本，对应原版 `Style.getInsertion`
     *
     * 玩家点击该文本组件时，这段字符串会被插入聊天输入框。
     */
    [[nodiscard]] const std::optional<std::string>& getInsertion() const noexcept
    {
        return m_insertion;
    }

    /**
     * @brief 设置插入文本，对应原版 `Style.withInsertion`
     */
    void setInsertion(std::optional<std::string> insertion) noexcept
    {
        m_insertion = std::move(insertion);
    }

    // ========== 阴影色与字体 ==========

    /**
     * @brief 获取文字阴影色，对应原版 `Style.getShadowColor`
     *
     * 原版字段类型是 `@Nullable Integer`，值是 **ARGB 打包整数**（例如 `-16777216`
     * 表示不透明黑），不是 `#RRGGBB` 字符串——`ExtraCodecs.ARGB_COLOR_CODEC`
     * 以 `Codec.INT` 为主形态。未设置时返回 nullopt。
     */
    [[nodiscard]] const std::optional<i32>& getShadowColor() const noexcept
    {
        return m_shadowColor;
    }

    /**
     * @brief 设置文字阴影色，对应原版 `Style.withShadowColor`
     */
    void setShadowColor(std::optional<i32> shadowColor) noexcept
    {
        m_shadowColor = shadowColor;
    }

    /**
     * @brief 获取字体，对应原版 `Style.getFont`
     *
     * 原版 1.21.11 的 `FontDescription.CODEC` 只支持 `FontDescription.Resource`（用
     * `Identifier.CODEC.flatComapMap` 构造），因此形态就是**资源位置字符串**；其余
     * 变体（AtlasSprite / PlayerSprite）在该 codec 上无法编码。未设置时返回 nullopt，
     * 原版此时 `getFont()` 回落到 `minecraft:default`。
     */
    [[nodiscard]] const std::optional<std::string>& getFont() const noexcept
    {
        return m_font;
    }

    /**
     * @brief 设置字体，对应原版 `Style.withFont`
     */
    void setFont(std::optional<std::string> font) noexcept
    {
        m_font = std::move(font);
    }

    // ========== 父样式继承 ==========

    /**
     * @brief 从父样式继承未设置的属性
     * @param parent 父样式
     * @return 合并后的新样式
     */
    [[nodiscard]] Style mergeWithParent(const Style& parent) const noexcept;

    // ========== 格式化套用 ==========

    /**
     * @brief 套用一个格式码，对应原版 `Style.applyFormat`
     *
     * 样式码（加粗、斜体、删除线、下划线、混淆）只把对应标志置 true，**不动其他标志**；
     * 颜色码只替换颜色；`Reset` 返回空样式。
     *
     * 注意这里与 `applyLegacyFormat` 的唯一区别：**处理颜色时不清除已有的样式标志**。
     */
    [[nodiscard]] Style applyFormat(TextFormatting formatting) const;

    /**
     * @brief 按旧版 § 码语义套用格式，对应原版 `Style.applyLegacyFormat`
     *
     * 与 `applyFormat` 几乎相同，唯一区别在**颜色码分支**：这里会把五个样式标志
     * **全部置 false**，因为 § 语义下换颜色等同于开启一个新段落，旧的加粗/斜体等
     * 一并作废。样式码分支的行为两者完全一致（只置 true，不动其他）。
     */
    [[nodiscard]] Style applyLegacyFormat(TextFormatting formatting) const;

    /**
     * @brief 以本样式为**父**、把它的未设置项填到 `other` 上，对应原版 `Style.applyTo`
     *
     * 语义是「本样式优先，`other` 兜底」：本样式显式设置过的项保持不变，未设置的项
     * 取 `other` 的值。
     *
     * 两处短路：本样式为空时直接返回 `other`；`other` 为空时直接返回本样式。
     */
    [[nodiscard]] Style applyTo(const Style& other) const;

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
    std::optional<TextColor> m_color;
    std::optional<bool> m_bold;
    std::optional<bool> m_italic;
    std::optional<bool> m_underlined;
    std::optional<bool> m_strikethrough;
    std::optional<bool> m_obfuscated;
    std::optional<ClickEvent> m_clickEvent;
    std::optional<HoverEvent> m_hoverEvent;
    std::optional<std::string> m_insertion;
    std::optional<i32> m_shadowColor;
    std::optional<std::string> m_font;
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
