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

#include "PortalSize.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace mc {

// ============================================================================
// PortalSizeResult 方法
// ============================================================================

std::vector<BlockPos> PortalSizeResult::getPortalBlocks() const
{
    std::vector<BlockPos> blocks;
    if (!valid) {
        return blocks;
    }

    blocks.reserve(static_cast<size_t>(width) * static_cast<size_t>(height));

    // 根据轴向确定遍历方向
    Direction widthDir = (axis == Axis::X) ? Direction::East : Direction::South;

    for (i32 h = 0; h < height; ++h) {
        for (i32 w = 0; w < width; ++w) {
            BlockPos pos = corner;
            pos = pos.offset(Direction::Up, h);
            pos = pos.offset(widthDir, w);
            blocks.push_back(pos);
        }
    }

    return blocks;
}

// ============================================================================
// PortalSize 方法
// ============================================================================

std::optional<PortalSizeResult> PortalSize::findNetherPortal(IWorld& world, const BlockPos& pos, bool preferXAxis)
{
    // 先尝试优先轴向，再尝试另一个轴向
    // 当 axis 为 X 时，rightDir 为 WEST（向左方向）
    Direction firstDir = preferXAxis ? Direction::West : Direction::South;
    auto result = _tryFindPortalOnAxis(world, pos, firstDir);
    if (result.has_value() && result->valid && result->portalBlockCount == 0) {
        return result;
    }

    // 再尝试另一个轴向
    Direction secondDir = preferXAxis ? Direction::South : Direction::West;
    result = _tryFindPortalOnAxis(world, pos, secondDir);
    if (result.has_value() && result->valid && result->portalBlockCount == 0) {
        return result;
    }

    return std::nullopt;
}

bool PortalSize::lightNetherPortal(IWorld& world, const PortalSizeResult& portal)
{
    if (!portal.valid) {
        return false;
    }

    auto blocks = portal.getPortalBlocks();
    if (blocks.empty()) {
        return false;
    }

    if (VanillaBlocks::NETHER_PORTAL == nullptr) {
        return false;
    }

    // 设置传送门方块轴向
    const BlockState* portalState =
        &VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), portal.axis);

    for (const BlockPos& blockPos : blocks) {
        // Block update flags: 2 = notify neighbors, 16 = prevent recursion
        // Combined: 18 = notify neighbors without triggering recursive updates
        constexpr i32 BLOCK_UPDATE_NOTIFY_NEIGHBORS = 2;
        constexpr i32 BLOCK_UPDATE_NO_RECURSION = 16;
        world.setBlockState(blockPos, portalState, BLOCK_UPDATE_NOTIFY_NEIGHBORS | BLOCK_UPDATE_NO_RECURSION);
    }

    return true;
}

bool PortalSize::canConnect(const BlockState& state)
{
    if (state.isAir()) {
        return true;
    }
    if (VanillaBlocks::FIRE != nullptr && state.is(VanillaBlocks::FIRE)) {
        return true;
    }
    if (VanillaBlocks::NETHER_PORTAL != nullptr && state.is(VanillaBlocks::NETHER_PORTAL)) {
        return true;
    }
    return false;
}

bool PortalSize::_isPortalFrame(const BlockState& state)
{
    if (VanillaBlocks::OBSIDIAN != nullptr && state.is(VanillaBlocks::OBSIDIAN)) {
        return true;
    }
    // 未来可扩展支持模组自定义框架方块
    return false;
}

std::optional<PortalSizeResult> PortalSize::_tryFindPortalOnAxis(IWorld& world, const BlockPos& pos, Direction rightDir)
{
    auto bottomLeft = _findBottomLeft(world, pos, rightDir);
    if (!bottomLeft.has_value()) {
        return std::nullopt;
    }

    i32 width = _calculateWidth(world, *bottomLeft, rightDir);
    if (width < MIN_WIDTH || width > MAX_WIDTH) {
        return std::nullopt;
    }

    i32 portalBlockCount = 0;
    i32 height = _calculateHeight(world, *bottomLeft, rightDir, width, portalBlockCount);
    if (height < MIN_HEIGHT || height > MAX_HEIGHT) {
        return std::nullopt;
    }

    if (!_checkTopFrame(world, *bottomLeft, rightDir, width, height)) {
        return std::nullopt;
    }

    // 将 Direction 转换为 Axis
    Axis axis = (rightDir == Direction::West || rightDir == Direction::East) ? Axis::X : Axis::Z;

    PortalSizeResult result;
    result.corner = *bottomLeft;
    result.width = width;
    result.height = height;
    result.axis = axis;
    result.portalBlockCount = portalBlockCount;
    result.valid = true;
    return result;
}

std::optional<BlockPos> PortalSize::_findBottomLeft(IWorld& world, const BlockPos& pos, Direction rightDir)
{
    Direction leftDir = Directions::opposite(rightDir);

    // 第一步：向下搜索找到内部底部
    BlockPos currentPos = pos;
    i32 minY = std::max(world::MIN_BUILD_HEIGHT, pos.y - MAX_SEARCH_DOWN);

    while (currentPos.y > minY) {
        BlockPos belowPos = currentPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !canConnect(*belowState)) {
            break;
        }
        currentPos = belowPos;
    }

    // 第二步：向左搜索框架
    // 连接方块需底部有框架，框架方块则返回
    i32 leftDistance = -1;
    for (i32 i = 0; i <= MAX_WIDTH + 1; ++i) {
        BlockPos checkPos = currentPos.offset(leftDir, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (state == nullptr) {
            break;
        }

        if (canConnect(*state)) {
            BlockPos belowCheckPos = checkPos.offset(Direction::Down);
            const BlockState* belowState = world.getBlockState(belowCheckPos);
            if (belowState == nullptr || !_isPortalFrame(*belowState)) {
                break;
            }
            continue;
        }

        if (_isPortalFrame(*state)) {
            leftDistance = i;
            break;
        }

        break;
    }

    if (leftDistance <= 0) {
        return std::nullopt;
    }

    return currentPos.offset(leftDir, leftDistance - 1);
}

i32 PortalSize::_calculateWidth(IWorld& world, const BlockPos& bottomLeft, Direction rightDir)
{
    for (i32 i = 0; i <= MAX_WIDTH; ++i) {
        BlockPos checkPos = bottomLeft.offset(rightDir, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (state == nullptr) {
            return 0;
        }

        if (!canConnect(*state)) {
            if (_isPortalFrame(*state)) {
                return i;
            }
            return 0;
        }

        BlockPos belowPos = checkPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !_isPortalFrame(*belowState)) {
            return 0;
        }
    }
    return 0;
}

i32 PortalSize::_calculateHeight(
    IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32& outPortalBlockCount)
{
    outPortalBlockCount = 0;
    Direction leftDir = Directions::opposite(rightDir);

    for (i32 h = 0; h < MAX_HEIGHT; ++h) {
        // 检查左边框架
        BlockPos leftFramePos = bottomLeft.offset(Direction::Up, h).offset(leftDir);
        const BlockState* leftState = world.getBlockState(leftFramePos);
        if (leftState == nullptr || !_isPortalFrame(*leftState)) {
            return h;
        }

        // 检查右边框架
        BlockPos rightFramePos = bottomLeft.offset(Direction::Up, h).offset(rightDir, width);
        const BlockState* rightState = world.getBlockState(rightFramePos);
        if (rightState == nullptr || !_isPortalFrame(*rightState)) {
            return h;
        }

        // 检查内部
        for (i32 w = 0; w < width; ++w) {
            BlockPos interiorPos = bottomLeft.offset(Direction::Up, h).offset(rightDir, w);
            const BlockState* interiorState = world.getBlockState(interiorPos);
            if (interiorState == nullptr || !canConnect(*interiorState)) {
                return h;
            }

            if (VanillaBlocks::NETHER_PORTAL != nullptr && interiorState->is(VanillaBlocks::NETHER_PORTAL)) {
                ++outPortalBlockCount;
            }
        }
    }

    return MAX_HEIGHT;
}

bool PortalSize::_checkTopFrame(IWorld& world, const BlockPos& bottomLeft, Direction rightDir, i32 width, i32 height)
{
    BlockPos topFramePos = bottomLeft.offset(Direction::Up, height);

    for (i32 w = 0; w < width; ++w) {
        BlockPos pos = topFramePos.offset(rightDir, w);
        const BlockState* state = world.getBlockState(pos);
        if (state == nullptr || !_isPortalFrame(*state)) {
            return false;
        }
    }

    return true;
}

} // namespace mc
