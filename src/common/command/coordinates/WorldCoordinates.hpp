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
#include "WorldCoordinate.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::command {

/**
 * @brief 世界坐标容器
 *
 * 由三个 WorldCoordinate 分量组成，支持绝对坐标和相对坐标（~ 前缀）。
 * getPosition() 将相对分量与命令源位置相加，绝对分量直接使用。
 *
 * 对应 MC Java 的 WorldCoordinates。
 *
 * 示例:
 * - `100 64 -200` → WorldCoordinates(abs(100), abs(64), abs(-200))
 * - `~ ~ ~`       → WorldCoordinates(rel(0), rel(0), rel(0))
 * - `~5 ~-3 ~10`  → WorldCoordinates(rel(5), rel(-3), rel(10))
 * - `100 ~5 -200` → WorldCoordinates(abs(100), rel(5), abs(-200))
 */
class WorldCoordinates : public Coordinates {
public:
    WorldCoordinates() = default;

    /**
     * @brief 构造世界坐标
     * @param x X 分量
     * @param y Y 分量
     * @param z Z 分量
     */
    WorldCoordinates(WorldCoordinate x, WorldCoordinate y, WorldCoordinate z)
        : m_x(x)
        , m_y(y)
        , m_z(z)
    {}

    /**
     * @brief 从三个双精度值构造绝对坐标
     */
    static WorldCoordinates absolute(f64 x, f64 y, f64 z)
    {
        return WorldCoordinates(WorldCoordinate(false, x), WorldCoordinate(false, y), WorldCoordinate(false, z));
    }

    /**
     * @brief 从三个双精度值构造相对坐标
     */
    static WorldCoordinates relative(f64 x, f64 y, f64 z)
    {
        return WorldCoordinates(WorldCoordinate(true, x), WorldCoordinate(true, y), WorldCoordinate(true, z));
    }

    // ========== Coordinates 接口实现 ==========

    [[nodiscard]] Vector3d getPosition(const Vector3d& anchorPosition, const Vector2f& rotation) const override
    {
        // 世界坐标：相对分量与锚点位置相加，绝对分量直接使用
        return Vector3d(m_x.get(anchorPosition.x), m_y.get(anchorPosition.y), m_z.get(anchorPosition.z));
    }

    [[nodiscard]] Vector2f getRotation(const Vector2f& rotation) const override
    {
        // 对于旋转参数，x 分量对应 pitch，y 分量对应 yaw
        return Vector2f(static_cast<f32>(m_x.get(static_cast<f64>(rotation.x))),
            static_cast<f32>(m_y.get(static_cast<f64>(rotation.y))));
    }

    [[nodiscard]] bool isXRelative() const override { return m_x.isRelative(); }
    [[nodiscard]] bool isYRelative() const override { return m_y.isRelative(); }
    [[nodiscard]] bool isZRelative() const override { return m_z.isRelative(); }

    // ========== 分量访问 ==========

    [[nodiscard]] const WorldCoordinate& x() const noexcept { return m_x; }
    [[nodiscard]] const WorldCoordinate& y() const noexcept { return m_y; }
    [[nodiscard]] const WorldCoordinate& z() const noexcept { return m_z; }

private:
    WorldCoordinate m_x;
    WorldCoordinate m_y;
    WorldCoordinate m_z;
};

} // namespace mc::command
