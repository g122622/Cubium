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
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::AxisAlignedBB;
using mc::f32;
using mc::f64;
using mc::Vector3;
using mc::Vector3d;

/**
 * @brief 实体尺寸类
 *
 * 存储实体的宽度、高度和眼睛高度，用于计算碰撞箱与视线位置。
 * 固定尺寸(fixed=true)的实体不会根据姿态变化调整尺寸。
 */
class EntitySize {
public:
    /**
     * @brief 构造实体尺寸
     * @param width 宽度（方块单位）
     * @param height 高度（方块单位）
     * @param fixed 是否为固定尺寸
     */
    constexpr EntitySize(f32 width, f32 height, bool fixed = false)
        : EntitySize(width, height, _defaultEyeHeight(height), fixed)
    {}

    /**
     * @brief 构造实体尺寸
     * @param width 宽度（方块单位）
     * @param height 高度（方块单位）
     * @param eyeHeight 眼睛高度（方块单位）
     * @param fixed 是否为固定尺寸
     */
    constexpr EntitySize(f32 width, f32 height, f32 eyeHeight, bool fixed)
        : m_width(width)
        , m_height(height)
        , m_eyeHeight(eyeHeight)
        , m_fixed(fixed)
    {}

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] constexpr f32 width() const { return m_width; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] constexpr f32 height() const { return m_height; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] constexpr f32 eyeHeight() const { return m_eyeHeight; }

    /**
     * @brief 是否为固定尺寸
     *
     * 固定尺寸的实体不会根据姿态（如蹲下、游泳）调整尺寸。
     * 例如：船、矿车、盔甲架
     */
    [[nodiscard]] constexpr bool isFixed() const { return m_fixed; }

    /**
     * @brief 创建碰撞箱
     * @param x 实体X坐标
     * @param y 实体Y坐标（脚底）
     * @param z 实体Z坐标
     * @return AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB createBoundingBox(f64 x, f64 y, f64 z) const { return makeBoundingBox(x, y, z); }

    /**
     * @brief 创建碰撞箱
     * @param x, y, z 实体脚底位置
     * @return 以脚底为基准的AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB makeBoundingBox(f64 x, f64 y, f64 z) const
    {
        f32 halfWidth = m_width / 2.0f;
        return AxisAlignedBB(static_cast<f32>(x) - halfWidth,
            static_cast<f32>(y),
            static_cast<f32>(z) - halfWidth,
            static_cast<f32>(x) + halfWidth,
            static_cast<f32>(y) + m_height,
            static_cast<f32>(z) + halfWidth);
    }

    /**
     * @brief 创建碰撞箱
     * @param pos 实体位置
     * @return AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB createBoundingBox(const Vector3& pos) const
    {
        return makeBoundingBox(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 创建碰撞箱（双精度版本）
     * @param pos 实体位置（双精度）
     * @return AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB createBoundingBox(const Vector3d& pos) const
    {
        return makeBoundingBox(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 使用新的眼睛高度创建尺寸副本
     * @param eyeHeight 新的眼睛高度
     * @return 复制后的尺寸对象
     */
    [[nodiscard]] EntitySize withEyeHeight(f32 eyeHeight) const
    {
        return EntitySize(m_width, m_height, eyeHeight, m_fixed);
    }

    /**
     * @brief 缩放尺寸
     * @param factor 缩放因子
     * @return 缩放后的尺寸
     *
     * 如果是固定尺寸，返回原尺寸。
     */
    [[nodiscard]] EntitySize scale(f32 factor) const { return scale(factor, factor); }

    /**
     * @brief 分别缩放宽度和高度
     * @param widthFactor 宽度缩放因子
     * @param heightFactor 高度缩放因子
     * @return 缩放后的尺寸
     *
     * 如果是固定尺寸，返回原尺寸。
     */
    [[nodiscard]] EntitySize scale(f32 widthFactor, f32 heightFactor) const
    {
        if (m_fixed || (widthFactor == 1.0f && heightFactor == 1.0f)) {
            return *this;
        }
        return EntitySize(m_width * widthFactor, m_height * heightFactor, m_eyeHeight * heightFactor, false);
    }

    /**
     * @brief 创建可变尺寸
     */
    static constexpr EntitySize flexible(f32 width, f32 height) { return EntitySize(width, height, false); }

    /**
     * @brief 创建固定尺寸
     */
    static constexpr EntitySize fixed(f32 width, f32 height) { return EntitySize(width, height, true); }

    /**
     * @brief 比较操作符
     */
    [[nodiscard]] bool operator==(const EntitySize& other) const noexcept
    {
        return m_width == other.m_width && m_height == other.m_height && m_eyeHeight == other.m_eyeHeight &&
            m_fixed == other.m_fixed;
    }

    [[nodiscard]] bool operator!=(const EntitySize& other) const noexcept { return !(*this == other); }

private:
    [[nodiscard]] static constexpr f32 _defaultEyeHeight(f32 height) { return height * 0.85f; }

    f32 m_width;
    f32 m_height;
    f32 m_eyeHeight;
    bool m_fixed;
};

} // namespace mc::entity
