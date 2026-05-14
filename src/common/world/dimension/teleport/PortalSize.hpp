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

#include "../../../core/Types.hpp"
#include "../../../util/Direction.hpp"
#include "../../block/BlockPos.hpp"
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

    [[nodiscard]] std::vector<BlockPos> getPortalBlocks() const;
};

/**
 * @brief 传送门尺寸检测工具
 *
 * 参考 MC 1.16.5 PortalSize
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
     */
    static bool lightNetherPortal(IWorld& world, const PortalSizeResult& portal);

    /**
     * @brief 检查方块状态是否可以作为传送门内部方块
     */
    [[nodiscard]] static bool canConnect(const BlockState& state);

private:
    [[nodiscard]] static std::optional<PortalSizeResult> tryFindPortalOnAxis(
        IWorld& world, const BlockPos& pos, Direction rightDir);

    [[nodiscard]] static std::optional<BlockPos> findBottomLeft(IWorld& world, const BlockPos& pos, Direction rightDir);

    [[nodiscard]] static i32 calculateWidth(IWorld& world, const BlockPos& bottomLeft, Direction rightDir);

    [[nodiscard]] static i32 calculateHeight(
        IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32& outPortalBlockCount);

    [[nodiscard]] static bool checkTopFrame(
        IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32 height);

    [[nodiscard]] static bool isPortalFrame(const BlockState& state);
};

} // namespace mc
