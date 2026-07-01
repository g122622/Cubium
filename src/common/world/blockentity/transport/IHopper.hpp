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

#include "util/AxisAlignedBB.hpp"
#include "util/Direction.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 漏斗接口
 *
 * 定义漏斗的通用接口，用于统一处理漏斗方块和漏斗矿车。
 * 提供位置获取方法，用于物品传输时查找相邻容器。
 */
class IHopper {
public:
    virtual ~IHopper() = default;

    /**
     * @brief 获取漏斗所在的世界
     * @return 世界指针（可能为nullptr）
     */
    [[nodiscard]] virtual IWorld* getWorld() = 0;
    [[nodiscard]] virtual const IWorld* getWorld() const = 0;

    /**
     * @brief 获取漏斗的X坐标（世界坐标）
     * @return X坐标
     */
    [[nodiscard]] virtual f64 getXPos() const = 0;

    /**
     * @brief 获取漏斗的Y坐标（世界坐标）
     * @return Y坐标
     */
    [[nodiscard]] virtual f64 getYPos() const = 0;

    /**
     * @brief 获取漏斗的Z坐标（世界坐标）
     * @return Z坐标
     */
    [[nodiscard]] virtual f64 getZPos() const = 0;

    /**
     * @brief 获取漏斗位置（方块坐标）
     * @return 方块位置
     */
    [[nodiscard]] virtual BlockPos getHopperPos() const = 0;

    /**
     * @brief 获取漏斗输出方向
     * @return 输出方向（默认向下）
     */
    [[nodiscard]] virtual Direction getOutputDirection() const { return Direction::Down; }

    /**
     * @brief 漏斗是否对齐网格
     * @return 方块漏斗返回true，漏斗矿车返回false
     *
     * MC Java 的 Hopper.isGridAligned()：方块漏斗返回 true，
     * 漏斗矿车返回 false。影响物品吸取逻辑：当 isGridAligned() 为 true
     * 且上方方块碰撞形状为完整方块时，漏斗不会吸取物品实体。
     */
    [[nodiscard]] virtual bool isGridAligned() const { return true; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取漏斗上方的收集区域
     * @param hopper 漏斗
     * @return 收集区域的AABB
     *
     * MC Java 的 SUCK_AABB 由 Block.column(16.0, 11.0, 32.0) 定义，
     * 转换后为方块局部坐标 (0, 11/16, 0) -> (1, 2, 1)，
     * 即从漏斗碗口顶部（Y=11/16）向上延伸一整格。
     * 世界坐标下为 (blockX, blockY + 0.6875, blockZ) -> (blockX + 1, blockY + 2, blockZ + 1)。
     */
    [[nodiscard]] static AxisAlignedBB getCollectionArea(const IHopper& hopper);

    /**
     * @brief 获取漏斗的输出位置
     * @param hopper 漏斗
     * @return 输出位置
     */
    [[nodiscard]] static BlockPos getOutputPosition(const IHopper& hopper) noexcept
    {
        return hopper.getHopperPos().offset(hopper.getOutputDirection());
    }
};

} // namespace blockentity
} // namespace mc
