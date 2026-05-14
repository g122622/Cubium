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

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include <algorithm>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace world {
namespace redstone {

/**
 * @brief 红石辅助工具函数
 *
 * 提供红石系统常用的辅助函数。
 */
class RedstoneHelper {
public:
    /**
     * @brief 检查方块是否是实体方块（可以传导信号）
     *
     * 实体方块的特征：
     * - 是固体
     * - 是不透明的
     * - 不是空气
     *
     * @param state 方块状态
     * @return true 如果是实体方块
     */
    [[nodiscard]] static bool isNormalCube(const BlockState& state);

    /**
     * @brief 检查方块是否可以连接红石
     *
     * 可以连接红石的方块：
     * - 红石线
     * - 红石火把
     * - 中继器
     * - 比较器
     * - 其他可以输出红石信号的方块
     *
     * @param state 方块状态
     * @return true 如果可以连接
     */
    [[nodiscard]] static bool canConnectRedstone(const BlockState& state);

    /**
     * @brief 检查方块是否是红石导体
     *
     * 红石导体可以传导弱信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @return true 如果是红石导体
     */
    [[nodiscard]] static bool isRedstoneConductor(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取实体的红石信号强度
     *
     * 某些实体（如矿车）可以输出红石信号。
     *
     * @param world 世界引用
     * @param pos 检测位置
     * @return i32 信号强度 0-15
     */
    [[nodiscard]] static i32 getEntitySignal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方向是否是水平方向
     *
     * @param dir 方向
     * @return true 如果是水平方向（北、东、南、西）
     */
    [[nodiscard]] static bool isHorizontal(Direction dir) { return Directions::isHorizontal(dir); }

    /**
     * @brief 检查方向是否是垂直方向
     *
     * @param dir 方向
     * @return true 如果是垂直方向（上、下）
     */
    [[nodiscard]] static bool isVertical(Direction dir) { return Directions::isVertical(dir); }

    /**
     * @brief 计算红石信号衰减后的强度
     *
     * @param strength 原始强度 (0-15)
     * @param distance 传输距离
     * @return i32 衰减后的强度 (最小为0)
     */
    [[nodiscard]] static i32 attenuate(i32 strength, i32 distance) { return std::max(0, strength - distance); }

    /**
     * @brief 限制红石信号强度在有效范围内
     *
     * @param strength 信号强度
     * @return i32 限制后的强度 (0-15)
     */
    [[nodiscard]] static i32 clamp(i32 strength) { return std::clamp(strength, MIN_POWER, MAX_POWER); }

    /// 红石信号最小强度
    static constexpr i32 MIN_POWER = 0;

    /// 红石信号最大强度
    static constexpr i32 MAX_POWER = 15;
};

} // namespace redstone
} // namespace world
} // namespace mc
