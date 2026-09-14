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
#include "JigsawTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 地物拼图块
 *
 * 对应 MC 1.21 的 FeaturePoolElement。在拼图结构中放置一个配置化地物（如树木、仙人掌、干草堆等）。
 * 不应用方块处理器和重力处理器，直接在地物位置调用 ConfiguredFeature::place()。
 *
 * FeatureJigsawPiece 不加载结构模板，getSize() 返回 (0,0,0)，
 * 默认携带一个向下的 minecraft:bottom 连接点（对应 MC 的 FeaturePoolElement.getShuffledJigsawBlocks()）。
 */
class FeatureJigsawPiece : public JigsawPiece {
public:
    explicit FeatureJigsawPiece(
        const std::string& featureId, JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid);

    const std::string& getTypeName() const override { return s_typeName; }
    const std::string& getFeatureId() const { return m_featureId; }
    std::unique_ptr<JigsawPiece> clone() const override
    {
        auto piece = std::make_unique<FeatureJigsawPiece>(m_featureId, getPlacementBehaviour());
        piece->setGroundLevelDelta(getGroundLevelDelta());
        for (const auto& joint : m_joints) {
            piece->addJoint(joint);
        }
        return piece;
    }
    bool isEmpty() const override { return false; }
    BlockPos getSize() const override { return BlockPos(0, 0, 0); }

    void place(IWorldWriter& world,
        const PlacedPiece& placed,
        class feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    std::string m_featureId;
    static std::string s_typeName;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
