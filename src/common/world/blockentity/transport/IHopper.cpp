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
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"

namespace mc {
namespace blockentity {

// ========== 静态工具方法 ==========

AxisAlignedBB IHopper::getCollectionArea(const IHopper& hopper)
{
    // MC Java: SUCK_AABB = Block.column(16.0, 11.0, 32.0)
    // column(16.0, 16.0, 11.0, 32.0) -> box(0, 11, 0, 16, 32, 16) -> box(0, 11/16, 0, 1, 2, 1)
    // 在方块局部坐标中: Y 从 11/16 (0.6875) 到 2，X/Z 从 0 到 1
    // 使用 getLevelX/Y/Z 转换为世界坐标:
    //   方块漏斗: getLevelX = blockX + 0.5, move 偏移 -0.5, 所以 X 范围 = [blockX, blockX + 1]
    //   同理 Z 范围 = [blockZ, blockZ + 1]
    //   Y: blockY + 0.5 + (0.6875 - 0.5) = blockY + 0.6875 到 blockY + 0.5 + (2 - 0.5) = blockY + 2

    f64 x = hopper.getXPos();
    f64 y = hopper.getYPos();
    f64 z = hopper.getZPos();

    return AxisAlignedBB(static_cast<f32>(x - 0.5),
        static_cast<f32>(y - 0.5 + 11.0 / 16.0), // 碗口顶部 Y = blockY + 0.6875
        static_cast<f32>(z - 0.5),
        static_cast<f32>(x + 0.5),
        static_cast<f32>(y + 1.5), // 上方一格顶部 Y = blockY + 2
        static_cast<f32>(z + 0.5));
}

} // namespace blockentity
} // namespace mc
