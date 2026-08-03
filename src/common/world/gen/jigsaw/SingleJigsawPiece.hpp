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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 单模板拼图块
 *
 * 对应 MC 1.21 的 SinglePoolElement。放置时加载结构模板并应用处理器链：
 * 1. BlockIgnoreStructureProcessor（standard 忽略 STRUCTURE_BLOCK，legacy 忽略 STRUCTURE_AND_AIR）
 * 2. JigsawReplacementStructureProcessor（替换 jigsaw 方块为 final_state）
 * 3. Piece 自带处理器列表（从 ProcessorListRegistry 查找）
 * 4. GravityStructureProcessor（仅 terrain_matching 投影）
 */
class SingleJigsawPiece : public JigsawPiece {
public:
    explicit SingleJigsawPiece(const std::string& templateName,
        JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid,
        const std::optional<ResourceLocation>& processorListId = std::nullopt);

    const std::string& getTypeName() const override { return s_typeName; }
    const std::string& getTemplateName() const { return m_templateName; }

    /**
     * @brief 获取处理器列表资源位置
     *
     * 模板池元素可通过 processors 字段引用一个已注册的处理器列表，
     * 如 "minecraft:mossify_10_percent"、"minecraft:street_plains" 等。
     */
    const std::optional<ResourceLocation>& getProcessorListId() const { return m_processorListId; }
    void setProcessorListId(const ResourceLocation& id) { m_processorListId = id; }
    bool hasProcessors() const { return m_processorListId.has_value(); }

    std::unique_ptr<JigsawPiece> clone() const override
    {
        auto piece = std::make_unique<SingleJigsawPiece>(m_templateName, getPlacementBehaviour(), m_processorListId);
        piece->setGroundLevelDelta(getGroundLevelDelta());
        for (const auto& joint : m_joints) {
            piece->addJoint(joint);
        }
        return piece;
    }

    BlockPos getSize() const override { return m_size; }
    void setSize(const BlockPos& size) { m_size = size; }

    void place(IWorldWriter& world,
        const PlacedPiece& placed,
        class feature::template_::TemplateManager& templateManager,
        math::Random& rng,
        const structure::StructureBoundingBox* bounds,
        world::chunk::ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

protected:
    std::string m_templateName;
    BlockPos m_size;
    std::optional<ResourceLocation> m_processorListId;
    static std::string s_typeName;
};

/**
 * @brief Legacy 单模板拼图块
 *
 * 与 SingleJigsawPiece 的区别：放置时使用 BlockIgnoreProcessor.STRUCTURE_AND_AIR
 * （忽略结构方块和空气方块），而标准 SingleJigsawPiece 只忽略 STRUCTURE_BLOCK。
 * 这是因为旧版结构模板中空气方块是显式放置的，legacy 模式需要忽略它们
 * 以避免覆盖已有地形。
 */
class LegacySingleJigsawPiece : public SingleJigsawPiece {
public:
    explicit LegacySingleJigsawPiece(const std::string& templateName,
        JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid,
        const std::optional<ResourceLocation>& processorListId = std::nullopt);

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override
    {
        auto piece =
            std::make_unique<LegacySingleJigsawPiece>(m_templateName, getPlacementBehaviour(), m_processorListId);
        piece->setGroundLevelDelta(getGroundLevelDelta());
        for (const auto& joint : m_joints) {
            piece->addJoint(joint);
        }
        return piece;
    }
    bool isLegacy() const override { return true; }

private:
    static std::string s_typeName;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
