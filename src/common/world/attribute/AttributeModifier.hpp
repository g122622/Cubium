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
#include "common/world/attribute/LerpFunction.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 属性修改器操作标识
 *
 * MC 1.21.11 enum AttributeModifier.OperationId。
 * 标识修改器在属性类型修饰符库中的名字，同时也是数据包 JSON 中 `modifier` 字段的取值。
 */
enum class AttributeModifierOperation : u8 {
    Override,
    AlphaBlend,
    Add,
    Subtract,
    Multiply,
    BlendToGray,
    Minimum,
    Maximum,
    And,
    Nand,
    Or,
    Nor,
    Xor,
    Xnor,
};

/**
 * @brief 取操作标识对应的序列化名
 */
[[nodiscard]] constexpr std::string_view attributeModifierOperationName(AttributeModifierOperation operation) noexcept
{
    switch (operation) {
        case AttributeModifierOperation::Override:
            return "override";
        case AttributeModifierOperation::AlphaBlend:
            return "alpha_blend";
        case AttributeModifierOperation::Add:
            return "add";
        case AttributeModifierOperation::Subtract:
            return "subtract";
        case AttributeModifierOperation::Multiply:
            return "multiply";
        case AttributeModifierOperation::BlendToGray:
            return "blend_to_gray";
        case AttributeModifierOperation::Minimum:
            return "minimum";
        case AttributeModifierOperation::Maximum:
            return "maximum";
        case AttributeModifierOperation::And:
            return "and";
        case AttributeModifierOperation::Nand:
            return "nand";
        case AttributeModifierOperation::Or:
            return "or";
        case AttributeModifierOperation::Nor:
            return "nor";
        case AttributeModifierOperation::Xor:
            return "xor";
        case AttributeModifierOperation::Xnor:
            return "xnor";
    }
    return "override";
}

/**
 * @brief 把无状态修改器单例包装成不拥有所有权的 shared_ptr
 *
 * 时间线轨道以 shared_ptr 持有修改器（Timeline 需要可拷贝存储），
 * 而布尔/浮点/颜色修改器是全局常量单例，不应被释放。
 *
 * @param modifier 全局修改器单例地址
 * @return 不释放所指对象的共享指针
 */
template <typename Modifier>
[[nodiscard]] std::shared_ptr<const Modifier> nonOwningSharedPtr(const Modifier* modifier)
{
    return std::shared_ptr<const Modifier>(modifier, [](const Modifier*) {});
}

/**
 * @brief 属性修改器
 *
 * MC 1.21.11 interface AttributeModifier<Subject, Argument>。
 * 定义如何将关键帧采样得到的 argument 应用到基础值 subject 上。
 */
template <typename Subject, typename Argument>
class AttributeModifier {
public:
    virtual ~AttributeModifier() = default;

    /**
     * @brief 将 argument 应用到 subject
     *
     * @param subject 基础值
     * @param argument 关键帧采样得到的参数
     * @return 应用后的值
     */
    [[nodiscard]] virtual Argument apply(Subject subject, Argument argument) const = 0;

    /**
     * @brief 操作标识
     *
     * 用于在属性类型的修饰符库中定位本修改器，以及数据包序列化。
     */
    [[nodiscard]] virtual AttributeModifierOperation operation() const noexcept = 0;

    /**
     * @brief 关键帧插值函数（作用于 Argument）
     *
     * 不同修改器的参数语义不同（如布尔取常量、浮点取线性），
     * 因此插值函数由修改器而非属性类型决定。
     */
    [[nodiscard]] virtual LerpFunction<Argument> argumentKeyframeLerp() const = 0;
};

/**
 * @brief 修改器适配器：以 Arg == Subject 的身份暴露底层修改器
 *
 * 环境属性在时间线上被驱动时，属性层只接受 `AttributeModifier<Value, Value>`
 * （关键帧轨道采样出的参数经转换后成为属性值的一部分）。本适配器在类型层面桥接参数类型：
 * - applyValue：值直接作为参数传入底层修改器；
 * - applyArgumentValue：把轨道采样的值先经 convert 转换成参数，再执行底层修改器。
 *
 * 适配器持有原生数字类型（bool / f32 / i32）或纯函数状态，
 * 与其绑定的属性层具有相同的生命周期，因此按值自持是安全的。
 */
template <typename Value, typename Argument>
class TypedModifierAdapter final : public AttributeModifier<Value, Value> {
public:
    /// 值 → 参数的转换
    using ConvertFn = std::function<Argument(Value)>;

    TypedModifierAdapter(const AttributeModifier<Value, Argument>& inner, ConvertFn convert)
        : m_inner(inner)
        , m_convert(std::move(convert))
    {}

    /// 直接把值当作参数应用底层修改器
    [[nodiscard]] Value applyValue(Value subject) const
    {
        const Argument argument = m_convert(subject);
        return m_inner.apply(std::move(subject), argument);
    }

    /// 把轨道采样值转换为参数后应用底层修改器
    [[nodiscard]] Value applyArgumentValue(Value subject, Value argument) const
    {
        return m_inner.apply(std::move(subject), m_convert(std::move(argument)));
    }

    [[nodiscard]] Value apply(Value subject, Value argument) const override
    {
        return applyArgumentValue(std::move(subject), std::move(argument));
    }

    /// 底层修改器（供调试与诊断）
    [[nodiscard]] const AttributeModifier<Value, Argument>& inner() const noexcept
    {
        return m_inner;
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return m_inner.operation();
    }

    [[nodiscard]] LerpFunction<Value> argumentKeyframeLerp() const override
    {
        return m_inner.argumentKeyframeLerp().template map<Value>(m_convert);
    }

private:
    /// 底层修改器：无状态单例或属性层自持的修改器，不承担所有权
    const AttributeModifier<Value, Argument>& m_inner;
    ConvertFn m_convert;
};

/**
 * @brief 覆写修改器
 *
 * MC 1.21.11 record AttributeModifier.Override<Value>。
 * 直接返回 argument，忽略 subject。
 */
template <typename Value>
class OverrideModifier final : public AttributeModifier<Value, Value> {
public:
    [[nodiscard]] Value apply(Value, Value argument) const override
    {
        return std::move(argument);
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return AttributeModifierOperation::Override;
    }

    [[nodiscard]] LerpFunction<Value> argumentKeyframeLerp() const override
    {
        // 覆写修改器的参数插值直接沿用属性类型自身的关键帧插值
        return m_keyframeLerp;
    }

    /**
     * @brief 绑定属性类型的关键帧插值
     *
     * vanilla 中 OverrideModifier.argumentKeyframeLerp 读取 `attribute.type().keyframeLerp()`，
     * 而修改器本身是无状态单例；C++ 侧以可拷贝的绑定对象承载该引用关系。
     */
    [[nodiscard]] OverrideModifier withKeyframeLerp(LerpFunction<Value> keyframeLerp) const
    {
        OverrideModifier copy;
        copy.m_keyframeLerp = std::move(keyframeLerp);
        return copy;
    }

private:
    LerpFunction<Value> m_keyframeLerp = LerpFunction<Value>::ofStep(1.0f);
};

// ============================================================================
// 布尔修改器（MC BooleanModifier）
// ============================================================================

class BooleanModifier final : public AttributeModifier<bool, bool> {
public:
    constexpr BooleanModifier(AttributeModifierOperation operation, bool (*fn)(bool, bool)) noexcept
        : m_operation(operation)
        , m_fn(fn)
    {}

    /// 全部逻辑运算单例（AND / NAND / OR / NOR / XOR / XNOR）
    static const BooleanModifier AND;
    static const BooleanModifier NAND;
    static const BooleanModifier OR;
    static const BooleanModifier NOR;
    static const BooleanModifier XOR;
    static const BooleanModifier XNOR;

    [[nodiscard]] bool apply(bool subject, bool argument) const override
    {
        return m_fn(subject, argument);
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return m_operation;
    }

    [[nodiscard]] LerpFunction<bool> argumentKeyframeLerp() const override
    {
        return LerpFunction<bool>::ofConstant<bool>();
    }

private:
    AttributeModifierOperation m_operation;
    bool (*m_fn)(bool argument, bool subject);
};

// ============================================================================
// 浮点修改器（MC FloatModifier.Simple）
// ============================================================================

class FloatModifier final : public AttributeModifier<f32, f32> {
public:
    constexpr FloatModifier(AttributeModifierOperation operation, f32 (*fn)(f32, f32)) noexcept
        : m_operation(operation)
        , m_fn(fn)
    {}

    /// 全部单值浮点运算单例（ADD / SUBTRACT / MULTIPLY / MINIMUM / MAXIMUM）
    static const FloatModifier ADD;
    static const FloatModifier SUBTRACT;
    static const FloatModifier MULTIPLY;
    static const FloatModifier MINIMUM;
    static const FloatModifier MAXIMUM;

    [[nodiscard]] f32 apply(f32 subject, f32 argument) const override
    {
        return m_fn(subject, argument);
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return m_operation;
    }

    [[nodiscard]] LerpFunction<f32> argumentKeyframeLerp() const override
    {
        return LerpFunction<f32>::ofFloat();
    }

private:
    AttributeModifierOperation m_operation;
    f32 (*m_fn)(f32 subject, f32 argument);
};

// ============================================================================
// 颜色修改器（MC ColorModifier，ARGB 32 位）
// ============================================================================

/**
 * @brief 取红色通道
 */
[[nodiscard]] constexpr i32 argbRed(i32 color) noexcept
{
    return (color >> 16) & 0xFF;
}

/**
 * @brief 取绿色通道
 */
[[nodiscard]] constexpr i32 argbGreen(i32 color) noexcept
{
    return (color >> 8) & 0xFF;
}

/**
 * @brief 取蓝色通道
 */
[[nodiscard]] constexpr i32 argbBlue(i32 color) noexcept
{
    return color & 0xFF;
}

/**
 * @brief 取透明度通道
 */
[[nodiscard]] constexpr i32 argbAlpha(i32 color) noexcept
{
    return static_cast<i32>(static_cast<u32>(color) >> 24);
}

/**
 * @brief 由通道值组装 ARGB 颜色
 */
[[nodiscard]] constexpr i32 argbColor(i32 alpha, i32 red, i32 green, i32 blue) noexcept
{
    return ((alpha & 0xFF) << 24) | ((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF);
}

/**
 * @brief 按通道相乘（四个通道一并相乘，结果按 255 取整）
 */
[[nodiscard]] constexpr i32 argbMultiply(i32 left, i32 right) noexcept
{
    if (left == -1) {
        return right;
    }
    if (right == -1) {
        return left;
    }

    const auto scale = [](i32 a, i32 b) {
        return a * b / 255;
    };
    return argbColor(scale(argbAlpha(left), argbAlpha(right)),
        scale(argbRed(left), argbRed(right)),
        scale(argbGreen(left), argbGreen(right)),
        scale(argbBlue(left), argbBlue(right)));
}

/**
 * @brief 按通道相加（保留左侧透明度，各通道饱和到 255）
 */
[[nodiscard]] constexpr i32 argbAddRgb(i32 left, i32 right) noexcept
{
    return argbColor(argbAlpha(left),
        std::min(argbRed(left) + argbRed(right), 255),
        std::min(argbGreen(left) + argbGreen(right), 255),
        std::min(argbBlue(left) + argbBlue(right), 255));
}

/**
 * @brief 按通道相减（保留左侧透明度，各通道饱和到 0）
 */
[[nodiscard]] constexpr i32 argbSubtractRgb(i32 left, i32 right) noexcept
{
    return argbColor(argbAlpha(left),
        std::max(argbRed(left) - argbRed(right), 0),
        std::max(argbGreen(left) - argbGreen(right), 0),
        std::max(argbBlue(left) - argbBlue(right), 0));
}

/**
 * @brief 把左色按右色的透明度混合到右色上
 *
 * 与 ARGB.alphaBlend 一致：右色不透明时直接返回右色，完全透明时返回左色，
 * 否则先求混合后的透明度 k = j + i * (255 - j) / 255，
 * 再按预乘 alpha 公式逐通道混合：`(b * j + a * (k - j)) / k`。
 */
[[nodiscard]] constexpr i32 argbAlphaBlend(i32 left, i32 right) noexcept
{
    const i32 leftAlpha = argbAlpha(left);
    const i32 rightAlpha = argbAlpha(right);
    if (rightAlpha == 255) {
        return right;
    }
    if (rightAlpha == 0) {
        return left;
    }

    const i32 blendedAlpha = rightAlpha + leftAlpha * (255 - rightAlpha) / 255;
    const auto mixChannel = [blendedAlpha, rightAlpha](i32 a, i32 b) {
        return (b * rightAlpha + a * (blendedAlpha - rightAlpha)) / blendedAlpha;
    };
    return argbColor(blendedAlpha,
        mixChannel(argbRed(left), argbRed(right)),
        mixChannel(argbGreen(left), argbGreen(right)),
        mixChannel(argbBlue(left), argbBlue(right)));
}

/**
 * @brief 颜色修改器（无参数型：ADD / SUBTRACT / MULTIPLY）
 */
class RgbColorModifier final : public AttributeModifier<i32, i32> {
public:
    constexpr RgbColorModifier(AttributeModifierOperation operation, i32 (*fn)(i32, i32)) noexcept
        : m_operation(operation)
        , m_fn(fn)
    {}

    /// 全部单值颜色运算单例（ADD / SUBTRACT / MULTIPLY / ALPHA_BLEND）
    static const RgbColorModifier ADD;
    static const RgbColorModifier SUBTRACT;
    static const RgbColorModifier MULTIPLY;
    static const RgbColorModifier ALPHA_BLEND;

    [[nodiscard]] i32 apply(i32 subject, i32 argument) const override
    {
        return m_fn(subject, argument);
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return m_operation;
    }

    [[nodiscard]] LerpFunction<i32> argumentKeyframeLerp() const override
    {
        return LerpFunction<i32>::ofColor();
    }

private:
    AttributeModifierOperation m_operation;
    i32 (*m_fn)(i32 subject, i32 argument);
};

/**
 * @brief 颜色修改器参数
 *
 * ALPHA_BLEND 使用 argb；BLEND_TO_GRAY 使用 blendToGray（参数按 16 位定点编码进 i32）。
 * 未使用的字段无意义。
 */
struct ColorModifierArgument {
    /// ALPHA_BLEND 的覆盖色（ARGB）
    i32 argb = 0;
    /// BLEND_TO_GRAY 的混合参数（打包后交给修改器）
    i32 blendToGray = 0;

    /// 以 ARGB 颜色作为参数
    static ColorModifierArgument ofArgb(i32 color)
    {
        return ColorModifierArgument{color, 0};
    }

    /// 以灰度混合参数作为参数（亮度与进度各占 16 位定点数）
    static ColorModifierArgument ofBlendToGray(f32 brightness, f32 factor)
    {
        const auto pack = [](f32 value) -> i32 {
            const f32 clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
            return static_cast<i32>(clamped * 65535.0f);
        };
        return ColorModifierArgument{0, (pack(brightness) << 16) | pack(factor)};
    }
};

/**
 * @brief 混合到灰度修改器（MC ColorModifier.BLEND_TO_GRAY）
 *
 * 先把 subject 转灰度并乘以 brightness，再以 factor 为进度与原色插值。
 */
class BlendToGrayModifier final : public AttributeModifier<i32, i32> {
public:
    [[nodiscard]] i32 apply(i32 subject, i32 argumentValue) const override
    {
        // 参数编码：高 16 位为亮度、低 16 位为混合进度的定点数 [0, 65535]
        const f32 brightness = static_cast<f32>((argumentValue >> 16) & 0xFFFF) / 65535.0f;
        const f32 factor = static_cast<f32>(argumentValue & 0xFFFF) / 65535.0f;
        const i32 luminance = static_cast<i32>(static_cast<f32>(argbRed(subject)) * 0.3f +
            static_cast<f32>(argbGreen(subject)) * 0.59f + static_cast<f32>(argbBlue(subject)) * 0.11f);
        const i32 grayChannel = std::clamp(static_cast<i32>(static_cast<f32>(luminance) * brightness), 0, 255);
        const i32 gray = argbColor(argbAlpha(subject), grayChannel, grayChannel, grayChannel);
        return LerpFunction<i32>::srgbLerp(factor, subject, gray);
    }

    [[nodiscard]] AttributeModifierOperation operation() const noexcept override
    {
        return AttributeModifierOperation::BlendToGray;
    }

    /// 参数按打包定点数线性插值（高低 16 位分别对应亮度与进度）
    [[nodiscard]] LerpFunction<i32> argumentKeyframeLerp() const override
    {
        return LerpFunction<i32>([](f32 progress, i32 from, i32 to) {
            const auto lerpPacked = [progress](i32 a, i32 b) {
                return static_cast<i32>(static_cast<f32>(a) + progress * static_cast<f32>(b - a));
            };
            return (lerpPacked((from >> 16) & 0xFFFF, (to >> 16) & 0xFFFF) << 16) |
                lerpPacked(from & 0xFFFF, to & 0xFFFF);
        });
    }
};

} // namespace attribute
} // namespace world
} // namespace mc

inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::AND{
    AttributeModifierOperation::And, [](bool argument, bool subject) {
        return argument && subject;
    }};
inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::NAND{
    AttributeModifierOperation::Nand, [](bool argument, bool subject) {
        return !argument || !subject;
    }};
inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::OR{
    AttributeModifierOperation::Or, [](bool argument, bool subject) {
        return argument || subject;
    }};
inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::NOR{
    AttributeModifierOperation::Nor, [](bool argument, bool subject) {
        return !argument && !subject;
    }};
inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::XOR{
    AttributeModifierOperation::Xor, [](bool argument, bool subject) {
        return argument != subject;
    }};
inline const mc::world::attribute::BooleanModifier mc::world::attribute::BooleanModifier::XNOR{
    AttributeModifierOperation::Xnor, [](bool argument, bool subject) {
        return argument == subject;
    }};

inline const mc::world::attribute::FloatModifier mc::world::attribute::FloatModifier::ADD{
    AttributeModifierOperation::Add, [](f32 subject, f32 argument) {
        return subject + argument;
    }};
inline const mc::world::attribute::FloatModifier mc::world::attribute::FloatModifier::SUBTRACT{
    AttributeModifierOperation::Subtract, [](f32 subject, f32 argument) {
        return subject - argument;
    }};
inline const mc::world::attribute::FloatModifier mc::world::attribute::FloatModifier::MULTIPLY{
    AttributeModifierOperation::Multiply, [](f32 subject, f32 argument) -> f32 {
        return subject * argument;
    }};
inline const mc::world::attribute::FloatModifier mc::world::attribute::FloatModifier::MINIMUM{
    AttributeModifierOperation::Minimum, [](f32 subject, f32 argument) {
        return argument < subject ? argument : subject;
    }};
inline const mc::world::attribute::FloatModifier mc::world::attribute::FloatModifier::MAXIMUM{
    AttributeModifierOperation::Maximum, [](f32 subject, f32 argument) {
        return argument > subject ? argument : subject;
    }};

inline const mc::world::attribute::RgbColorModifier mc::world::attribute::RgbColorModifier::ADD{
    AttributeModifierOperation::Add, [](i32 subject, i32 argument) {
        return argbAddRgb(subject, argument);
    }};
inline const mc::world::attribute::RgbColorModifier mc::world::attribute::RgbColorModifier::SUBTRACT{
    AttributeModifierOperation::Subtract, [](i32 subject, i32 argument) {
        return argbSubtractRgb(subject, argument);
    }};
inline const mc::world::attribute::RgbColorModifier mc::world::attribute::RgbColorModifier::MULTIPLY{
    AttributeModifierOperation::Multiply, [](i32 subject, i32 argument) {
        return argbMultiply(subject, argument);
    }};
inline const mc::world::attribute::RgbColorModifier mc::world::attribute::RgbColorModifier::ALPHA_BLEND{
    AttributeModifierOperation::AlphaBlend, [](i32 subject, i32 argument) {
        return argbAlphaBlend(subject, argument);
    }};
