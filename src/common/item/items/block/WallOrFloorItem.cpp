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

#include "WallOrFloorItem.hpp"

#include <vector>

#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

WallOrFloorItem::WallOrFloorItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties)
    : BlockItem(floorBlock, properties)
    , m_wallBlock(&wallBlock)
{}

const BlockState* WallOrFloorItem::getStateForPlacement(const BlockItemUseContext& context) const
{
    // 获取玩家视线方向的优先级列表
    const std::vector<Direction> nearestDirections = context.getNearestLookingDirections();

    const IWorld& world = context.getWorld();
    const BlockPos& pos = context.placementPos();

    const BlockState* resultState = nullptr;

    // 遍历方向，寻找可放置的方向
    for (Direction direction : nearestDirections) {
        // 跳过 UP 方向（墙上的物品不能放在天花板上）
        if (direction == Direction::Up) {
            continue;
        }

        const BlockState* candidateState = nullptr;

        if (direction == Direction::Down) {
            // 向下看时，放置地板方块
            candidateState = &block().defaultState();
        } else {
            // 水平方向时，放置墙壁方块的默认状态
            // 注意：墙壁方块的朝向需要根据点击面设置
            candidateState = &m_wallBlock->defaultState();
        }

        // 检查状态是否有效
        if (candidateState != nullptr) {
            // 检查位置是否在世界边界内
            if (!world.isWithinWorldBounds(pos)) {
                continue;
            }

            // 调用方块的 isValidPosition 方法检查放置条件
            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            const Block& blockObj = candidateState->owner();
            if (blockObj.isValidPosition(*candidateState, blockReader, pos)) {
                resultState = candidateState;
                break;
            }
        }
    }

    return resultState;
}

} // namespace mc
