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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 空拼图块
 *
 * 不放置任何内容，用作模板池的终止元素或占位符。
 * 单例模式，clone() 返回 nullptr（调用方需处理）。
 */
class EmptyJigsawPiece : public JigsawPiece {
public:
    static EmptyJigsawPiece& instance();

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override;
    bool isEmpty() const override { return true; }
    BlockPos getSize() const override { return BlockPos(0, 0, 0); }

    void place(IWorldWriter& world,
        const PlacedPiece& placed,
        class feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    EmptyJigsawPiece()
        : JigsawPiece(JigsawPlacementBehaviour::Rigid)
    {}

private:
    static std::string s_typeName;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
