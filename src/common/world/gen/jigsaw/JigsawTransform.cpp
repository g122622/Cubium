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
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

BlockPos JigsawTransform::transformPosition(
    const BlockPos& pos, Rotation rotation, Mirror mirror, const BlockPos& templateSize)
{
    BlockPos result = pos;

    // 先应用镜像（相对于模板中心）
    switch (mirror) {
        case Mirror::FrontBack: // X 轴镜像
            result = BlockPos(templateSize.x - 1 - result.x, result.y, result.z);
            break;
        case Mirror::LeftRight: // Z 轴镜像
            result = BlockPos(result.x, result.y, templateSize.z - 1 - result.z);
            break;
        default:
            break;
    }

    // 然后应用旋转
    switch (rotation) {
        case Rotation::Clockwise90:
            result = BlockPos(templateSize.z - 1 - result.z, result.y, result.x);
            break;
        case Rotation::Clockwise180:
            result = BlockPos(templateSize.x - 1 - result.x, result.y, templateSize.z - 1 - result.z);
            break;
        case Rotation::CounterClockwise90:
            result = BlockPos(result.z, result.y, templateSize.x - 1 - result.x);
            break;
        default:
            break;
    }

    return result;
}

std::vector<JigsawJoint> JigsawTransform::getTransformedJoints(
    const JigsawPiece& piece, const BlockPos& position, Rotation rotation, Mirror mirror)
{
    std::vector<JigsawJoint> transformed;
    transformed.reserve(piece.getJoints().size());

    BlockPos size = piece.getSize();

    for (const auto& joint : piece.getJoints()) {
        JigsawJoint transformedJoint;
        transformedJoint.sourcePos = transformPosition(joint.sourcePos, rotation, mirror, size) + position;
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
    BlockPos size = piece.getSize();

    // 根据旋转调整尺寸
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        size = BlockPos(size.z, size.y, size.x);
    }

    if (size.x == 0 || size.y == 0 || size.z == 0) {
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    return structure::StructureBoundingBox(
        pos.x, pos.y, pos.z, pos.x + size.x - 1, pos.y + size.y - 1, pos.z + size.z - 1);
}

Rotation JigsawTransform::getRandomRotation(math::Random& rng)
{
    return static_cast<Rotation>(rng.nextInt(4));
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
