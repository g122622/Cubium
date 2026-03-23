#include "JigsawPiece.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

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

SingleJigsawPiece::SingleJigsawPiece(const String& templateName, JigsawPlacementBehaviour behaviour)
    : JigsawPiece(behaviour)
    , m_templateName(templateName)
{
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
