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

BlockPos JigsawTransform::transformPosition(const BlockPos& pos, Rotation rotation, Mirror mirror)
{
    // 对应 MC 1.21 StructureTemplate.transform(BlockPos, Mirror, Rotation, pivot)：
    // **以模板原点角为轴**（pivot 恒为 BlockPos.ZERO），镜像先取负、再旋转：
    //   CCW90 → (z, y, -x)     CW90 → (-z, y, x)     CW180 → (-x, y, -z)
    // 旋转后坐标可能为负——这是原版语义（包围盒由两个对角点归一化得到），
    // 与 Template::transformBlockPos 的方块放置变换必须保持同一约定，
    // 否则"构件包围盒"描述的就不是方块实际落点。
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

    switch (rotation) {
        case Rotation::CounterClockwise90:
            return BlockPos(k, j, -i);
        case Rotation::Clockwise90:
            return BlockPos(-k, j, i);
        case Rotation::Clockwise180:
            return BlockPos(-i, j, -k);
        default:
            return mirrored ? BlockPos(i, j, k) : pos;
    }
}

std::vector<JigsawJoint> JigsawTransform::getTransformedJoints(
    const JigsawPiece& piece, const BlockPos& position, Rotation rotation, Mirror mirror)
{
    std::vector<JigsawJoint> transformed;
    transformed.reserve(piece.getJoints().size());

    for (const auto& joint : piece.getJoints()) {
        JigsawJoint transformedJoint;
        transformedJoint.sourcePos = transformPosition(joint.sourcePos, rotation, mirror) + position;
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

    const BlockPos cornerA = transformPosition(BlockPos(0, 0, 0), rotation, Mirror::None);
    const BlockPos cornerB = transformPosition(BlockPos(size.x - 1, size.y - 1, size.z - 1), rotation, Mirror::None);

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
