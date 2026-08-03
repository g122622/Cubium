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
 * copies or substantial portions of the Software, and to subject to the following
 * notice and other disclaimer in the following conditions:
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

#include "ListJigsawPiece.hpp"

#include "AssemblyTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

std::string ListJigsawPiece::s_typeName = "list_pool_element";

ListJigsawPiece::ListJigsawPiece(JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
{}

std::unique_ptr<JigsawPiece> ListJigsawPiece::clone() const
{
    auto piece = std::make_unique<ListJigsawPiece>(getPlacementBehaviour());
    piece->setGroundLevelDelta(getGroundLevelDelta());
    piece->m_name = m_name;
    for (const auto& joint : m_joints) {
        piece->addJoint(joint);
    }
    // 深拷贝子块（bug 修复 #3：原 ListJigsawPiece::clone 未拷贝 m_joints，此处同时修复）
    for (const auto& child : m_pieces) {
        if (child) {
            piece->addPiece(child->clone());
        }
    }
    return piece;
}

void ListJigsawPiece::addPiece(std::unique_ptr<JigsawPiece> piece)
{
    if (piece) {
        m_pieces.push_back(std::move(piece));
    }
}

void ListJigsawPiece::place(IWorldWriter& world,
    const PlacedPiece& placed,
    class feature::template_::TemplateManager& templateManager,
    math::Random& rng,
    const structure::StructureBoundingBox* bounds,
    world::chunk::ChunkPrimer* chunk,
    IChunkGenerator* generator)
{
    // 对应 MC 1.21 ListPoolElement.place()：递归放置所有子块。
    // 子块继承父块的位置、旋转、镜像和投影类型；每个子块按自身 size 计算边界框。
    for (const auto& child : m_pieces) {
        if (!child) {
            continue;
        }

        // 按子块 size 与父块旋转计算边界框
        BlockPos childSize = child->getSize();
        if (placed.rotation == Rotation::Clockwise90 || placed.rotation == Rotation::CounterClockwise90) {
            childSize = BlockPos(childSize.z, childSize.y, childSize.x);
        }

        structure::StructureBoundingBox childBox;
        if (childSize.x == 0 || childSize.y == 0 || childSize.z == 0) {
            childBox = structure::StructureBoundingBox(placed.position.x,
                placed.position.y,
                placed.position.z,
                placed.position.x,
                placed.position.y,
                placed.position.z);
        } else {
            childBox = structure::StructureBoundingBox(placed.position.x,
                placed.position.y,
                placed.position.z,
                placed.position.x + childSize.x - 1,
                placed.position.y + childSize.y - 1,
                placed.position.z + childSize.z - 1);
        }

        // 子块继承父块的投影类型和 groundLevelDelta
        child->setPlacementBehaviour(placed.projection);
        child->setGroundLevelDelta(placed.groundLevelDelta);

        // 构造子块 PlacedPiece 并递归放置
        PlacedPiece childPlaced(
            child->clone(), placed.position, placed.rotation, placed.mirror, placed.groundLevelDelta, childBox);
        childPlaced.projection = placed.projection;

        // 递归调用子块的 place()（多态分发），透传 chunk 和 generator（FeatureJigsawPiece 子块可能需要）
        child->place(world, childPlaced, templateManager, rng, bounds, chunk, generator);
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
