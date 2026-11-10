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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "IglooStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <memory>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;

// ============================================================================
// 静态常量
// ============================================================================

const std::string IglooStructure::s_name = "Igloo";

// 雪屋模板名称
const std::string IglooStructure::s_topTemplateName = "igloo/top";
const std::string IglooStructure::s_middleTemplateName = "igloo/middle";
const std::string IglooStructure::s_bottomTemplateName = "igloo/bottom";

// ============================================================================
// IglooPiece
// ============================================================================

namespace {

/// 各模板的旋转枢轴与相对锚点的偏移（原版 `IglooPieces.PIVOTS` / `OFFSETS`）。
struct IglooPieceShape {
    const std::string* templateName;
    BlockPos pivot;
    BlockPos offset;
};

const IglooPieceShape& shapeOf(const std::string& templateName)
{
    static const IglooPieceShape shapes[] = {
        {&IglooStructure::s_topTemplateName, BlockPos(3, 5, 5), BlockPos(0, 0, 0)},
        {&IglooStructure::s_middleTemplateName, BlockPos(1, 3, 1), BlockPos(2, -3, 4)},
        {&IglooStructure::s_bottomTemplateName, BlockPos(3, 6, 7), BlockPos(0, -3, -2)},
    };
    for (const auto& shape : shapes) {
        if (*shape.templateName == templateName) {
            return shape;
        }
    }
    // 未知模板名属于装配错误（模板表就三个），直接暴露而不是退回某个默认枢轴。
    MC_ASSERT_RELEASE_MSG(false, "IglooPiece: unknown template");
    return shapes[0];
}

} // namespace

IglooPiece::IglooPiece(const std::string& templateName, const BlockPos& position, Rotation rotation)
    : StructurePiece(StructurePieceTypes::IGLOO, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_templateName(templateName)
    , m_templatePosition(position)
{
    // 旋转与镜像存进基类（子类同名成员会遮蔽基类，getRotation() 读的正是基类那个）。
    setRotation(rotation);
    setMirror(Mirror::None);
    // 原版 `TemplateStructurePiece` 在构造期就以模板真实包围盒确定几何；等到后处理阶段才展开
    // 会让 STRUCTURE_STARTS 阶段每个构件退化成一个点。
    m_template = jigsaw::JigsawAssembler::getTemplateManager().getTemplate(ResourceLocation(m_templateName));
    if (m_template == nullptr) {
        return;
    }
    m_size = m_template->getSize();
    _updateBoundingBox();
}

void IglooPiece::_updateBoundingBox()
{
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setRotationPivot(shapeOf(m_templateName).pivot);
    const StructureBoundingBox rect = m_template->getBoundingBox(settings, BlockPos(0, 0, 0));

    m_minX = m_templatePosition.x + rect.minX();
    m_minY = m_templatePosition.y + rect.minY();
    m_minZ = m_templatePosition.z + rect.minZ();
    m_maxX = m_templatePosition.x + rect.maxX();
    m_maxY = m_templatePosition.y + rect.maxY();
    m_maxZ = m_templatePosition.z + rect.maxZ();
}

void IglooPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (m_templateManager == nullptr) {
        m_templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }
    if (m_template == nullptr) {
        // 构造期模板管理器还没装好时补一次加载。
        m_template = m_templateManager->getTemplate(ResourceLocation(m_templateName));
        if (m_template == nullptr) {
            spdlog::warn("IglooPiece: template '{}' is missing", m_templateName);
            return;
        }
        m_size = m_template->getSize();
        _updateBoundingBox();
    }

    const StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "IglooPiece::generate requires an IWorld");

    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setRotationPivot(shapeOf(m_templateName).pivot);
    settings.setBoundingBox(&chunkBounds);

    m_template->placeInWorld(*reader, m_templatePosition, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS);
}

// ============================================================================
// IglooStructure
// ============================================================================

IglooStructure::IglooStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* IglooStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_IGLOO();
}

std::unique_ptr<StructureStart> IglooStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 生物群系校验落在候选生成点（区块中心、地表高度）上；不匹配即返回空起点。
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::WorldSurfaceWG))) {
        return start;
    }

    // 原版 findGenerationPoint 走 onTopOfChunkCenter：它**不消耗随机数**（X/Z 是区块中心的
    // 定值，Y 走高度图），generatePieces 第一件事就是取旋转。这里同样直接从旋转开始。
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (templateManager == nullptr) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    const i32 baseX = chunkX * world::CHUNK_WIDTH;
    const i32 baseZ = chunkZ * world::CHUNK_WIDTH;
    const BlockPos anchor(baseX, 90, baseZ);
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 原版 `IglooPieces.addPieces`：50% 概率带地下室——先放最底下的实验室，再逐层往上放阶梯，
    // 最后才是地面上的雪屋本体。顺序即后处理顺序，不能打乱。
    //
    // 每个构件的模板原点 = 锚点 + 该模板的 OFFSETS 再下移层数*3；层偏移只作用在 Y 上。
    if (rng.nextDouble() < 0.5) {
        const i32 depth = rng.nextInt(8) + 4;
        auto bottom = std::make_unique<IglooPiece>(
            s_bottomTemplateName, anchor + BlockPos(0, -3, -2) - BlockPos(0, 3 * depth, 0), rotation);
        bottom->setTemplateManager(templateManager);
        start->addPiece(std::move(bottom));

        for (i32 index = 0; index < depth - 1; ++index) {
            auto middle = std::make_unique<IglooPiece>(
                s_middleTemplateName, anchor + BlockPos(2, -3, 4) - BlockPos(0, 3 * index, 0), rotation);
            middle->setTemplateManager(templateManager);
            start->addPiece(std::move(middle));
        }
    }

    auto top = std::make_unique<IglooPiece>(s_topTemplateName, anchor, rotation);
    top->setTemplateManager(templateManager);
    start->addPiece(std::move(top));

    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
