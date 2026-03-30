#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/AxisAlignedBB.hpp"

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::f32;
using mc::f64;
using mc::Vector3;
using mc::Vector3d;
using mc::AxisAlignedBB;

/**
 * @brief 实体尺寸类
 *
 * 存储实体的宽度和高度，用于计算碰撞箱。
 * 固定尺寸(fixed=true)的实体不会根据姿态变化调整尺寸。
 *
 * 参考 MC 1.16.5 EntitySize (EntityDimensions)
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
        : m_width(width)
        , m_height(height)
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
    [[nodiscard]] AxisAlignedBB createBoundingBox(f64 x, f64 y, f64 z) const {
        f32 halfWidth = m_width / 2.0f;
        return AxisAlignedBB(
            static_cast<f32>(x) - halfWidth, static_cast<f32>(y), static_cast<f32>(z) - halfWidth,
            static_cast<f32>(x) + halfWidth, static_cast<f32>(y) + m_height, static_cast<f32>(z) + halfWidth
        );
    }

    /**
     * @brief 创建碰撞箱
     * @param pos 实体位置
     * @return AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB createBoundingBox(const Vector3& pos) const {
        return createBoundingBox(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 创建碰撞箱（双精度版本）
     * @param pos 实体位置（双精度）
     * @return AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB createBoundingBox(const Vector3d& pos) const {
        return createBoundingBox(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 缩放尺寸
     * @param factor 缩放因子
     * @return 缩放后的尺寸
     *
     * 如果是固定尺寸，返回原尺寸。
     */
    [[nodiscard]] EntitySize scale(f32 factor) const {
        return scale(factor, factor);
    }

    /**
     * @brief 分别缩放宽度和高度
     * @param widthFactor 宽度缩放因子
     * @param heightFactor 高度缩放因子
     * @return 缩放后的尺寸
     *
     * 如果是固定尺寸，返回原尺寸。
     */
    [[nodiscard]] EntitySize scale(f32 widthFactor, f32 heightFactor) const {
        if (m_fixed || (widthFactor == 1.0f && heightFactor == 1.0f)) {
            return *this;
        }
        return flexible(m_width * widthFactor, m_height * heightFactor);
    }

    /**
     * @brief 创建可变尺寸
     */
    static constexpr EntitySize flexible(f32 width, f32 height) {
        return EntitySize(width, height, false);
    }

    /**
     * @brief 创建固定尺寸
     */
    static constexpr EntitySize fixed(f32 width, f32 height) {
        return EntitySize(width, height, true);
    }

    /**
     * @brief 比较操作符
     */
    bool operator==(const EntitySize& other) const {
        return m_width == other.m_width &&
               m_height == other.m_height &&
               m_fixed == other.m_fixed;
    }

    bool operator!=(const EntitySize& other) const {
        return !(*this == other);
    }

private:
    f32 m_width;
    f32 m_height;
    bool m_fixed;
};

} // namespace mc::entity
