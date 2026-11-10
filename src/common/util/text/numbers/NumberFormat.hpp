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
#include "common/util/text/ITextComponentFwd.hpp"
#include "common/util/text/TextStyle.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace mc::text::numbers {

/**
 * @brief 数字格式类型标识
 *
 * 对应原版 `NumberFormatType` 在 `Registries.NUMBER_FORMAT_TYPE` 注册表中的条目。**枚举值就是
 * 网络上的注册表 id**（原版 `ByteBufCodecs.registry(...).dispatch(...)` 写出的 VarInt 就是它），
 * 因此顺序不可随意调整：
 *
 * | 枚举值 | 注册名 | 线格式载荷 |
 * |---|---|---|
 * | 0 | `minecraft:blank` | 无 |
 * | 1 | `minecraft:styled` | Style 的 NBT 根标签 |
 * | 2 | `minecraft:fixed` | Component 的 NBT 根标签 |
 */
enum class NumberFormatType : u8 {
    Blank = 0,  ///< 不显示数字（渲染成空组件）
    Styled = 1, ///< 数字本身套一层样式
    Fixed = 2,  ///< 所有数字统一显示成同一个固定组件
};

/// 类型数量（供断言与遍历使用）
inline constexpr usize NUMBER_FORMAT_TYPE_COUNT = 3;

/**
 * @brief 取类型的注册名（`minecraft:blank` / `minecraft:styled` / `minecraft:fixed`）
 */
[[nodiscard]] std::string_view numberFormatTypeName(NumberFormatType type) noexcept;

/**
 * @brief 由注册名解析类型
 *
 * 接受带命名空间的完整写法（`minecraft:styled`）与省略命名空间的简写（`styled`），
 * 与原版 `ResourceLocation` 的解析规则一致。
 *
 * @param name 注册名
 * @return 解析出的类型，无法识别时返回 nullopt
 */
[[nodiscard]] std::optional<NumberFormatType> numberFormatTypeFromName(std::string_view name) noexcept;

/**
 * @brief 数字格式
 *
 * 原版是 `NumberFormat` 接口 + `BlankFormat` / `StyledFormat` / `FixedFormat` 三个 record。
 * 这里收成一个值类型：三种变体各自只用一个字段，拆成类层次会让「记分板目标、分数条目、
 * 存档数据」三处都要持有指针并处理拷贝语义，得不偿失。
 *
 * 三种变体的语义（原版 `NumberFormat#format(int)`）：
 * - `Blank`：返回空组件，数字完全不显示。
 * - `Styled`：返回 `Component.literal(Integer.toString(value))` 并套上样式。
 * - `Fixed`：返回固定组件的副本，数字内容不影响输出。
 *
 * 拷贝是深拷贝（`Fixed` 的组件会被复制），因此可以像普通值一样存进 `std::optional`。
 */
class NumberFormat {
public:
    /**
     * @brief 构造 blank 格式（不显示数字）
     */
    NumberFormat();

    // 深拷贝：Fixed 变体持有组件所有权，必须复制其内容
    NumberFormat(const NumberFormat& other);
    NumberFormat& operator=(const NumberFormat& other);
    NumberFormat(NumberFormat&& other) noexcept;
    NumberFormat& operator=(NumberFormat&& other) noexcept;
    ~NumberFormat();

    /**
     * @brief 构造 styled 格式（数字套用给定样式）
     */
    [[nodiscard]] static NumberFormat styled(Style style);

    /**
     * @brief 构造 fixed 格式（所有数字显示成同一个组件）
     *
     * @param value 固定组件；传入 nullptr 时退化为空组件（等价 blank 的输出，但类型仍是 fixed）
     */
    [[nodiscard]] static NumberFormat fixed(std::unique_ptr<ITextComponent> value);

    /**
     * @brief 类型标识
     */
    [[nodiscard]] NumberFormatType type() const noexcept
    {
        return m_type;
    }

    /**
     * @brief styled 变体的样式；其它变体返回 nullptr
     */
    [[nodiscard]] const Style* style() const noexcept;

    /**
     * @brief fixed 变体的固定组件；其它变体返回 nullptr
     */
    [[nodiscard]] const ITextComponent* value() const noexcept;

    /**
     * @brief 把一个分数渲染成组件
     *
     * 返回的组件归调用方所有；`Fixed` 变体返回的是副本，调用方可以随意修改。
     *
     * @param value 分数值
     * @return 渲染结果组件
     */
    [[nodiscard]] std::unique_ptr<ITextComponent> format(i32 value) const;

    bool operator==(const NumberFormat& other) const noexcept;
    bool operator!=(const NumberFormat& other) const noexcept
    {
        return !(*this == other);
    }

private:
    NumberFormatType m_type = NumberFormatType::Blank;
    /// 仅 Styled 有效
    Style m_style;
    /// 仅 Fixed 有效
    std::unique_ptr<ITextComponent> m_value;
};

} // namespace mc::text::numbers