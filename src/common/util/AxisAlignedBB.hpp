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

#include "../core/Types.hpp"
#include "../util/math/Vector3.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

/**
 * @brief 轴对齐包围盒 (AABB)
 *
 * 用于碰撞检测的轴对齐包围盒，与Minecraft的AxisAlignedBB兼容。
 * 坐标系：minX/minY/minZ为最小角点，maxX/maxY/maxZ为最大角点。
 *
 * 注意：
 * - min坐标必须小于等于max坐标
 * - 所有坐标为世界坐标，非方块本地坐标
 * - calculateXOffset等方法实现MC的碰撞逻辑
 */
class AxisAlignedBB {
public:
    f32 minX, minY, minZ, maxX, maxY, maxZ;

    // 构造函数
    AxisAlignedBB() noexcept
        : minX(0.0f)
        , minY(0.0f)
        , minZ(0.0f)
        , maxX(0.0f)
        , maxY(0.0f)
        , maxZ(0.0f)
    {}

    /**
     * @brief 构造AABB
     * @param x1, y1, z1 第一个角点
     * @param x2, y2, z2 第二个角点
     * 注意：坐标会自动排序，确保min <= max
     */
    AxisAlignedBB(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) noexcept
        : minX(std::min(x1, x2))
        , minY(std::min(y1, y2))
        , minZ(std::min(z1, z2))
        , maxX(std::max(x1, x2))
        , maxY(std::max(y1, y2))
        , maxZ(std::max(z1, z2))
    {}

    /**
     * @brief 从实体位置创建AABB
     * @param pos 实体脚底位置
     * @param width 实体宽度
     * @param height 实体高度
     * @return 以pos为底面中心的AABB
     */
    [[nodiscard]] static AxisAlignedBB fromPosition(const Vector3& pos, f32 width, f32 height) noexcept
    {
        f32 hw = width / 2.0f;
        return AxisAlignedBB(pos.x - hw, pos.y, pos.z - hw, pos.x + hw, pos.y + height, pos.z + hw);
    }

    /**
     * @brief 从方块坐标创建AABB
     * @param x, y, z 方块坐标
     * @return 覆盖整个方块的AABB (x, y, z) -> (x+1, y+1, z+1)
     */
    [[nodiscard]] static AxisAlignedBB fromBlock(i32 x, i32 y, i32 z) noexcept
    {
        return AxisAlignedBB(static_cast<f32>(x),
            static_cast<f32>(y),
            static_cast<f32>(z),
            static_cast<f32>(x + 1),
            static_cast<f32>(y + 1),
            static_cast<f32>(z + 1));
    }

    /**
     * @brief 以指定点为中心构造AABB
     * @param center 中心点
     * @param xSize X轴方向总尺寸（非半尺寸）
     * @param ySize Y轴方向总尺寸
     * @param zSize Z轴方向总尺寸
     * @return 以 center 为中心、各轴尺寸为指定值的 AABB
     *
     * 对应 MC 1.21.11 AABB.ofSize(Vec3, double, double, double)。
     * min = center - size/2，max = center + size/2。
     */
    [[nodiscard]] static AxisAlignedBB ofSize(const Vector3& center, f32 xSize, f32 ySize, f32 zSize) noexcept
    {
        const f32 hx = xSize / 2.0f;
        const f32 hy = ySize / 2.0f;
        const f32 hz = zSize / 2.0f;
        return AxisAlignedBB(center.x - hx, center.y - hy, center.z - hz, center.x + hx, center.y + hy, center.z + hz);
    }

    // 基本属性
    [[nodiscard]] f32 width() const noexcept { return maxX - minX; }
    [[nodiscard]] f32 height() const noexcept { return maxY - minY; }
    [[nodiscard]] f32 depth() const noexcept { return maxZ - minZ; }

    [[nodiscard]] Vector3 center() const noexcept
    {
        return Vector3((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }

    [[nodiscard]] f32 volume() const noexcept { return (maxX - minX) * (maxY - minY) * (maxZ - minZ); }

    // 相交检测
    [[nodiscard]] bool intersects(const AxisAlignedBB& other) const noexcept
    {
        return minX < other.maxX && maxX > other.minX && minY < other.maxY && maxY > other.minY && minZ < other.maxZ &&
            maxZ > other.minZ;
    }

    /**
     * @brief 检查点是否在AABB内部
     * @param point 要检查的点
     * @return 点是否在AABB内部
     * @note 使用半开区间 [min, max)，与MC 1.16.5对齐
     */
    [[nodiscard]] bool contains(const Vector3& point) const noexcept
    {
        return point.x >= minX && point.x < maxX && point.y >= minY && point.y < maxY && point.z >= minZ &&
            point.z < maxZ;
    }

    [[nodiscard]] bool contains(const AxisAlignedBB& other) const noexcept
    {
        return minX <= other.minX && maxX >= other.maxX && minY <= other.minY && maxY >= other.maxY &&
            minZ <= other.minZ && maxZ >= other.maxZ;
    }

    // 变换
    void offset(f32 dx, f32 dy, f32 dz) noexcept
    {
        minX += dx;
        maxX += dx;
        minY += dy;
        maxY += dy;
        minZ += dz;
        maxZ += dz;
    }

    [[nodiscard]] AxisAlignedBB offsetted(f32 dx, f32 dy, f32 dz) const noexcept
    {
        return AxisAlignedBB(minX + dx, minY + dy, minZ + dz, maxX + dx, maxY + dy, maxZ + dz);
    }

    /**
     * @brief 扩展AABB（向各方向扩展相同距离）
     * @param dx, dy, dz 各方向的扩展量
     * @return 扩展后的AABB
     */
    [[nodiscard]] AxisAlignedBB expand(f32 dx, f32 dy, f32 dz) const noexcept
    {
        return AxisAlignedBB(minX - dx, minY - dy, minZ - dz, maxX + dx, maxY + dy, maxZ + dz);
    }

    /**
     * @brief 均匀扩展AABB
     * @param amount 各方向扩展量
     */
    [[nodiscard]] AxisAlignedBB grow(f32 amount) const noexcept { return expand(amount, amount, amount); }

    /**
     * @brief 均匀收缩AABB
     * @param amount 各方向收缩量
     */
    [[nodiscard]] AxisAlignedBB shrink(f32 amount) const noexcept { return expand(-amount, -amount, -amount); }

    /**
     * @brief 均匀收缩AABB（MC 命名，等价于 shrink）
     *
     * 对应 MC 1.21 的 AABB.deflate(double)。Jigsaw 空间追踪中用于将新拼图块的 AABB
     * 收缩 0.25 格后与 freeShape 做交集检测，避免相邻块被误判为重叠。
     *
     * @param amount 各方向收缩量
     * @return 收缩后的 AABB
     */
    [[nodiscard]] AxisAlignedBB deflate(f32 amount) const noexcept { return expand(-amount, -amount, -amount); }

    /**
     * @brief 沿速度向量方向非对称扩展 AABB（对应 MC 1.21 AABB.expandTowards）
     *
     * 与对称扩展的 expand/grow 不同：仅沿各轴速度分量的方向扩展——速度分量为正时
     * 扩展 max 侧，为负时扩展 min 侧，零则不扩。用于把实体"本帧将扫过"的区域纳入
     * 碰撞/命中检测（如 Projectile.isOutsideOwnerCollisionRange 用 expandTowards(
     * deltaMovement) 构造移动 AABB）。
     *
     * 对应 vanilla:
     *   AABB.expandTowards(double x, double y, double z) {
     *     double d0 = x < 0 ? x : 0, d1 = x > 0 ? x : 0; // 同理 y/z
     *     return new AABB(minX+d0, minY+d2, minZ+d4, maxX+d1, maxY+d3, maxZ+d5);
     *   }
     *
     * @param dx, dy, dz 各轴速度分量（可正可负）
     * @return 沿速度方向扩展后的 AABB
     */
    [[nodiscard]] AxisAlignedBB expandTowards(f32 dx, f32 dy, f32 dz) const noexcept
    {
        const f32 minXoff = dx < 0.0f ? dx : 0.0f;
        const f32 maxXoff = dx > 0.0f ? dx : 0.0f;
        const f32 minYoff = dy < 0.0f ? dy : 0.0f;
        const f32 maxYoff = dy > 0.0f ? dy : 0.0f;
        const f32 minZoff = dz < 0.0f ? dz : 0.0f;
        const f32 maxZoff = dz > 0.0f ? dz : 0.0f;
        return AxisAlignedBB(
            minX + minXoff, minY + minYoff, minZ + minZoff, maxX + maxXoff, maxY + maxYoff, maxZ + maxZoff);
    }

    /**
     * @brief 沿向量方向非对称扩展 AABB（对应 MC 1.21 AABB.expandTowards(Vec3)）
     * @param movement 速度向量
     */
    [[nodiscard]] AxisAlignedBB expandTowards(const Vector3& movement) const noexcept
    {
        return expandTowards(movement.x, movement.y, movement.z);
    }

    /**
     * @brief 均匀向外膨胀 AABB（对应 MC 1.21 AABB.inflate(double)）
     *
     * 与 grow 等价，使用 vanilla 命名以便对齐源码时直译。grow 保留为项目既有命名。
     *
     * @param amount 各方向膨胀量
     */
    [[nodiscard]] AxisAlignedBB inflate(f32 amount) const noexcept { return expand(amount, amount, amount); }

    // ========== MC碰撞检测核心算法 ==========

    /**
     * @brief 计算沿X轴移动时的最大允许偏移量
     *
     * 这是Minecraft Entity.move中的核心碰撞算法。
     * 当实体沿X轴移动时，检查是否会与另一个AABB碰撞，
     * 返回不会发生穿透的最大偏移量。
     *
     * @param other 另一个碰撞箱
     * @param offsetX 期望的X轴偏移量（可正可负）
     * @return 实际允许的偏移量（不会穿透other）
     *
     * 注意：
     * - 如果Y或Z轴范围不相交，返回原偏移量
     * - 正偏移时，返回min(offsetX, other.minX - maxX)
     * - 负偏移时，返回max(offsetX, other.maxX - minX)
     */
    [[nodiscard]] f32 calculateXOffset(const AxisAlignedBB& other, f32 offsetX) const noexcept
    {
        // Y或Z范围不相交，不影响移动
        if (maxY <= other.minY || minY >= other.maxY) return offsetX;
        if (maxZ <= other.minZ || minZ >= other.maxZ) return offsetX;

        if (offsetX > 0.0f) {
            // 仅当other在当前包围盒右侧时才可能阻挡
            if (other.minX >= maxX) {
                f32 maxMove = other.minX - maxX;
                if (maxMove < offsetX) {
                    offsetX = maxMove;
                }
            }
            return offsetX;
        } else if (offsetX < 0.0f) {
            // 仅当other在当前包围盒左侧时才可能阻挡
            if (other.maxX <= minX) {
                f32 maxMove = other.maxX - minX;
                if (maxMove > offsetX) {
                    offsetX = maxMove;
                }
            }
            return offsetX;
        }
        return offsetX;
    }

    /**
     * @brief 计算沿Y轴移动时的最大允许偏移量
     * @param other 另一个碰撞箱
     * @param offsetY 期望的Y轴偏移量
     * @return 实际允许的偏移量
     */
    [[nodiscard]] f32 calculateYOffset(const AxisAlignedBB& other, f32 offsetY) const noexcept
    {
        // X或Z范围不相交，不影响移动
        if (maxX <= other.minX || minX >= other.maxX) return offsetY;
        if (maxZ <= other.minZ || minZ >= other.maxZ) return offsetY;

        if (offsetY > 0.0f) {
            // 仅当other在当前包围盒上方时才可能阻挡
            if (other.minY >= maxY) {
                f32 maxMove = other.minY - maxY;
                if (maxMove < offsetY) {
                    offsetY = maxMove;
                }
            }
            return offsetY;
        } else if (offsetY < 0.0f) {
            // 仅当other在当前包围盒下方时才可能阻挡
            if (other.maxY <= minY) {
                f32 maxMove = other.maxY - minY;
                if (maxMove > offsetY) {
                    offsetY = maxMove;
                }
            }
            return offsetY;
        }
        return offsetY;
    }

    /**
     * @brief 计算沿Z轴移动时的最大允许偏移量
     * @param other 另一个碰撞箱
     * @param offsetZ 期望的Z轴偏移量
     * @return 实际允许的偏移量
     */
    [[nodiscard]] f32 calculateZOffset(const AxisAlignedBB& other, f32 offsetZ) const noexcept
    {
        // X或Y范围不相交，不影响移动
        if (maxX <= other.minX || minX >= other.maxX) return offsetZ;
        if (maxY <= other.minY || minY >= other.maxY) return offsetZ;

        if (offsetZ > 0.0f) {
            // 仅当other在当前包围盒前方时才可能阻挡
            if (other.minZ >= maxZ) {
                f32 maxMove = other.minZ - maxZ;
                if (maxMove < offsetZ) {
                    offsetZ = maxMove;
                }
            }
            return offsetZ;
        } else if (offsetZ < 0.0f) {
            // 仅当other在当前包围盒后方时才可能阻挡
            if (other.maxZ <= minZ) {
                f32 maxMove = other.maxZ - minZ;
                if (maxMove > offsetZ) {
                    offsetZ = maxMove;
                }
            }
            return offsetZ;
        }
        return offsetZ;
    }

    /**
     * @brief 计算点到本 AABB 最近表面距离的平方
     *
     * 对应 MC 1.21.11 AABB.distanceToSqr(Vec3)。点在盒内则返回 0。
     * 每轴取 max(min - p, p - max, 0)，再求 lengthSquared。
     * 用于实体交互范围判定（玩家眼位到目标 AABB）。
     */
    [[nodiscard]] f32 distanceToSqr(const Vector3& p) const noexcept
    {
        const f32 dx = std::max({minX - p.x, p.x - maxX, 0.0f});
        const f32 dy = std::max({minY - p.y, p.y - maxY, 0.0f});
        const f32 dz = std::max({minZ - p.z, p.z - maxZ, 0.0f});
        return dx * dx + dy * dy + dz * dz;
    }

    // 比较运算
    [[nodiscard]] bool operator==(const AxisAlignedBB& other) const noexcept
    {
        return minX == other.minX && minY == other.minY && minZ == other.minZ && maxX == other.maxX &&
            maxY == other.maxY && maxZ == other.maxZ;
    }

    [[nodiscard]] bool operator!=(const AxisAlignedBB& other) const noexcept { return !(*this == other); }
};

} // namespace mc
