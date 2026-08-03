/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "CollisionShape.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"

namespace mc {

bool CollisionShape::coversFullBlock() const
{
    // 快速路径：完整方块和空形状
    if (isFullBlock()) {
        return true;
    }
    if (isEmpty()) {
        return false;
    }

    // 使用 VoxelShape 布尔运算精确判断，与 MC Java 的 Block.isShapeFullBlock 一致：
    // isShapeFullBlock(shape) = !Shapes.joinIsNotEmpty(Shapes.block(), shape, BooleanOp.NOT_SAME)
    //
    // NOT_SAME 操作 (a != b) 检查两个形状是否存在差异体素：
    // - 如果 joinIsNotEmpty 返回 true，说明存在差异体素，形状不覆盖完整方块
    // - 如果 joinIsNotEmpty 返回 false，说明两个形状完全一致，形状覆盖完整方块
    const VoxelShape voxelShape = Shapes::fromCollisionShape(*this);
    return !Shapes::joinIsNotEmpty(Shapes::block(), voxelShape, BooleanOps::NotSame());
}

} // namespace mc
