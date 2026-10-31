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

/**
 * @file GroundPathNavigator.hpp
 * @brief 地面寻路导航器（对应原版 `GroundPathNavigation`）。
 *
 * 与基类的差别都是地面实体特有的：
 *   - 只在站在地面、液体中或骑乘时跟随路径（悬空下落时无法转向）；
 *   - 卡住判定用「地表 Y」而不是实体实际 Y，避免下落过程被误判为卡住；
 *   - 目标点若悬在空中，向下 / 向上吸附到最近的地表（原版 `findSurfacePosition`）；
 *   - 可选的阳光避让：路径在第一个暴露于天空的节点处截断。
 */

#pragma once

#include "PathNavigator.hpp"

namespace mc::entity::ai::pathfinding {

/**
 * @brief 地面寻路导航器
 */
class GroundPathNavigator final : public PathNavigator {
public:
    GroundPathNavigator(MobEntity* mob, std::unique_ptr<PathFinder> pathFinder);

    [[nodiscard]] bool canNavigateGround() const override
    {
        return true;
    }

    /**
     * @brief 走到指定方块，先把目标吸附到地表
     *
     * @param target 目标方块坐标
     * @param reachRange 判定到达的曼哈顿半径
     * @param speed 速度倍率
     * @return 是否成功建立并开始跟随路径
     */
    [[nodiscard]] bool moveToBlock(const BlockPos& target, i32 reachRange, f64 speed);

    /**
     * @brief 是否只寻找地表目标（false 时目标按原样使用）
     */
    void setCanPathToTargetsBelowSurface(bool canPathToTargetsBelowSurface) noexcept
    {
        m_canPathToTargetsBelowSurface = canPathToTargetsBelowSurface;
    }

protected:
    [[nodiscard]] bool _canUpdatePath() const override;

    [[nodiscard]] Vector3d _getTempMobPosition() const override;

    void _trimPath() override;

private:
    /**
     * @brief 把目标 Y 吸附到最近的地表
     *
     * 目标在空气中时向下找第一块非空气方块，再取其上方；目标是实心方块时向上找第一块
     * 空气。这样「点击头顶的空气」和「点击脚下的方块」都会落到同一层地面上。
     */
    [[nodiscard]] BlockPos _findSurfacePosition(const BlockPos& target) const;

    /// 地面实体的「地表 Y」。
    [[nodiscard]] i32 _getSurfaceY() const;

    bool m_canPathToTargetsBelowSurface = false;
};

} // namespace mc::entity::ai::pathfinding