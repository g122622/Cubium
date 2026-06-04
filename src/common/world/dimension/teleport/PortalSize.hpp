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
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>
#include <vector>

namespace mc {

class IWorld;
class BlockState;

/**
 * @brief 传送门尺寸检测结果
 *
 * 包含检测到的传送门的位置和尺寸信息。
 */
struct PortalSizeResult {
    BlockPos corner;          ///< 传送门内部左下角位置
    i32 width = 0;            ///< 内部宽度 (2-21)
    i32 height = 0;           ///< 内部高度 (3-21)
    Axis axis;                ///< 传送门轴向 (X 或 Z)
    i32 portalBlockCount = 0; ///< 已存在的传送门方块数量
    bool valid = false;       ///< 是否有效

    /**
     * @brief 获取传送门内部所有方块位置
     * @return 方块位置列表
     */
    [[nodiscard]] std::vector<BlockPos> getPortalBlocks() const;
};

/**
 * @brief 传送门尺寸检测工具
 *
 * 检测黑曜石框架（下界传送门）和末地传送门框架。
 */
class PortalSize {
public:
    static constexpr i32 MIN_WIDTH = 2;
    static constexpr i32 MAX_WIDTH = 21;
    static constexpr i32 MIN_HEIGHT = 3;
    static constexpr i32 MAX_HEIGHT = 21;
    static constexpr i32 MAX_SEARCH_DOWN = 21;

    /**
     * @brief 在指定位置寻找有效的下界传送门框架
     *
     * @param world 世界引用
     * @param pos 搜索中心位置（通常是火焰位置）
     * @param preferXAxis 是否优先搜索 X 轴传送门
     * @return 检测结果
     */
    [[nodiscard]] static std::optional<PortalSizeResult> findNetherPortal(
        IWorld& world, const BlockPos& pos, bool preferXAxis = true);

    /**
     * @brief 点燃下界传送门
     * @param world 世界引用
     * @param portal 传送门尺寸信息
     * @return 是否成功点燃
     */
    static bool lightNetherPortal(IWorld& world, const PortalSizeResult& portal);

    /**
     * @brief 检查方块状态是否可以作为传送门内部方块
     * @param state 方块状态
     * @return 是否可以连接（空气、火焰或传送门方块）
     */
    [[nodiscard]] static bool canConnect(const BlockState& state);

private:
    /**
     * @brief 尝试在指定轴向上寻找传送门
     * @param world 世界引用
     * @param pos 搜索位置
     * @param rightDir 右方向（决定轴向）
     * @return 检测结果
     */
    [[nodiscard]] static std::optional<PortalSizeResult> _tryFindPortalOnAxis(
        IWorld& world, const BlockPos& pos, Direction rightDir);

    /**
     * @brief 寻找传送门左下角位置
     * @param world 世界引用
     * @param pos 搜索位置
     * @param rightDir 右方向
     * @return 左下角位置
     */
    [[nodiscard]] static std::optional<BlockPos> _findBottomLeft(
        IWorld& world, const BlockPos& pos, Direction rightDir);

    /**
     * @brief 计算传送门宽度
     * @param world 世界引用
     * @param bottomLeft 左下角位置
     * @param rightDir 右方向
     * @return 宽度（无效返回0）
     */
    [[nodiscard]] static i32 _calculateWidth(IWorld& world, const BlockPos& bottomLeft, Direction rightDir);

    /**
     * @brief 计算传送门高度
     * @param world 世界引用
     * @param bottomLeft 左下角位置
     * @param rightDir 右方向
     * @param width 已计算的宽度
     * @param outPortalBlockCount 输出：已存在的传送门方块数量
     * @return 高度
     */
    [[nodiscard]] static i32 _calculateHeight(
        IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32& outPortalBlockCount);

    /**
     * @brief 检查顶部框架是否完整
     * @param world 世界引用
     * @param bottomLeft 左下角位置
     * @param rightDir 右方向
     * @param width 宽度
     * @param height 高度
     * @return 顶部框架是否有效
     */
    [[nodiscard]] static bool _checkTopFrame(
        IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32 height);

    /**
     * @brief 检查方块是否为传送门框架方块（黑曜石）
     * @param state 方块状态
     * @return 是否为框架方块
     */
    [[nodiscard]] static bool _isPortalFrame(const BlockState& state);
};

} // namespace mc
