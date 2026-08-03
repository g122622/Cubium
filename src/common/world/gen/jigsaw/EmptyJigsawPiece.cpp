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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "EmptyJigsawPiece.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

std::string EmptyJigsawPiece::s_typeName = "empty_pool_element";

EmptyJigsawPiece& EmptyJigsawPiece::instance()
{
    static EmptyJigsawPiece instance;
    return instance;
}

std::unique_ptr<JigsawPiece> EmptyJigsawPiece::clone() const
{
    // EmptyJigsawPiece 是单例，clone 返回 nullptr。
    // 调用方（TemplatePool::addPiece 等）需处理 nullptr。
    // 对应 MC 1.21 的 EmptyPoolElement：池中遇到空元素即停止搜索，不克隆。
    return nullptr;
}

void EmptyJigsawPiece::place(IWorldWriter& /*world*/,
    const PlacedPiece& /*placed*/,
    class feature::template_::TemplateManager& /*templateManager*/,
    math::Random& /*rng*/,
    const structure::StructureBoundingBox* /*bounds*/,
    world::chunk::ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    // 空拼图块不放置任何内容
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
