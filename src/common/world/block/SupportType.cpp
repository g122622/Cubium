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

#include "SupportType.hpp"

#include "Block.hpp"
#include "BlockState.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

namespace support_type_detail {

// ============================================================================
// 支撑形状常量
// ============================================================================

const VoxelShape& centerSupportShape()
{
    // Block.column(2.0, 0.0, 10.0) = box(7, 0, 7, 9, 10, 9) 像素
    // 转换为方块坐标：box(7/16, 0, 7/16, 9/16, 10/16, 9/16)
    static const VoxelShape shape = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 10.0 / 16.0, 9.0 / 16.0);
    return shape;
}

const VoxelShape& rigidSupportShape()
{
    // RIGID_SUPPORT_SHAPE = Shapes.join(Shapes.block(), Block.column(12.0, 0.0, 16.0), BooleanOp.ONLY_FIRST)
    // 即完整方块减去中心 12×12 像素柱（直径 12、高度 0-16 像素）
    // column(12.0, 0.0, 16.0) = box(2, 0, 2, 14, 16, 14) 像素 = box(2/16, 0, 2/16, 14/16, 1, 14/16) 方块坐标
    // 由于本项目当前未实现形状的差集运算（Shapes::join with ONLY_FIRST），
    // 这里直接构造外环形状：由四个矩形组成的"框"。
    // 外环 = 完整方块 - 中心 (2/16, 0, 2/16)-(14/16, 1, 14/16) 区域
    // 实际上，我们可以使用 Shapes::joinUnoptimized(Shapes::block(), centerColumn, BooleanOps::OnlyFirst())
    // 来获得差集。但为了清晰和性能，直接构造外环。
    //
    // 但是，更简洁的方式是：在 isSupportingRigid 中使用 joinIsNotEmpty 判定，
    // 形式与 isSupportingCenter 一致：!joinIsNotEmpty(faceShape, RIGID_SUPPORT_SHAPE, ONLY_SECOND)。
    // 这里的 RIGID_SUPPORT_SHAPE 实际上需要是"外环"形状。
    //
    // 等价实现：RIGID_SUPPORT_SHAPE = Shapes::join(Shapes::block(), centerColumn(12), ONLY_FIRST)
    // 我们使用 Shapes::joinUnoptimized 来计算差集。
    static const VoxelShape shape = []() {
        const VoxelShape centerColumn = Shapes::box(2.0 / 16.0, 0.0, 2.0 / 16.0, 14.0 / 16.0, 1.0, 14.0 / 16.0);
        return Shapes::join(Shapes::block(), centerColumn, BooleanOps::OnlyFirst());
    }();
    return shape;
}

// ============================================================================
// 支撑判定实现
// ============================================================================

bool isSupportingFull(const BlockState& state, IWorld& /*world*/, const BlockPos& /*pos*/, Direction direction)
{
    // Full：方块面投影必须覆盖整个 1×1 面
    // 对应 MC Java 的 SupportType.FULL: Block.isFaceFull(blockSupportShape, direction)
    // 注意：MC Java 使用 getBlockSupportShape，而 Cubium 的 Block::isFaceFull 使用 getCollisionShape
    // 这里使用 getBlockSupportShape 以与 MC Java 一致
    return Block::isFaceFull(state.getBlockSupportShape(), direction);
}

bool isSupportingCenter(const BlockState& state, IWorld& /*world*/, const BlockPos& /*pos*/, Direction direction)
{
    // Center：方块支撑形状在指定方向的面投影必须包含 CENTER_SUPPORT_SHAPE
    // 等价于 !Shapes.joinIsNotEmpty(faceShape, CENTER_SUPPORT_SHAPE, BooleanOp.ONLY_SECOND)
    const auto& supportShape = state.getBlockSupportShape();
    if (supportShape.isEmpty()) {
        return false;
    }
    // 完整方块快速路径
    if (supportShape.isFullBlock()) {
        return true;
    }
    // 转换为 VoxelShape 并获取面切片
    const VoxelShape supportVoxel = Shapes::fromCollisionShape(supportShape);
    const VoxelShape faceShape = supportVoxel.getFaceShape(direction);
    if (faceShape.isEmpty()) {
        return false;
    }
    // !joinIsNotEmpty(faceShape, CENTER, ONLY_SECOND) = faceShape 完全包含 CENTER
    return !Shapes::joinIsNotEmpty(faceShape, centerSupportShape(), BooleanOps::OnlySecond());
}

bool isSupportingRigid(const BlockState& state, IWorld& /*world*/, const BlockPos& /*pos*/, Direction direction)
{
    // Rigid：方块支撑形状在指定方向的面投影必须包含 RIGID_SUPPORT_SHAPE（外环）
    // 等价于 !Shapes.joinIsNotEmpty(faceShape, RIGID_SUPPORT_SHAPE, BooleanOp.ONLY_SECOND)
    const auto& supportShape = state.getBlockSupportShape();
    if (supportShape.isEmpty()) {
        return false;
    }
    if (supportShape.isFullBlock()) {
        return true;
    }
    const VoxelShape supportVoxel = Shapes::fromCollisionShape(supportShape);
    const VoxelShape faceShape = supportVoxel.getFaceShape(direction);
    if (faceShape.isEmpty()) {
        return false;
    }
    return !Shapes::joinIsNotEmpty(faceShape, rigidSupportShape(), BooleanOps::OnlySecond());
}

} // namespace support_type_detail

// ============================================================================
// SupportType 静态实例
// ============================================================================

const SupportType SupportType::Full(&support_type_detail::isSupportingFull);
const SupportType SupportType::Center(&support_type_detail::isSupportingCenter);
const SupportType SupportType::Rigid(&support_type_detail::isSupportingRigid);

} // namespace mc
