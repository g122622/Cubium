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

#include "IHopper.hpp"

namespace mc {
namespace blockentity {

// ========== 静态工具方法 ==========

AxisAlignedBB IHopper::getCollectionArea(const IHopper& hopper)
{
    // 收集区域包括两部分:
    // 1. 漏斗内部碗状区域 (2, 11, 2) -> (14, 16, 14)
    // 2. 上方一格方块区域 (0, 16, 0) -> (16, 32, 16)
    //
    // 在世界坐标中，相对于漏斗位置
    f64 x = hopper.getXPos();
    f64 y = hopper.getYPos();
    f64 z = hopper.getZPos();

    // 合并两个区域的AABB
    // 碗状区域: (x-0.5+2/16, y-0.5+11/16, z-0.5+2/16) -> (x-0.5+14/16, y-0.5+1, z-0.5+14/16)
    // 上方区域: (x-0.5, y-0.5+1, z-0.5) -> (x+0.5, y-0.5+2, z+0.5)
    //
    // 简化：使用合并后的区域
    // X: [x - 0.5 + 2/16, x + 0.5 + 2/16] = [x - 6/16, x + 10/16]
    // 但实际上应该覆盖:
    // - 碗内部: (2/16, 11/16, 2/16) 到 (14/16, 1, 14/16)
    // - 上方: (0, 1, 0) 到 (1, 2, 1)
    //
    // Minecraft的COLLECTION_AREA使用VoxelShapes.or()合并两个形状
    // 我们这里返回合并后的简化AABB

    // 使用最简单的实现：上方一格完整区域
    // 因为碗状区域在方块内部，物品主要从上方进入
    return AxisAlignedBB(static_cast<f32>(x - 0.5),
        static_cast<f32>(y - 0.5 + 11.0 / 16.0), // 碗底部
        static_cast<f32>(z - 0.5),
        static_cast<f32>(x + 0.5),
        static_cast<f32>(y + 1.5), // 上方一格顶部
        static_cast<f32>(z + 0.5));
}

} // namespace blockentity
} // namespace mc
