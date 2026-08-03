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

#include "EndCityStructure.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../biome/BiomeTags.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../feature/template/TemplateManager.hpp"
#include "../../jigsaw/JigsawAssembler.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;
using namespace end_city;

// ============================================================================
// 常量定义
// ============================================================================

namespace {
/// 塔桥连接点配置
struct BridgeAttachment {
    feature::template_::Rotation rotation;
    BlockPos offset;
};

const BridgeAttachment TOWER_BRIDGES[] = {
    {feature::template_::Rotation::None, BlockPos(1, -1, 0)},
    {feature::template_::Rotation::Clockwise90, BlockPos(6, -1, 1)},
    {feature::template_::Rotation::CounterClockwise90, BlockPos(0, -1, 5)},
    {feature::template_::Rotation::Clockwise180, BlockPos(5, -1, 6)},
};

/// 胖塔桥连接点配置
const BridgeAttachment FAT_TOWER_BRIDGES[] = {
    {feature::template_::Rotation::None, BlockPos(4, -1, 0)},
    {feature::template_::Rotation::Clockwise90, BlockPos(12, -1, 4)},
    {feature::template_::Rotation::CounterClockwise90, BlockPos(0, -1, 8)},
    {feature::template_::Rotation::Clockwise180, BlockPos(8, -1, 12)},
};

/// 旋转加法
feature::template_::Rotation addRotation(feature::template_::Rotation a, feature::template_::Rotation b)
{
    return static_cast<feature::template_::Rotation>((static_cast<i32>(a) + static_cast<i32>(b)) % 4);
}

} // namespace

// ============================================================================
// EndCityStructure 实现
// ============================================================================

const std::string EndCityStructure::s_name = "End_City";

EndCityStructure::EndCityStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* EndCityStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_END_CITY();
}

bool EndCityStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * world::CHUNK_WIDTH + 8, 64, chunkZ * world::CHUNK_WIDTH + 8);
    if (isValidBiome(biome)) {
        // 末地城只在高度 >= 60 的位置生成
        return _getYPosition(chunkX, chunkZ, generator) >= 60;
    }
    return false;
}

std::unique_ptr<StructureStart> EndCityStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * world::CHUNK_WIDTH + 8;
    i32 z = chunkZ * world::CHUNK_WIDTH + 8;
    i32 y = _getYPosition(chunkX, chunkZ, generator);

    if (y < 60) {
        return start;
    }

    // 随机旋转
    feature::template_::Rotation rotation = static_cast<feature::template_::Rotation>(rng.nextInt(4));

    // 使用递归生成器系统
    std::vector<std::unique_ptr<StructurePiece>> pieces;

    // 获取模板管理器
    auto& templateManager = jigsaw::JigsawAssembler::getTemplateManager();

    // 启动房屋塔生成
    startHouseTower(templateManager, BlockPos(x, y, z), rotation, pieces, rng);

    // 将所有片段添加到 StructureStart（方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行）
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();
    return start;
}

i32 EndCityStructure::_getYPosition(i32 chunkX, i32 chunkZ, IChunkGenerator& generator)
{
    math::Random random(static_cast<i64>(chunkX + chunkZ * 10387313));
    feature::template_::Rotation rotation = static_cast<feature::template_::Rotation>(random.nextInt(4));

    i32 i = 5;
    i32 j = 5;
    switch (rotation) {
        case feature::template_::Rotation::Clockwise90:
            i = -5;
            break;
        case feature::template_::Rotation::Clockwise180:
            i = -5;
            j = -5;
            break;
        case feature::template_::Rotation::CounterClockwise90:
            j = -5;
            break;
        default:
            break;
    }

    i32 k = (chunkX << world::CHUNK_SHIFT) + 7;
    i32 l = (chunkZ << world::CHUNK_SHIFT) + 7;
    i32 i1 = generator.getHeight(k, l, HeightmapType::WorldSurface);
    i32 j1 = generator.getHeight(k, l + j, HeightmapType::WorldSurface);
    i32 k1 = generator.getHeight(k + i, l, HeightmapType::WorldSurface);
    i32 l1 = generator.getHeight(k + i, l + j, HeightmapType::WorldSurface);

    return std::min({i1, j1, k1, l1});
}

// ============================================================================
// end_city::CityTemplate 实现
// ============================================================================

namespace end_city {

CityTemplate::CityTemplate(
    const std::string& templateName, const BlockPos& pos, feature::template_::Rotation rotation, bool overwrite)
    : StructurePiece(StructurePieceTypes::END_CITY, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z)
    , m_templateName(templateName)
    , m_templatePosition(pos)
    , m_rotation(rotation)
    , m_overwrite(overwrite)
{
    m_settings.setRotation(rotation);
    m_settings.setIgnoreEntities(true);

    // 设置处理器：overwrite 模式替换所有方块，insert 模式保留空气
    if (overwrite) {
        // OVERWRITE: 替换所有方块（包括结构方块）
        // 使用默认设置即可
    } else {
        // INSERT: 保留空气和结构方块
        // 在模板放置时处理
    }
}

void CityTemplate::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 获取模板
    auto& templateManager = jigsaw::JigsawAssembler::getTemplateManager();
    const feature::template_::Template* templ =
        templateManager.getTemplate(ResourceLocation("minecraft", "end_city/" + m_templateName));

    if (!templ || templ->getBlockCount() == 0) {
        // 模板未找到，使用占位方块
        const BlockState* endStoneBricks = VanillaBlocks::getState(VanillaBlocks::END_STONE_BRICKS);
        if (endStoneBricks) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    for (int z = 0; z < 4; ++z) {
                        BlockPos worldPos =
                            BlockPos(m_templatePosition.x + x, m_templatePosition.y + y, m_templatePosition.z + z);
                        if (chunkBounds.contains(worldPos.x, worldPos.y, worldPos.z)) {
                            world.setBlockState(worldPos.x, worldPos.y, worldPos.z, endStoneBricks, 2);
                        }
                    }
                }
            }
        }
        return;
    }

    // 更新模板大小
    m_size = templ->getSize();

    // 放置模板
    templ->place(world, m_templatePosition, m_settings, rng, m_overwrite ? 18 : 2);

    // 更新边界框 - 根据旋转计算实际尺寸
    switch (m_rotation) {
        case feature::template_::Rotation::None:
        case feature::template_::Rotation::Clockwise180:
            m_maxX = m_minX + m_size.x - 1;
            m_maxZ = m_minZ + m_size.z - 1;
            break;
        case feature::template_::Rotation::Clockwise90:
        case feature::template_::Rotation::CounterClockwise90:
            m_maxX = m_minX + m_size.z - 1;
            m_maxZ = m_minZ + m_size.x - 1;
            break;
    }
    m_maxY = m_minY + m_size.y - 1;
}

BlockPos CityTemplate::calculateConnectedPos(const BlockPos& localPos, feature::template_::Rotation newRotation) const
{
    MC_UNUSED(newRotation);
    // 计算从当前模板的 localPos 位置，连接到新模板的偏移

    // 获取当前模板尺寸
    BlockPos size = m_size;

    // 根据旋转计算变换
    BlockPos transformedLocal;

    // 先根据当前旋转变换 localPos
    switch (m_rotation) {
        case feature::template_::Rotation::None:
            transformedLocal = localPos;
            break;
        case feature::template_::Rotation::Clockwise90:
            transformedLocal = BlockPos(size.z - 1 - localPos.z, localPos.y, localPos.x);
            break;
        case feature::template_::Rotation::Clockwise180:
            transformedLocal = BlockPos(size.x - 1 - localPos.x, localPos.y, size.z - 1 - localPos.z);
            break;
        case feature::template_::Rotation::CounterClockwise90:
            transformedLocal = BlockPos(localPos.z, localPos.y, size.x - 1 - localPos.x);
            break;
    }

    // 计算世界坐标
    return m_templatePosition + transformedLocal;
}

// ============================================================================
// 辅助函数
// ============================================================================

CityTemplate* addHelper(std::vector<std::unique_ptr<StructurePiece>>& pieces, std::unique_ptr<CityTemplate> piece)
{
    CityTemplate* ptr = piece.get();
    pieces.push_back(std::move(piece));
    return ptr;
}

std::unique_ptr<CityTemplate> addPiece(feature::template_::TemplateManager& templateManager,
    CityTemplate& parent,
    const BlockPos& offset,
    const std::string& templateName,
    feature::template_::Rotation rotation,
    bool overwrite)
{
    // 创建新片段
    auto piece = std::make_unique<CityTemplate>(templateName, parent.templatePosition(), rotation, overwrite);

    // 计算连接位置
    BlockPos connectedPos = parent.calculateConnectedPos(offset, rotation);

    // 偏移到正确位置
    piece->offset(connectedPos.x - piece->templatePosition().x,
        connectedPos.y - piece->templatePosition().y,
        connectedPos.z - piece->templatePosition().z);

    return piece;
}

bool recursiveChildren(feature::template_::TemplateManager& templateManager,
    IGenerator& generator,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    // 最大深度为 8
    if (depth > 8) {
        return false;
    }

    // 临时列表存储新生成的片段
    std::vector<std::unique_ptr<StructurePiece>> newPieces;

    // 调用生成器
    if (!generator.generate(templateManager, depth, parent, offset, newPieces, rng)) {
        return false;
    }

    // 检查碰撞
    i32 componentId = rng.nextInt();
    bool hasCollision = false;

    for (auto& newPiece : newPieces) {
        CityTemplate* cityPiece = dynamic_cast<CityTemplate*>(newPiece.get());
        if (!cityPiece) {
            continue;
        }

        cityPiece->componentId = componentId;

        // 检查与现有片段的碰撞
        for (const auto& existingPiece : pieces) {
            const CityTemplate* existingCity = dynamic_cast<const CityTemplate*>(existingPiece.get());
            if (existingCity && existingCity->componentId != parent.componentId) {
                if (cityPiece->intersects(existingCity->getBoundingBox())) {
                    hasCollision = true;
                    break;
                }
            }
        }

        if (hasCollision) {
            break;
        }
    }

    if (hasCollision) {
        return false;
    }

    // 添加新片段到主列表
    for (auto& piece : newPieces) {
        pieces.push_back(std::move(piece));
    }

    return true;
}

void startHouseTower(feature::template_::TemplateManager& templateManager,
    const BlockPos& startPos,
    feature::template_::Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    static HouseTowerGenerator houseTowerGen;
    static TowerGenerator towerGen;
    static TowerBridgeGenerator bridgeGen;
    static FatTowerGenerator fatTowerGen;

    // 初始化生成器
    houseTowerGen.init();
    towerGen.init();
    bridgeGen.init();
    fatTowerGen.init();

    // 创建基础片段
    auto baseFloor = std::make_unique<CityTemplate>("base_floor", startPos, rotation, true);
    CityTemplate* current = addHelper(pieces, std::move(baseFloor));

    // second_floor_1 + third_floor_1 + third_roof
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 0, -1), "second_floor_1", rotation, false));
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "third_floor_1", rotation, false));
    current = addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 8, -1), "third_roof", rotation, true));

    // 递归生成塔
    recursiveChildren(templateManager, towerGen, 1, *current, BlockPos(), pieces, rng);
}

// ============================================================================
// HouseTowerGenerator 实现
// ============================================================================

bool HouseTowerGenerator::generate(feature::template_::TemplateManager& templateManager,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    if (depth > 8) {
        return false;
    }

    feature::template_::Rotation rotation = parent.rotation();

    // 放置 base_floor
    CityTemplate* current =
        addHelper(pieces, addPiece(templateManager, parent, BlockPos(), "base_floor", rotation, true));

    // 随机选择房屋类型
    i32 variant = rng.nextInt(3);

    if (variant == 0) {
        // 简单屋顶
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "base_roof", rotation, true));
    } else if (variant == 1) {
        // 二层房屋
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(-1, 0, -1), "second_floor_2", rotation, false));
        current =
            addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 8, -1), "second_roof", rotation, false));

        // 递归生成塔
        static TowerGenerator towerGen;
        recursiveChildren(templateManager, towerGen, depth + 1, *current, BlockPos(), pieces, rng);
    } else {
        // 三层房屋
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(-1, 0, -1), "second_floor_2", rotation, false));
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "third_floor_2", rotation, false));
        current =
            addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 8, -1), "third_roof", rotation, true));

        // 递归生成塔
        static TowerGenerator towerGen;
        recursiveChildren(templateManager, towerGen, depth + 1, *current, BlockPos(), pieces, rng);
    }

    return true;
}

// ============================================================================
// TowerGenerator 实现
// ============================================================================

bool TowerGenerator::generate(feature::template_::TemplateManager& templateManager,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    feature::template_::Rotation rotation = parent.rotation();

    // 放置 tower_base
    i32 towerBaseOffsetX = 3 + rng.nextInt(2);
    i32 towerBaseOffsetZ = 3 + rng.nextInt(2);
    CityTemplate* current = addHelper(pieces,
        addPiece(
            templateManager, parent, BlockPos(towerBaseOffsetX, -3, towerBaseOffsetZ), "tower_base", rotation, true));

    // 放置 tower_piece
    current = addHelper(pieces, addPiece(templateManager, *current, BlockPos(0, 7, 0), "tower_piece", rotation, true));

    // 确定是否有桥连接点
    CityTemplate* bridgeAnchor = (rng.nextInt(3) == 0) ? current : nullptr;

    // 添加额外的 tower_piece
    i32 numPieces = 1 + rng.nextInt(3);
    for (i32 i = 0; i < numPieces; ++i) {
        current =
            addHelper(pieces, addPiece(templateManager, *current, BlockPos(0, 4, 0), "tower_piece", rotation, true));

        // 最后一个之前的片段可以作为桥连接点
        if (i < numPieces - 1 && rng.nextBoolean()) {
            bridgeAnchor = current;
        }
    }

    if (bridgeAnchor != nullptr) {
        // 有桥连接点：生成桥和塔顶
        static TowerBridgeGenerator bridgeGen;

        for (const auto& bridge : TOWER_BRIDGES) {
            if (rng.nextBoolean()) {
                feature::template_::Rotation bridgeRot = addRotation(rotation, bridge.rotation);
                CityTemplate* bridgeEnd = addHelper(
                    pieces, addPiece(templateManager, *bridgeAnchor, bridge.offset, "bridge_end", bridgeRot, true));
                recursiveChildren(templateManager, bridgeGen, depth + 1, *bridgeEnd, BlockPos(), pieces, rng);
            }
        }

        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "tower_top", rotation, true));
    } else {
        // 无桥连接点：可能生成胖塔或塔顶
        if (depth != 7) {
            static FatTowerGenerator fatTowerGen;
            return recursiveChildren(templateManager, fatTowerGen, depth + 1, *current, BlockPos(), pieces, rng);
        }

        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "tower_top", rotation, true));
    }

    return true;
}

// ============================================================================
// TowerBridgeGenerator 实现
// ============================================================================

bool TowerBridgeGenerator::generate(feature::template_::TemplateManager& templateManager,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    feature::template_::Rotation rotation = parent.rotation();

    // 添加桥段
    i32 numSegments = rng.nextInt(4) + 1;
    CityTemplate* current =
        addHelper(pieces, addPiece(templateManager, parent, BlockPos(0, 0, -4), "bridge_piece", rotation, true));
    current->componentId = -1; // 标记为桥段

    i32 yOffset = 0;

    for (i32 i = 0; i < numSegments; ++i) {
        if (rng.nextBoolean()) {
            // 平直桥
            current = addHelper(
                pieces, addPiece(templateManager, *current, BlockPos(0, yOffset, -4), "bridge_piece", rotation, true));
            yOffset = 0;
        } else {
            // 带坡度的桥
            if (rng.nextBoolean()) {
                // 陡坡
                current = addHelper(pieces,
                    addPiece(
                        templateManager, *current, BlockPos(0, yOffset, -8), "bridge_steep_stairs", rotation, true));
            } else {
                // 缓坡
                current = addHelper(pieces,
                    addPiece(
                        templateManager, *current, BlockPos(0, yOffset, -4), "bridge_gentle_stairs", rotation, true));
            }
            yOffset = 4;
        }
    }

    // 决定生成末地船或房屋
    if (!m_shipCreated && rng.nextInt(10 - depth) == 0) {
        // 生成末地船
        i32 shipOffsetX = -8 + rng.nextInt(8);
        i32 shipOffsetZ = -70 + rng.nextInt(10);
        addHelper(pieces,
            addPiece(templateManager, *current, BlockPos(shipOffsetX, yOffset, shipOffsetZ), "ship", rotation, true));
        m_shipCreated = true;
    } else {
        // 生成房屋
        static HouseTowerGenerator houseTowerGen;
        if (!recursiveChildren(
                templateManager, houseTowerGen, depth + 1, *current, BlockPos(-3, yOffset + 1, -11), pieces, rng)) {
            return false;
        }
    }

    // 添加桥的另一端
    current = addHelper(pieces,
        addPiece(templateManager,
            *current,
            BlockPos(4, yOffset, 0),
            "bridge_end",
            addRotation(rotation, feature::template_::Rotation::Clockwise180),
            true));
    current->componentId = -1;

    return true;
}

// ============================================================================
// FatTowerGenerator 实现
// ============================================================================

bool FatTowerGenerator::generate(feature::template_::TemplateManager& templateManager,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng)
{
    feature::template_::Rotation rotation = parent.rotation();

    // 放置 fat_tower_base
    CityTemplate* current =
        addHelper(pieces, addPiece(templateManager, parent, BlockPos(-3, 4, -3), "fat_tower_base", rotation, true));

    // 放置 fat_tower_middle
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(0, 4, 0), "fat_tower_middle", rotation, true));

    // 可能添加更多层和桥
    static TowerBridgeGenerator bridgeGen;
    bridgeGen.init();

    for (i32 layer = 0; layer < 2 && rng.nextInt(3) != 0; ++layer) {
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(0, 8, 0), "fat_tower_middle", rotation, true));

        // 为每层添加桥
        for (const auto& bridge : FAT_TOWER_BRIDGES) {
            if (rng.nextBoolean()) {
                feature::template_::Rotation bridgeRot = addRotation(rotation, bridge.rotation);
                CityTemplate* bridgeEnd = addHelper(
                    pieces, addPiece(templateManager, *current, bridge.offset, "bridge_end", bridgeRot, true));
                recursiveChildren(templateManager, bridgeGen, depth + 1, *bridgeEnd, BlockPos(), pieces, rng);
            }
        }
    }

    // 放置 fat_tower_top
    addHelper(pieces, addPiece(templateManager, *current, BlockPos(-2, 8, -2), "fat_tower_top", rotation, true));

    return true;
}

} // namespace end_city

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
