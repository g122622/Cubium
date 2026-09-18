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

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/world/attribute/AttributeType.hpp"

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 预定义属性类型
 *
 * MC 1.21.11 class AttributeTypes。
 * Cubium 目前仅实现活动类型，其余类型（FLOAT / ANGLE_DEGREES / RGB_COLOR /
 * MOON_PHASE / BED_RULE / PARTICLE 等）按需补充。
 */
class AttributeTypes {
public:
    /**
     * @brief 活动类型
     *
     * 不可插值：keyframeLerp = ofStep(1.0)，采样进度到达或越过 toTicks 时
     * 返回下一关键帧的值，否则保持当前关键帧的值。
     */
    [[nodiscard]] static const AttributeType<entity::ai::brain::schedule::Activity>& ACTIVITY();
};

} // namespace attribute
} // namespace world
} // namespace mc
