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

#include "JigsawPiece.hpp"

#include "JigsawAssembler.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/JigsawOrientation.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawTransform.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

using feature::template_::Template;
using feature::template_::TemplateJigsawBlockInfo;
using feature::template_::TemplateManager;

structure::StructureBoundingBox JigsawPiece::getBoundingBox(const BlockPos& pos, Rotation rotation) const
{
    // 对应 MC StructureTemplate.getBoundingBox(pos, rot, pivot=ZERO, mirror)：
    // 把模板的两个对角点（0 与 size-1）各自变换后归一化，再整体平移到 pos。
    // 变换以原点角为轴，故旋转后坐标可为负 —— 不能用"旋转后尺寸直接铺在 pos 上"代替。
    const BlockPos size = getSize();
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) {
        // 退化模板（地物池元素的 getSize 为 Vec3i.ZERO）：没有实体范围，包围盒退化到 pos
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    const BlockPos cornerA =
        JigsawTransform::transformPosition(BlockPos(0, 0, 0), rotation, Mirror::None, BlockPos(0, 0, 0));
    const BlockPos cornerB = JigsawTransform::transformPosition(
        BlockPos(size.x - 1, size.y - 1, size.z - 1), rotation, Mirror::None, BlockPos(0, 0, 0));

    return structure::StructureBoundingBox(pos.x + std::min(cornerA.x, cornerB.x),
        pos.y + std::min(cornerA.y, cornerB.y),
        pos.z + std::min(cornerA.z, cornerB.z),
        pos.x + std::max(cornerA.x, cornerB.x),
        pos.y + std::max(cornerA.y, cornerB.y),
        pos.z + std::max(cornerA.z, cornerB.z));
}

std::vector<JigsawJoint> JigsawPiece::getShuffledJoints(math::IRandom& rng) const
{
    // 经 getJoints() 而非直接读 m_joints：SingleJigsawPiece 的连接点在首次访问时才解析
    std::vector<JigsawJoint> shuffled = getJoints();
    rng.shuffle(shuffled);

    // 按 selectionPriority 降序稳定排序（高优先级先处理），对应 MC 1.21 的
    // SinglePoolElement.getShuffledJigsawBlocks()：先 shuffle 再按 selectionPriority 稳定排序。
    std::stable_sort(shuffled.begin(), shuffled.end(), [](const JigsawJoint& a, const JigsawJoint& b) {
        return a.selectionPriority > b.selectionPriority;
    });

    return shuffled;
}

void JigsawPiece::getShuffledJointPointers(math::IRandom& rng, std::vector<const JigsawJoint*>& shuffled) const
{
    const auto& joints = getJoints();
    shuffled.clear();
    if (shuffled.capacity() < joints.size()) {
        shuffled.reserve(joints.size());
    }
    for (const JigsawJoint& joint : joints) {
        shuffled.push_back(&joint);
    }
    rng.shuffle(shuffled);

    std::stable_sort(shuffled.begin(), shuffled.end(), [](const JigsawJoint* a, const JigsawJoint* b) {
        return a->selectionPriority > b->selectionPriority;
    });
}

bool JigsawPiece::loadJointsFromTemplate(
    const std::string& templateName, std::vector<JigsawJoint>& joints, BlockPos& size) const
{
    ResourceLocation loc(templateName);
    // 通过 JigsawAssembler 的静态 TemplateManager 访问点加载模板（确保数据包集成）
    const Template* templ = JigsawAssembler::getTemplateManager().getTemplate(loc);

    if (!templ) {
        return false;
    }

    // 获取模板大小
    size = templ->getSize();

    // 获取所有 Jigsaw 方块信息
    const auto& jigsawBlocks = templ->getJigsawBlocks();
    joints.clear();
    joints.reserve(jigsawBlocks.size());

    for (const auto& jigsawInfo : jigsawBlocks) {
        JigsawJoint joint;
        joint.sourcePos = jigsawInfo.pos;
        joint.sourceName = jigsawInfo.name;
        joint.targetPool = jigsawInfo.targetPool;
        joint.targetName = jigsawInfo.targetName;
        joint.jointType = static_cast<JigsawJointType>(jigsawInfo.jointType);
        joint.projection = getPlacementBehaviour();
        joint.sourceGroundY = jigsawInfo.pos.y; // 源地面高度 = 连接点 Y 坐标
        joint.placementPriority = jigsawInfo.placementPriority;
        joint.selectionPriority = jigsawInfo.selectionPriority;

        // 从方块状态读取 orientation
        joint.orientation = JigsawOrientation::NorthUp; // 默认值

        if (jigsawInfo.blockStateId != 0) {
            const BlockState* state = BlockRegistry::instance().getBlockState(jigsawInfo.blockStateId);
            if (state != nullptr) {
                // 读取 orientation 属性
                joint.orientation = state->get(BlockStateProperties::ORIENTATION());
            }
        }

        joints.push_back(joint);
    }

    return true;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
