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

#include "SingleJigsawPiece.hpp"

#include "JigsawPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/AssemblyTypes.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/ProcessorListRegistry.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

using feature::template_::BlockIgnoreStructureProcessor;
using feature::template_::GravityStructureProcessor;
using feature::template_::JigsawReplacementStructureProcessor;
using feature::template_::PlacementSettings;
using feature::template_::StructureProcessorList;
using feature::template_::Template;
using feature::template_::TemplateManager;

std::string SingleJigsawPiece::s_typeName = "single_pool_element";

std::string LegacySingleJigsawPiece::s_typeName = "legacy_single_pool_element";

SingleJigsawPiece::SingleJigsawPiece(const std::string& templateName,
    JigsawPlacementBehaviour behaviour,
    const std::optional<ResourceLocation>& processorListId)
    : JigsawPiece(behaviour)
    , m_templateName(templateName)
    , m_processorListId(processorListId)
{
    // 尝试加载模板并填充连接点
    loadJointsFromTemplate(templateName, m_joints, m_size);
}

LegacySingleJigsawPiece::LegacySingleJigsawPiece(const std::string& templateName,
    JigsawPlacementBehaviour behaviour,
    const std::optional<ResourceLocation>& processorListId)
    : SingleJigsawPiece(templateName, behaviour, processorListId)
{}

/**
 * @brief 获取 Structure Block 的方块状态 ID 列表（用于 BlockIgnore 处理器）
 *
 * BlockIgnoreStructureProcessor::process 比较 blockStateId，因此必须返回 stateId 而非 blockId。
 * 延迟初始化，因为 BlockRegistry 可能尚未就绪。
 */
static std::vector<u32> getStructureBlockStateIds()
{
    static std::vector<u32> ids;
    if (ids.empty()) {
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            ids.push_back(state->stateId());
        }
    }
    return ids;
}

/**
 * @brief 获取 Structure Block + Air 的方块状态 ID 列表（用于 Legacy BlockIgnore 处理器）
 *
 * Legacy 拼图块使用此列表，因为旧版结构模板中空气方块是显式放置的，
 * 需要忽略它们以避免覆盖已有地形。
 */
static std::vector<u32> getStructureAndAirStateIds()
{
    static std::vector<u32> ids;
    if (ids.empty()) {
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            ids.push_back(state->stateId());
        }
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::AIR)) {
            ids.push_back(state->stateId());
        }
    }
    return ids;
}

void SingleJigsawPiece::place(IWorldWriter& world,
    const PlacedPiece& placed,
    TemplateManager& templateManager,
    math::Random& rng,
    const structure::StructureBoundingBox* bounds,
    world::chunk::ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (m_templateName.empty()) {
        return;
    }

    ResourceLocation templateLoc(m_templateName);
    const Template* templ = templateManager.getTemplate(templateLoc);

    if (!templ) {
        // 模板未找到，由 JigsawPlacer 的回退逻辑处理
        JigsawPlacer::placeFallbackBlocks(world, placed, rng, bounds);
        return;
    }

    // 创建放置设置
    PlacementSettings settings;
    settings.setRotation(placed.rotation);
    settings.setMirror(placed.mirror);
    settings.setBoundingBox(bounds);

    // 构建处理器链（按 MC 1.21 SinglePoolElement.getSettings() 顺序）：
    // 1. BlockIgnoreStructureProcessor — legacy 用 STRUCTURE_AND_AIR，standard 用 STRUCTURE_BLOCK
    // 2. JigsawReplacementStructureProcessor — 替换 jigsaw 方块为 final_state
    // 3. Piece 自带 processor list — 从 ProcessorListRegistry 查找
    // 4. GravityStructureProcessor — terrain_matching 投影时添加
    StructureProcessorList processorList;

    // 1. BlockIgnore: legacy 忽略 structure_block + air，standard 只忽略 structure_block
    if (isLegacy()) {
        processorList.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(getStructureAndAirStateIds()));
    } else {
        processorList.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(getStructureBlockStateIds()));
    }

    // 2. JigsawReplacement: 替换 jigsaw 方块为 final_state
    processorList.addProcessor(std::make_unique<JigsawReplacementStructureProcessor>());

    // 3. Piece 自带处理器列表
    if (hasProcessors()) {
        const auto* pieceProcessors = ProcessorListRegistry::instance().getList(*m_processorListId);
        if (pieceProcessors) {
            for (const auto& proc : pieceProcessors->getProcessors()) {
                if (proc) {
                    processorList.addProcessor(proc->clone());
                }
            }
        } else {
            spdlog::warn("Processor list '{}' not found in registry", m_processorListId->toString());
        }
    }

    // 4. 投影处理器（terrain_matching → GravityStructureProcessor）
    // 对应 MC 1.21 StructureTemplatePool.Projection.TERRAIN_MATCHING 的固定处理器列表：
    //   new GravityProcessor(Heightmap.Types.WORLD_SURFACE_WG, -1)
    // heightmapType=0 对应 WorldSurfaceWG，offset 固定为 -1（不依赖 groundLevelDelta）。
    // groundLevelDelta 仅用于 JigsawJunction 的 deltaY 计算（见 JigsawAssembler::tryPlacePiece），不参与此处。
    if (placed.projection == JigsawPlacementBehaviour::TerrainMatching) {
        processorList.addProcessor(std::make_unique<GravityStructureProcessor>(0, -1));
    }

    settings.setProcessors(&processorList);

    // 设置世界引用（GravityStructureProcessor 需要 IWorld 查询高度图）
    const IWorld* iworld = dynamic_cast<const IWorld*>(&world);
    if (iworld) {
        settings.setWorld(iworld);
    }

    // 放置模板
    templ->place(world, placed.position, settings, rng, 18);
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
