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

// 前向声明
namespace mc {
class BlockState;
}

namespace mc::entity::ai::pathfinding {

/**
 * @brief 世界区域访问接口
 *
 * 提供寻路算法对世界区块的有限访问。
 * 这是一个抽象接口，允许不同实现（服务端世界、客户端世界等）。
 */
class Region {
public:
    virtual ~Region() = default;

    /**
     * @brief 获取指定位置的方块状态ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 方块状态ID，如果位置无效返回0（空气）
     */
    [[nodiscard]] virtual u32 getBlockStateId(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取指定位置的方块状态
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 方块状态指针，如果位置无效或为空气返回nullptr
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否在加载范围内
     */
    [[nodiscard]] virtual bool isLoaded(i32 x, i32 z) const = 0;

    /**
     * @brief 获取最高方块Y坐标
     * @param x X坐标
     * @param z Z坐标
     * @return 最高方块Y坐标
     */
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z) const = 0;

    /**
     * @brief 检查位置是否可通行
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否可通行
     */
    [[nodiscard]] virtual bool isWalkable(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否是水
     */
    [[nodiscard]] virtual bool isWater(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否是岩浆
     */
    [[nodiscard]] virtual bool isLava(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取方块顶部高度（考虑台阶、楼梯等）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 顶部高度偏移（通常为0或0.5）
     */
    [[nodiscard]] virtual f32 getBlockTopY(i32 /*x*/, i32 y, i32 /*z*/) const
    {
        // 默认实现
        return static_cast<f32>(y) + 1.0f;
    }

    /**
     * @brief 检查指定位置是否能看到天空
     *
     * 用于寻路中的阳光避让逻辑。如果位置上方无遮挡方块（天空光照>=15），
     * 则认为可以看到天空。
     *
     * TODO: 当前阳光避让路径截断通过 PathNavigator::_trimPath() 直接使用
     * IWorld::canSeeSky() 实现，此接口尚未被 WalkNodeProcessor 调用。
     * 未来若需要在节点代价计算级别实现阳光避让（如对阳光节点增加代价），
     * 则 WalkNodeProcessor 需要通过此接口查询天空可见性。
     * 届时具体 Region 实现需委托到 IWorld::canSeeSky() 或使用高度图快速判断。
     *
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否能看到天空
     */
    [[nodiscard]] virtual bool canSeeSky(i32 x, i32 y, i32 z) const = 0;
};

} // namespace mc::entity::ai::pathfinding
