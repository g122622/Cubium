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

#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/LerpFunction.hpp"

#include <map>
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 属性类型
 *
 * MC 1.21.11 record AttributeType<Value>。
 * 定义属性的关键帧插值方式、状态变化/空间/部分 tick 插值方式，以及该类型允许的修改器库。
 *
 * 修饰符库以操作标识为键、指向全局修改器单例的裸指针为值：修改器均为无状态对象，
 * 库不承担所有权（与 MC 中修改器是枚举/常量单例同理）。
 *
 * 条目统一以 `AttributeModifier<Value, Value>` 指针保存：参数类型同为 Value 的修改器
 * 直接复用其单例指针；参数类型不同（如布尔/浮点/颜色修改器的参数就是值本身）时，
 * 由 `TypedModifierAdapter` 以 Arg == Value 的身份转发给底层修改器。
 */
template <typename Value>
class AttributeType {
public:
    using ModifierLibrary = std::map<AttributeModifierOperation, const AttributeModifier<Value, Value>*>;

    AttributeType(LerpFunction<Value> keyframeLerp,
        LerpFunction<Value> stateChangeLerp,
        LerpFunction<Value> spatialLerp,
        LerpFunction<Value> partialTickLerp,
        ModifierLibrary modifierLibrary)
        : m_keyframeLerp(std::move(keyframeLerp))
        , m_stateChangeLerp(std::move(stateChangeLerp))
        , m_spatialLerp(std::move(spatialLerp))
        , m_partialTickLerp(std::move(partialTickLerp))
        , m_modifierLibrary(std::move(modifierLibrary))
    {}

    const LerpFunction<Value>& keyframeLerp() const noexcept
    {
        return m_keyframeLerp;
    }

    const LerpFunction<Value>& stateChangeLerp() const noexcept
    {
        return m_stateChangeLerp;
    }

    const LerpFunction<Value>& spatialLerp() const noexcept
    {
        return m_spatialLerp;
    }

    const LerpFunction<Value>& partialTickLerp() const noexcept
    {
        return m_partialTickLerp;
    }

    const ModifierLibrary& modifierLibrary() const noexcept
    {
        return m_modifierLibrary;
    }

    /**
     * @brief 修饰符是否被本类型允许
     *
     * 覆写修改器对所有类型都合法，其余修改器必须出现在修饰符库中。
     */
    [[nodiscard]] bool isAllowedModifier(AttributeModifierOperation operation) const noexcept
    {
        return operation == AttributeModifierOperation::Override || m_modifierLibrary.contains(operation);
    }

    /**
     * @brief 创建可插值的属性类型
     *
     * 关键帧/状态变化/空间三种插值共用同一函数，partTick 插值单独给出。
     */
    static AttributeType<Value> ofInterpolated(
        LerpFunction<Value> lerp, LerpFunction<Value> partialTickLerp, ModifierLibrary modifierLibrary)
    {
        return AttributeType<Value>(lerp, lerp, lerp, std::move(partialTickLerp), std::move(modifierLibrary));
    }

    /**
     * @brief 创建不可插值（阶跃）的属性类型
     *
     * keyframeLerp = ofStep(1.0)：progress >= 1.0 时切换到 to
     */
    static AttributeType<Value> ofNotInterpolated()
    {
        return AttributeType<Value>(LerpFunction<Value>::ofStep(1.0f), // keyframeLerp
            LerpFunction<Value>::ofStep(0.0f),                         // stateChangeLerp
            LerpFunction<Value>::ofStep(0.5f),                         // spatialLerp
            LerpFunction<Value>::ofStep(0.0f),                         // partialTickLerp
            ModifierLibrary{});
    }

    /**
     * @brief 创建带修饰符库的不可插值（阶跃）属性类型
     */
    static AttributeType<Value> ofNotInterpolated(ModifierLibrary modifierLibrary)
    {
        return AttributeType<Value>(LerpFunction<Value>::ofStep(1.0f),
            LerpFunction<Value>::ofStep(0.0f),
            LerpFunction<Value>::ofStep(0.5f),
            LerpFunction<Value>::ofStep(0.0f),
            std::move(modifierLibrary));
    }

private:
    LerpFunction<Value> m_keyframeLerp;
    LerpFunction<Value> m_stateChangeLerp;
    LerpFunction<Value> m_spatialLerp;
    LerpFunction<Value> m_partialTickLerp;
    ModifierLibrary m_modifierLibrary;
};

} // namespace attribute
} // namespace world
} // namespace mc
