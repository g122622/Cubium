/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "JigsawTransform.hpp"

#include "JigsawPiece.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include <algorithm>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

BlockPos JigsawTransform::transformPosition(
    const BlockPos& pos, Rotation rotation, Mirror mirror, const BlockPos& pivot)
{
    // 对应 MC 1.21 StructureTemplate.transform(BlockPos, Mirror, Rotation, pivot)：
    // 镜像先取负、再绕 pivot 旋转：
    //   CCW90 → (l - i1 + k, j, l + i1 - i)   CW90 → (l + i1 - k, j, i1 - l + i)
    //   CW180 → (2l - i, j, 2i1 - k)
    // 其中 (l, i1) = (pivot.x, pivot.z)；本项目两处调用（包围盒与连接点）都传原点。
    // 旋转后坐标可能为负——这是原版语义，与 Template::transformBlockPos 的方块放置变换
    // 必须保持同一约定，否则"构件包围盒"描述的就不是方块实际落点。
    i32 i = pos.x;
    i32 j = pos.y;
    i32 k = pos.z;

    bool mirrored = true;
    switch (mirror) {
        case Mirror::LeftRight: // Z 轴镜像
            k = -k;
            break;
        case Mirror::FrontBack: // X 轴镜像
            i = -i;
            break;
        default:
            mirrored = false;
            break;
    }

    const i32 l = pivot.x;
    const i32 i1 = pivot.z;
    switch (rotation) {
        case Rotation::CounterClockwise90:
            return BlockPos(l - i1 + k, j, l + i1 - i);
        case Rotation::Clockwise90:
            return BlockPos(l + i1 - k, j, i1 - l + i);
        case Rotation::Clockwise180:
            return BlockPos(l + l - i, j, i1 + i1 - k);
        default:
            return mirrored ? BlockPos(i, j, k) : pos;
    }
}

std::vector<JigsawJoint> JigsawTransform::getTransformedJoints(
    const JigsawPiece& piece, const BlockPos& position, Rotation rotation, Mirror mirror)
{
    std::vector<JigsawJoint> transformed;
    transformed.reserve(piece.getJoints().size());

    // 连接点绕**模板原点角**旋转（枢轴为零），与原版 StructureTemplate.transform 的
    // 方块坐标约定一致；包围盒也走同一枢轴，故位置变换在整条链路上只有一种约定。
    const BlockPos origin(0, 0, 0);

    for (const auto& joint : piece.getJoints()) {
        JigsawJoint transformedJoint;
        transformedJoint.sourcePos = transformPosition(joint.sourcePos, rotation, mirror, origin) + position;
        transformedJoint.sourceName = joint.sourceName;
        transformedJoint.targetPool = joint.targetPool;
        transformedJoint.targetName = joint.targetName;
        transformedJoint.projection = joint.projection;
        transformedJoint.jointType = joint.jointType;

        // 变换 Jigsaw 朝向
        transformedJoint.orientation = JigsawOrientations::rotate(joint.orientation, rotation);
        if (mirror != Mirror::None) {
            transformedJoint.orientation = JigsawOrientations::mirror(transformedJoint.orientation, mirror);
        }

        transformedJoint.sourceGroundY = joint.sourceGroundY;
        transformed.push_back(transformedJoint);
    }

    return transformed;
}

structure::StructureBoundingBox JigsawTransform::calculateBoundingBox(
    const JigsawPiece& piece, const BlockPos& pos, Rotation rotation)
{
    // 对应 MC StructureTemplate.getBoundingBox(pos, rot, pivot=ZERO, mirror)：
    // 把模板的两个对角点（0 与 size-1）各自变换后归一化，再整体平移到 pos。
    // 变换以原点角为轴，故旋转后坐标可为负 —— 不能用"旋转后尺寸直接铺在 pos 上"代替。
    const BlockPos size = piece.getSize();
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) {
        // 退化模板（地物池元素的 getSize 为 Vec3i.ZERO）：没有实体范围，包围盒退化到 pos
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    const BlockPos cornerA = transformPosition(BlockPos(0, 0, 0), rotation, Mirror::None, BlockPos(0, 0, 0));
    const BlockPos cornerB =
        transformPosition(BlockPos(size.x - 1, size.y - 1, size.z - 1), rotation, Mirror::None, BlockPos(0, 0, 0));

    return structure::StructureBoundingBox(pos.x + std::min(cornerA.x, cornerB.x),
        pos.y + std::min(cornerA.y, cornerB.y),
        pos.z + std::min(cornerA.z, cornerB.z),
        pos.x + std::max(cornerA.x, cornerB.x),
        pos.y + std::max(cornerA.y, cornerB.y),
        pos.z + std::max(cornerA.z, cornerB.z));
}

Rotation JigsawTransform::getRandomRotation(math::IRandom& rng)
{
    return static_cast<Rotation>(rng.nextInt(4));
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
