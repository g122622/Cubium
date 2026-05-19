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
