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

#include "RedstoneHelper.hpp"

#include "../../util/assert/AssertMacros.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"

namespace mc {
namespace world {
namespace redstone {

bool RedstoneHelper::isNormalCube(const BlockState& state)
{
    // 实体方块：固体、不透明、非空气
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstoneHelper::canConnectRedstone(const BlockState& state)
{
    // 检查方块是否可以输出红石信号
    return state.getBlock().canProvidePower(state);
}

bool RedstoneHelper::isRedstoneConductor(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 红石导体检查：
    // 1. 必须是实体方块
    // 2. 不阻挡红石信号传输

    if (!isNormalCube(state)) {
        return false;
    }

    // 某些特殊方块虽然不是红石导体（如观察者、红石块等）
    // 这些需要单独检查

    // 默认情况下，实体方块都是红石导体
    return true;
}

i32 RedstoneHelper::getEntitySignal(IWorld& world, const BlockPos& pos)
{
    // TODO: 检查位置上的实体是否输出红石信号
    // 例如：装有漏斗的矿车、动力矿车等
    // 待实体系统完善后实现
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 0;
}

} // namespace redstone
} // namespace world
} // namespace mc
