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

#include "RavagerNodeProcessor.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/Block.hpp"

namespace mc::entity::ai::pathfinding {

PathNodeType RavagerNodeProcessor::getNodeType(i32 x, i32 y, i32 z)
{
    // MC 1.16.5 RavagerEntity.Processor.func_215744_a_:
    // 首先检查是否是树叶
    // 如果是树叶，返回 OPEN 类型，让劫掠兽可以穿过树叶
    if (m_region) {
        const BlockState* state = m_region->getBlockState(x, y, z);
        if (state != nullptr && BlockTags::LEAVES().contains(*state)) {
            return PathNodeType::Open;
        }
    }

    // 调用父类方法获取基本类型
    PathNodeType type = WalkNodeProcessor::getNodeType(x, y, z);

    // 如果父类返回 Leaves（虽然目前不会发生），也转换为 Open
    if (type == PathNodeType::Leaves) {
        return PathNodeType::Open;
    }

    return type;
}

PathNodeType RavagerNodeProcessor::getNodeTypeWithEntity(i32 x, i32 y, i32 z)
{
    // MC 1.16.5: 首先检查是否是树叶
    if (m_region) {
        const BlockState* state = m_region->getBlockState(x, y, z);
        if (state != nullptr && BlockTags::LEAVES().contains(*state)) {
            return PathNodeType::Open;
        }
    }

    // 调用父类方法获取基本类型
    PathNodeType type = WalkNodeProcessor::getNodeTypeWithEntity(x, y, z);

    // 如果父类返回 Leaves，也转换为 Open
    if (type == PathNodeType::Leaves) {
        return PathNodeType::Open;
    }

    return type;
}

} // namespace mc::entity::ai::pathfinding
