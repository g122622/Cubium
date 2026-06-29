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

#include "AssemblyTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"

#include <vector>

namespace mc {

class IWorldWriter;
class IChunkGenerator;

namespace world {
namespace chunk {
class ChunkPrimer;
}

namespace gen {
namespace structure {
class StructureBoundingBox;
}

namespace jigsaw {

/**
 * @brief 拼图块放置器
 *
 * 负责将组装完成的 PlacedPiece 写入世界。遍历 PlacedPiece 调用 piece->place()（多态分发），
 * 各子类在 place() 中构造处理器链并调用 Template::place()。
 *
 * 对应 MC 1.21 中由 JigsawManager 拆分出的放置职责。
 */
class JigsawPlacer {
public:
    /**
     * @brief 放置已组装的拼图块列表到世界
     *
     * 遍历所有 PlacedPiece，对每个调用 piece->place()（多态分发）。
     *
     * @param world 世界写入器
     * @param placedPieces 已组装的拼图块列表
     * @param rng 随机数生成器
     * @param bounds 方块放置裁剪边界；无需裁剪时传入 nullptr
     * @param chunk 区块数据（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     * @param generator 区块生成器（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     */
    static void placePieces(IWorldWriter& world,
        std::vector<PlacedPiece>& placedPieces,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr);

    /**
     * @brief 放置单个已组装的拼图块到世界
     *
     * 通过 virtual place() 多态分发到具体子类（SingleJigsawPiece/ListJigsawPiece/
     * FeatureJigsawPiece/EmptyJigsawPiece）。对应原 JigsawManager::placePieceRecursive 的单块放置入口。
     *
     * @param world 世界写入器
     * @param placed 已放置的拼图块信息
     * @param rng 随机数生成器
     * @param bounds 方块放置裁剪边界；无需裁剪时传入 nullptr
     * @param chunk 区块数据（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     * @param generator 区块生成器（FeatureJigsawPiece 放置配置化地物时需要，可为 nullptr）
     */
    static void placePiece(IWorldWriter& world,
        const PlacedPiece& placed,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr);

    /**
     * @brief 放置回退方块（当模板未找到时）
     *
     * 在边界框边缘放置石砖框架标记结构位置。
     *
     * @param world 世界写入器
     * @param placed 已放置的拼图块信息
     * @param rng 随机数生成器
     * @param bounds 方块放置裁剪边界；无需裁剪时传入 nullptr
     */
    static void placeFallbackBlocks(IWorldWriter& world,
        const PlacedPiece& placed,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds);
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
