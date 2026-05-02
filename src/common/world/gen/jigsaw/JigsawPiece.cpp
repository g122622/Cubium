#include "JigsawPiece.hpp"
#include "../feature/template/TemplateManager.hpp"
#include "../feature/template/TemplateLoader.hpp"
#include "../../../resource/IResourcePack.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 使用 template_ 命名空间中的类型
using feature::template_::Template;
using feature::template_::TemplateManager;
using feature::template_::TemplateJigsawBlockInfo;

// 静态模板管理器实例（用于加载 Jigsaw 模板）
static TemplateManager s_jigsawTemplateManager;

String EmptyJigsawPiece::s_typeName = "empty_pool_element";
String SingleJigsawPiece::s_typeName = "single_pool_element";
String ListJigsawPiece::s_typeName = "list_pool_element";

EmptyJigsawPiece& EmptyJigsawPiece::instance() {
    static EmptyJigsawPiece instance;
    return instance;
}

std::unique_ptr<JigsawPiece> EmptyJigsawPiece::clone() const {
    // EmptyJigsawPiece is a singleton - return null to indicate empty
    return nullptr;
}

bool JigsawPiece::loadJointsFromTemplate(const String& templateName,
                                          std::vector<JigsawJoint>& joints,
                                          BlockPos& size) {
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

        // TODO: 从方块状态读取 orientation
        // 目前默认为 NorthUp
        joint.orientation = JigsawOrientation::NorthUp;

        joints.push_back(joint);
    }

    return true;
}

SingleJigsawPiece::SingleJigsawPiece(const String& templateName, JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
    , m_templateName(templateName)
{
    // 尝试加载模板并填充连接点
    loadJointsFromTemplate(templateName, m_joints, m_size);
}

ListJigsawPiece::ListJigsawPiece(JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
{
}

std::unique_ptr<JigsawPiece> ListJigsawPiece::clone() const {
    auto piece = std::make_unique<ListJigsawPiece>(getPlacementBehaviour());
    piece->setGroundLevelDelta(getGroundLevelDelta());
    for (const auto& child : m_pieces) {
        if (child) {
            piece->addPiece(child->clone());
        }
    }
    return piece;
}

void ListJigsawPiece::addPiece(std::unique_ptr<JigsawPiece> piece) {
    if (piece) {
        m_pieces.push_back(std::move(piece));
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
