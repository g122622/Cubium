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

#pragma once

#include "JigsawPiece.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 列表拼图块（包含多个子块）
 *
 * 对应 MC 1.21 的 ListPoolElement。放置时递归放置所有子块，
 * 子块继承父块的位置、旋转和镜像。
 */
class ListJigsawPiece : public JigsawPiece {
public:
    explicit ListJigsawPiece(JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;

    void addPiece(std::unique_ptr<JigsawPiece> piece);
    const std::vector<std::unique_ptr<JigsawPiece>>& getPieces() const { return m_pieces; }
    size_t getPieceCount() const { return m_pieces.size(); }

    void place(IWorldWriter& world,
        const PlacedPiece& placed,
        class feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    std::vector<std::unique_ptr<JigsawPiece>> m_pieces;
    static std::string s_typeName;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
