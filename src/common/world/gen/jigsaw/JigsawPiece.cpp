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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"

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

std::vector<JigsawJoint> JigsawPiece::getShuffledJoints(math::Random& rng) const
{
    std::vector<JigsawJoint> shuffled = m_joints;
    rng.shuffle(shuffled);

    // 按 selectionPriority 降序稳定排序（高优先级先处理），对应 MC 1.21 的
    // SinglePoolElement.getShuffledJigsawBlocks()：先 shuffle 再按 selectionPriority 稳定排序。
    std::stable_sort(shuffled.begin(), shuffled.end(), [](const JigsawJoint& a, const JigsawJoint& b) {
        return a.selectionPriority > b.selectionPriority;
    });

    return shuffled;
}

bool JigsawPiece::loadJointsFromTemplate(
    const std::string& templateName, std::vector<JigsawJoint>& joints, BlockPos& size)
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
