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

#include "../../../core/Types.hpp"
#include "../Vector3.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc {

/**
 * @brief 射线数据结构
 *
 * 用于射线检测，表示从origin出发沿direction方向的射线。
 * direction应该是归一化的单位向量。
 *
 * 参考MC的RayTraceContext
 */
class Ray {
public:
    Vector3f origin;
    Vector3f direction;

    Ray() = default;

    /**
     * @brief 构造射线
     * @param origin 起点
     * @param direction 方向（应该归一化）
     */
    Ray(const Vector3f& origin, const Vector3f& direction)
        : origin(origin)
        , direction(direction)
    {}

    /**
     * @brief 获取射线上距离t处的点
     * @param t 距离参数
     * @return 射线上的点 origin + direction * t
     */
    [[nodiscard]] Vector3f at(f32 t) const noexcept { return origin + direction * t; }

    /**
     * @brief 从pitch和yaw角度创建射线（角度制）
     *
     * 参考MC的Entity.getVectorForRotation()
     * MC中: pitch正为下，yaw正为左（从上往下看顺时针）
     *
     * @param origin 起点
     * @param pitchDeg 俯仰角（度），正为下看，负为上看
     * @param yawDeg 偏航角（度），正为左转
     * @return 射线对象
     */
    [[nodiscard]] static Ray fromAngles(const Vector3f& origin, f32 pitchDeg, f32 yawDeg) noexcept
    {
        // 转换为弧度
        const f32 pitchRad = math::toRadians(pitchDeg);
        const f32 yawRad = math::toRadians(yawDeg);

        // 使用Vector3f::fromAngles
        // 注意：MC的yaw是从南开始顺时针，我们需要调整
        // MC中 yaw=0 看向南方(正Z)，yaw=90 看向西方(负X)
        // Vector3f::fromAngles: yaw=0 看向X正方向
        // 所以我们需要将MC的yaw转换为数学角度
        const Vector3f dir = Vector3f::fromAngles(-pitchRad, -yawRad + math::PI * 0.5f);
        return Ray(origin, dir.normalized());
    }
};

} // namespace mc
