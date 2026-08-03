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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>

namespace mc::command {

/**
 * @brief 局部坐标容器（^ ^ ^ 格式）
 *
 * 基于执行者视线方向（yaw/pitch）的局部坐标系。
 * 三个分量分别表示左、上、前方向的偏移量。
 *
 * 对应 MC Java 的 LocalCoordinates。
 *
 * 局部坐标到世界坐标的转换公式：
 * 1. 从 yaw/pitch 计算三个基向量 (forward, up, left)
 * 2. 将局部偏移量作为权重组合三个基向量
 * 3. 加上锚点位置（feet/eyes）
 *
 * 示例:
 * - `^ ^ ^5`  → 前方5格
 * - `^5 ^ ^`  → 左方5格
 * - `^ ^5 ^`  → 上方5格
 * - `^1 ^2 ^3` → 左方1格 + 上方2格 + 前方3格
 */
class LocalCoordinates : public Coordinates {
public:
    LocalCoordinates() = default;

    /**
     * @brief 构造局部坐标
     * @param left 左方向偏移（^左）
     * @param up 上方向偏移（^上）
     * @param forwards 前方向偏移（^前）
     */
    LocalCoordinates(f64 left, f64 up, f64 forwards)
        : m_left(left)
        , m_up(up)
        , m_forwards(forwards)
    {}

    // ========== Coordinates 接口实现 ==========

    [[nodiscard]] Vector3d getPosition(const Vector3d& anchorPosition, const Vector2f& rotation) const override
    {
        // 从 yaw/pitch 计算世界偏移
        // rotation.x = pitch, rotation.y = yaw (与 MC Java Vec2 约定一致)
        Vector3d offset = _applyLocalCoordinatesToRotation(rotation, Vector3d(m_left, m_up, m_forwards));

        // 锚点位置 + 偏移
        return Vector3d(anchorPosition.x + offset.x, anchorPosition.y + offset.y, anchorPosition.z + offset.z);
    }

    [[nodiscard]] Vector2f getRotation(const Vector2f& rotation) const override
    {
        // 局部坐标不影响旋转
        return Vector2f(0.0f, 0.0f);
    }

    [[nodiscard]] bool isXRelative() const override { return true; }
    [[nodiscard]] bool isYRelative() const override { return true; }
    [[nodiscard]] bool isZRelative() const override { return true; }

    // ========== 分量访问 ==========

    [[nodiscard]] f64 left() const noexcept { return m_left; }
    [[nodiscard]] f64 up() const noexcept { return m_up; }
    [[nodiscard]] f64 forwards() const noexcept { return m_forwards; }

    /**
     * @brief 局部坐标前缀字符
     */
    static constexpr char PREFIX = '^';

private:
    f64 m_left = 0.0;
    f64 m_up = 0.0;
    f64 m_forwards = 0.0;

    /**
     * @brief 将局部坐标向量旋转到世界坐标系
     *
     * 对应 MC Java 的 Vec3.applyLocalCoordinatesToRotation(Vec2 rotation, Vec3 local)
     *
     * @param rotation 旋转角 (pitch=x, yaw=y)，度数
     * @param local 局部坐标偏移 (left, up, forwards)
     * @return 世界坐标系下的偏移向量
     */
    [[nodiscard]] static Vector3d _applyLocalCoordinatesToRotation(const Vector2f& rotation, const Vector3d& local)
    {
        // MC Java 中 Vec2 的 x = pitch (XRot), y = yaw (YRot)
        f32 yaw = rotation.y;
        f32 pitch = rotation.x;

        // 将度数转换为弧度
        f32 yawRad = (yaw + 90.0f) * math::DEG_TO_RAD;
        f32 pitchRad = -pitch * math::DEG_TO_RAD;
        f32 pitchPlus90Rad = (-pitch + 90.0f) * math::DEG_TO_RAD;

        f32 cosYaw = std::cos(yawRad);
        f32 sinYaw = std::sin(yawRad);
        f32 cosPitch = std::cos(pitchRad);
        f32 sinPitch = std::sin(pitchRad);
        f32 cosPitchPlus90 = std::cos(pitchPlus90Rad);
        f32 sinPitchPlus90 = std::sin(pitchPlus90Rad);

        // Forward 向量 — 实体面朝方向
        f64 forwardX = static_cast<f64>(cosYaw * cosPitch);
        f64 forwardY = static_cast<f64>(sinPitch);
        f64 forwardZ = static_cast<f64>(sinYaw * cosPitch);

        // Up 向量 — 实体头顶方向
        f64 upX = static_cast<f64>(cosYaw * cosPitchPlus90);
        f64 upY = static_cast<f64>(sinPitchPlus90);
        f64 upZ = static_cast<f64>(sinYaw * cosPitchPlus90);

        // Left 向量 = -1 * (forward × up)
        f64 leftX = -(forwardY * upZ - forwardZ * upY);
        f64 leftY = -(forwardZ * upX - forwardX * upZ);
        f64 leftZ = -(forwardX * upY - forwardY * upX);

        // 组合：offset = forward * forwards + up * up + left * left
        f64 offsetX = forwardX * local.z + upX * local.y + leftX * local.x;
        f64 offsetY = forwardY * local.z + upY * local.y + leftY * local.x;
        f64 offsetZ = forwardZ * local.z + upZ * local.y + leftZ * local.x;

        return Vector3d(offsetX, offsetY, offsetZ);
    }
};

} // namespace mc::command
