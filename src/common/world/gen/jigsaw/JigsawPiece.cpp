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

#include "JigsawPiece.hpp"
#include "../../../resource/IResourcePack.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../feature/template/TemplateLoader.hpp"
#include "../feature/template/TemplateManager.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 使用 template_ 命名空间中的类型
using feature::template_::Template;
using feature::template_::TemplateJigsawBlockInfo;
using feature::template_::TemplateManager;

// 静态模板管理器实例（用于加载 Jigsaw 模板）
static TemplateManager s_jigsawTemplateManager;

std::string EmptyJigsawPiece::s_typeName = "empty_pool_element";
std::string SingleJigsawPiece::s_typeName = "single_pool_element";
std::string ListJigsawPiece::s_typeName = "list_pool_element";

EmptyJigsawPiece& EmptyJigsawPiece::instance()
{
    static EmptyJigsawPiece instance;
    return instance;
}

std::unique_ptr<JigsawPiece> EmptyJigsawPiece::clone() const
{
    // MC 1.16.5: EmptyJigsawPiece 返回自身的克隆（单例模式，但仍需返回有效指针）
    // 参考: EmptyJigsawPiece.java - INSTANCE 单例，但在 JigsawPattern 中仍需有效指针
    return std::make_unique<EmptyJigsawPiece>();
}

bool JigsawPiece::loadJointsFromTemplate(
    const std::string& templateName, std::vector<JigsawJoint>& joints, BlockPos& size)
{
    ResourceLocation loc(templateName);
    const Template* templ = s_jigsawTemplateManager.getTemplate(loc);

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

        // 从方块状态读取 orientation
        // 参考 MC 1.16.5: JigsawBlock.getOrientation(state)
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

    // MC 1.16.5: 连接点不需要打乱顺序
    // 参考: SingleJigsawPiece.getJigsawBlocks() 返回的是模板中的原始顺序
    // 打乱是在 JigsawManager 中选择模板池中的块时进行的 (getShuffledPieces)
    // 而不是在加载连接点时进行的

    return true;
}

SingleJigsawPiece::SingleJigsawPiece(const std::string& templateName, JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
    , m_templateName(templateName)
{
    // 尝试加载模板并填充连接点
    loadJointsFromTemplate(templateName, m_joints, m_size);
}

ListJigsawPiece::ListJigsawPiece(JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
{}

std::unique_ptr<JigsawPiece> ListJigsawPiece::clone() const
{
    auto piece = std::make_unique<ListJigsawPiece>(getPlacementBehaviour());
    piece->setGroundLevelDelta(getGroundLevelDelta());
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

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
