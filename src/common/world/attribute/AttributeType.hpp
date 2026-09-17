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

#include "common/world/attribute/LerpFunction.hpp"

#include <functional>
#include <string>

namespace mc {
namespace world {
namespace attribute {

class AttributeModifierBase;

/**
 * @brief 属性类型
 *
 * MC 1.21.11 record AttributeType<Value>。
 * 定义属性的关键帧插值方式、修改器库等。
 *
 * Cubium 省略了 Codec 字段（MC 用于数据包序列化），
 * 保留 keyframeLerp（关键帧插值函数）与 modifierLibrary（修改器库）。
 */
template <typename Value>
class AttributeType {
public:
    AttributeType(LerpFunction<Value> keyframeLerp,
        LerpFunction<Value> stateChangeLerp,
        LerpFunction<Value> spatialLerp,
        LerpFunction<Value> partialTickLerp)
        : m_keyframeLerp(std::move(keyframeLerp))
        , m_stateChangeLerp(std::move(stateChangeLerp))
        , m_spatialLerp(std::move(spatialLerp))
        , m_partialTickLerp(std::move(partialTickLerp))
    {}

    const LerpFunction<Value>& keyframeLerp() const noexcept { return m_keyframeLerp; }
    const LerpFunction<Value>& stateChangeLerp() const noexcept { return m_stateChangeLerp; }
    const LerpFunction<Value>& spatialLerp() const noexcept { return m_spatialLerp; }
    const LerpFunction<Value>& partialTickLerp() const noexcept { return m_partialTickLerp; }

    /**
     * @brief 创建可插值的属性类型
     *
     * 所有的 lerp 函数都使用同一个 lerpfunction。
     */
    static AttributeType<Value> ofInterpolated(LerpFunction<Value> lerp)
    {
        return AttributeType<Value>(lerp, lerp, lerp, lerp);
    }

    /**
     * @brief 创建不可插值（阶跃）的属性类型
     *
     * keyframeLerp = ofStep(1.0)：progress >= 1.0 时切换到 to
     */
    static AttributeType<Value> ofNotInterpolated()
    {
        return AttributeType<Value>(
            LerpFunction<Value>::ofStep(1.0f),  // keyframeLerp
            LerpFunction<Value>::ofStep(0.0f),  // stateChangeLerp
            LerpFunction<Value>::ofStep(0.5f),  // spatialLerp
            LerpFunction<Value>::ofStep(0.0f)); // partialTickLerp
    }

    // TODO: modifierLibrary / modifierCodec 暂未实现，按需补充

private:
    LerpFunction<Value> m_keyframeLerp;
    LerpFunction<Value> m_stateChangeLerp;
    LerpFunction<Value> m_spatialLerp;
    LerpFunction<Value> m_partialTickLerp;
};

} // namespace attribute
} // namespace world
} // namespace mc
