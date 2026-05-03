#include "JigsawPiece.hpp"
#include "../feature/template/TemplateManager.hpp"
#include "../feature/template/TemplateLoader.hpp"
#include "../../../resource/IResourcePack.hpp"
#include "../../block/BlockRegistry.hpp"

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
    // MC 1.16.5: EmptyJigsawPiece 返回自身的克隆（单例模式，但仍需返回有效指针）
    // 参考: EmptyJigsawPiece.java - INSTANCE 单例，但在 JigsawPattern 中仍需有效指针
    return std::make_unique<EmptyJigsawPiece>();
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

        // 从方块状态读取 orientation
        // 方块状态ID已存储在 jigsawInfo.blockStateId 中
        // 需要通过 BlockRegistry 获取 BlockState 并读取 ORIENTATION 属性
        // 当前限制：JigsawBlock 未注册 ORIENTATION 属性，使用默认值
        // 参考 MC 1.16.5: JigsawBlock.getOrientation(state)
        joint.orientation = JigsawOrientation::NorthUp;

        // TODO: 当 JigsawBlock 注册 ORIENTATION 属性后，启用以下代码
        // if (jigsawInfo.blockStateId != 0) {
        //     const BlockState* state = BlockRegistry::instance().getBlockState(jigsawInfo.blockStateId);
        //     if (state != nullptr) {
        //         // 读取 orientation 属性
        //         // joint.orientation = state->get(BlockStateProperties::ORIENTATION());
        //     }
        // }

        joints.push_back(joint);
    }

    // MC 1.16.5: 连接点不需要打乱顺序
    // 参考: SingleJigsawPiece.getJigsawBlocks() 返回的是模板中的原始顺序
    // 打乱是在 JigsawManager 中选择模板池中的块时进行的 (getShuffledPieces)
    // 而不是在加载连接点时进行的

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
