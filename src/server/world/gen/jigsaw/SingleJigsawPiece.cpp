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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/AssemblyTypes.hpp"
#include "server/world/gen/jigsaw/JigsawPiece.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include "server/world/gen/jigsaw/ProcessorListRegistry.hpp"

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
    // 拼图块的名字即其模板资源路径：装配诊断（"这个构件来自哪个模板"）与
    // 逐构件 parity 校验都依赖它，缺失时无法区分"选错模板"与"摆放位置偏差"。
    setName(templateName);

    // 不在此处解析模板：模板池在启动期会构造全部 ~1 200 个本类实例，同步解析会让数据包里
    // 989 个结构模板（约 7.5 MB）在服务端开始接受连接之前全部读入并长期驻留，而其中绝大多数
    // 在本次运行中不会被用到。连接点与尺寸推迟到首次访问时经 _ensureLoaded() 解析，
    // 对应 MC 原版 SinglePoolElement 只持有模板 id、getSize/place 时才 getOrCreate 的行为。
}

LegacySingleJigsawPiece::LegacySingleJigsawPiece(const std::string& templateName,
    JigsawPlacementBehaviour behaviour,
    const std::optional<ResourceLocation>& processorListId)
    : SingleJigsawPiece(templateName, behaviour, processorListId)
{}

/**
 * @brief 获取 Structure Block 的方块 ID 列表（用于 BlockIgnore 处理器）
 *
 * BlockIgnoreStructureProcessor 按方块 ID 忽略，覆盖该方块的全部状态。
 * 延迟初始化，因为 BlockRegistry 可能尚未就绪。
 */
static std::vector<u32> getStructureBlockIds()
{
    static std::vector<u32> ids;
    if (ids.empty()) {
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            ids.push_back(state->blockId());
        }
    }
    return ids;
}

/**
 * @brief 获取 Structure Block + Air 的方块 ID 列表（用于 Legacy BlockIgnore 处理器）
 *
 * Legacy 拼图块使用此列表，因为旧版结构模板中空气方块是显式放置的，
 * 需要忽略它们以避免覆盖已有地形。
 */
static std::vector<u32> getStructureAndAirBlockIds()
{
    static std::vector<u32> ids;
    if (ids.empty()) {
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            ids.push_back(state->blockId());
        }
        if (auto* state = VanillaBlocks::getState(VanillaBlocks::AIR)) {
            ids.push_back(state->blockId());
        }
    }
    return ids;
}

void SingleJigsawPiece::place(IWorldWriter& world,
    const PlacedPiece& placed,
    TemplateManager& templateManager,
    math::IRandom& rng,
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
    // 1. BlockIgnoreStructureProcessor — 标准构件只忽略 structure_block，必须排在
    //    JigsawReplacement 之前
    // 2. JigsawReplacementStructureProcessor — 替换 jigsaw 方块为 final_state
    // 3. Piece 自带 processor list — 从 ProcessorListRegistry 查找
    // 4. GravityStructureProcessor — terrain_matching 投影时添加
    // 5. BlockIgnoreStructureProcessor — **仅 legacy 构件**，替换 1 中的忽略器并追加在最后
    StructureProcessorList processorList;

    // 1. 标准构件的 structure_block 忽略
    if (!isLegacy()) {
        processorList.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(getStructureBlockIds()));
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

    // 4. 投影处理器（terrain_matching → GravityStructureProcessor，按 WORLD_SURFACE_WG 贴地）
    if (placed.projection == JigsawPlacementBehaviour::TerrainMatching) {
        processorList.addProcessor(
            std::make_unique<GravityStructureProcessor>(world::chunk::HeightmapType::WorldSurfaceWG, -1));
    }

    // 5. legacy 构件：原版 `LegacySinglePoolElement#getSettings` 把 super 里的
    //    `STRUCTURE_BLOCK` 忽略器替换成 `STRUCTURE_AND_AIR`，追加在**整条链的末尾**。
    //    顺序不能提前：拼图方块的 `final_state` 往往是 air，必须等 JigsawReplacement
    //    把它换成空气之后才能被忽略掉（提前忽略只会看到 jigsaw 方块本身）。
    if (isLegacy()) {
        processorList.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(getStructureAndAirBlockIds()));
    }

    settings.setProcessors(&processorList);

    // 拼图构件在装配阶段已保证形状正确，放置后不再按邻居重算形状
    settings.setKnownShape(true);

    // 设置世界引用（GravityStructureProcessor 需要 IWorld 查询高度图）
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(iworld != nullptr, "SingleJigsawPiece::place requires an IWorld");
    settings.setWorld(iworld);

    // 放置模板
    templ->placeInWorld(*iworld,
        placed.position,
        settings,
        rng,
        world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
