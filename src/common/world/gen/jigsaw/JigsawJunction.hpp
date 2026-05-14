#pragma once

#include "../../../core/Types.hpp"
#include "JigsawPiece.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

class JigsawJunction {
public:
    JigsawJunction(i32 sourceX, i32 sourceGroundY, i32 sourceZ, i32 deltaY, JigsawPlacementBehaviour destProjection)
        : m_sourceX(sourceX)
        , m_sourceGroundY(sourceGroundY)
        , m_sourceZ(sourceZ)
        , m_deltaY(deltaY)
        , m_destProjection(destProjection)
    {}

    i32 getSourceX() const { return m_sourceX; }
    i32 getSourceGroundY() const { return m_sourceGroundY; }
    i32 getSourceZ() const { return m_sourceZ; }
    i32 getDeltaY() const { return m_deltaY; }
    JigsawPlacementBehaviour getDestProjection() const { return m_destProjection; }

    bool operator==(const JigsawJunction& other) const
    {
        return m_sourceX == other.m_sourceX && m_sourceZ == other.m_sourceZ && m_deltaY == other.m_deltaY &&
            m_destProjection == other.m_destProjection;
    }

    bool operator!=(const JigsawJunction& other) const { return !(*this == other); }

private:
    i32 m_sourceX;
    i32 m_sourceGroundY;
    i32 m_sourceZ;
    i32 m_deltaY;
    JigsawPlacementBehaviour m_destProjection;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
