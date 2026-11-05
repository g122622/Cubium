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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/JigsawTransform.hpp"
#include "server/world/gen/structure/Structure.hpp"
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

bool EndCityStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * world::CHUNK_WIDTH + 8, 64, chunkZ * world::CHUNK_WIDTH + 8);
    if (!isValidBiome(biome)) {
        return false;
    }
    // 末地城只在高度 >= 60 的位置生成。这里的采样朝向只影响采样列，不参与世界生成
    // 随机流（原版 canGenerate 也不消耗世界随机数）。
    return _getYPosition(chunkX, chunkZ, feature::template_::Rotation::None, generator) >= 60;
}

std::unique_ptr<StructureStart> EndCityStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 顺序不能变：原版 findGenerationPoint 先抽旋转，再用这个旋转去取 5x5 四角的最低
    // 高度，最后才判 Y < 60。此前这里是先算 Y（用的还是另一个自造随机源）、再抽旋转，
    // 既多抽了一次随机数，判定用的旋转也与实际生成用的不一致。
    feature::template_::Rotation rotation = static_cast<feature::template_::Rotation>(rng.nextInt(4));

    // 计算生成位置
    // 原版 getLowestYIn5by5BoxOffset7Blocks 用的锚点是 chunkPos.getBlockX(7) /
    // getBlockZ(7)，即区块内偏移 7，不是 8。差这 1 格会让整座塔在 X/Z 上整体偏移。
    const i32 x = chunkX * world::CHUNK_WIDTH + 7;
    const i32 z = chunkZ * world::CHUNK_WIDTH + 7;
    const i32 y = _getYPosition(chunkX, chunkZ, rotation, generator);

    if (y < 60) {
        return start;
    }

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

i32 EndCityStructure::_getYPosition(
    i32 chunkX, i32 chunkZ, feature::template_::Rotation rotation, IChunkGenerator& generator)
{
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
    // 原版走世界生成期高度图（WORLD_SURFACE_WG），
    // 不是区块内已生成的 WorldSurface；getHeight 返回的已是"最高方块上方一格"。
    const i32 i1 = generator.getHeight(k, l, HeightmapType::WorldSurfaceWG);
    const i32 j1 = generator.getHeight(k, l + j, HeightmapType::WorldSurfaceWG);
    const i32 k1 = generator.getHeight(k + i, l, HeightmapType::WorldSurfaceWG);
    const i32 l1 = generator.getHeight(k + i, l + j, HeightmapType::WorldSurfaceWG);

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
    , m_templatePath("minecraft:end_city/" + templateName)
    , m_templatePosition(pos)
    , m_overwrite(overwrite)
{
    setRotation(rotation);
    m_settings.setRotation(rotation);
    m_settings.setIgnoreEntities(true);

    // 【包围盒必须在构造期就确定】原版 `TemplateStructurePiece` 的构造直接用模板算出
    // 包围盒（`StructureTemplate.getBoundingBox(templatePosition, settings)`），后续
    // `move()` 只是平移这个盒子。这里原本把 m_min*/m_max* 全设成 pos，尺寸要等到
    // FEATURES 阶段的 generate() 才回填——于是 STRUCTURE_STARTS 阶段每个构件都是退化到
    // 单点的盒子：整座末地城的整体包围盒、构件碰撞判定、差分输出全部失真。
    //
    // 模板名不带命名空间，原版由 `makeIdentifier` 补上 `minecraft:` 前缀，这里同样处理。
    const feature::template_::Template* templ = jigsaw::JigsawAssembler::getTemplateManager().getTemplate(
        ResourceLocation("minecraft", "end_city/" + templateName));
    if (templ != nullptr) {
        // 包围盒 = 以**原点**为轴按旋转铺开后的范围，再平移到 templatePosition。
        // 原版 `TemplateStructurePiece` 的 BoundingBox 正是
        // `structureTemplate.getBoundingBox(templatePosition, placeSettings)`，内部同样是
        // 「变换两个对角点再整体平移」。注意旋转后范围不一定从 0 开始（90/270 度时 X 落在
        // [-(size.z-1), 0]），所以不能简单地"把 size 铺在 pos 上"。
        m_size = templ->getSize();
        const BlockPos cornerA =
            jigsaw::JigsawTransform::transformPosition(BlockPos(0, 0, 0), m_rotation, Mirror::None, BlockPos(0, 0, 0));
        const BlockPos cornerB = jigsaw::JigsawTransform::transformPosition(
            BlockPos(m_size.x - 1, m_size.y - 1, m_size.z - 1), m_rotation, Mirror::None, BlockPos(0, 0, 0));
        m_minX = pos.x + std::min(cornerA.x, cornerB.x);
        m_minY = pos.y + std::min(cornerA.y, cornerB.y);
        m_minZ = pos.z + std::min(cornerA.z, cornerB.z);
        m_maxX = pos.x + std::max(cornerA.x, cornerB.x);
        m_maxY = pos.y + std::max(cornerA.y, cornerB.y);
        m_maxZ = pos.z + std::max(cornerA.z, cornerB.z);
    }
}

void CityTemplate::setGenerators(std::shared_ptr<CityGenerators> generators)
{
    m_generators = std::move(generators);
}

// 同一座城的所有片段共享一组生成器：`addPiece` 从父片段继承，根片段由
// `startHouseTower` 挂载。断言非空可立即暴露漏挂。
void CityTemplate::shareGeneratorsFrom(const CityTemplate& parent)
{
    m_generators = parent.m_generators;
}

HouseTowerGenerator& CityTemplate::houseTowerGen()
{
    MC_ASSERT_RELEASE(m_generators != nullptr);
    return m_generators->houseTower;
}

TowerGenerator& CityTemplate::towerGen()
{
    MC_ASSERT_RELEASE(m_generators != nullptr);
    return m_generators->tower;
}

TowerBridgeGenerator& CityTemplate::bridgeGen()
{
    MC_ASSERT_RELEASE(m_generators != nullptr);
    return m_generators->bridge;
}

FatTowerGenerator& CityTemplate::fatTowerGen()
{
    MC_ASSERT_RELEASE(m_generators != nullptr);
    return m_generators->fatTower;
}

void CityTemplate::generate(IWorldWriter& world,
    math::IRandom& rng,
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
            for (i32 y = 0; y < 4; ++y) {
                for (i32 x = 0; x < 4; ++x) {
                    for (i32 z = 0; z < 4; ++z) {
                        BlockPos worldPos =
                            BlockPos(m_templatePosition.x + x, m_templatePosition.y + y, m_templatePosition.z + z);
                        if (chunkBounds.contains(worldPos.x, worldPos.y, worldPos.z)) {
                            world.setBlockState(worldPos.x,
                                worldPos.y,
                                worldPos.z,
                                endStoneBricks,
                                world::BlockUpdateFlags::UPDATE_CLIENTS);
                        }
                    }
                }
            }
        }
        return;
    }

    // 放置模板：覆盖模式声明形状已知（跳过形状重算），非覆盖模式仅同步客户端
    templ->place(world,
        m_templatePosition,
        m_settings,
        rng,
        m_overwrite ? (world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE)
                    : world::BlockUpdateFlags::UPDATE_CLIENTS);
}

BlockPos CityTemplate::calculateConnectedPos(const BlockPos& localPos, feature::template_::Rotation newRotation) const
{
    // 原版 StructureTemplate.calculateConnectedPosition：
    //
    //   calculateRelativePosition(settingsA, posA).subtract(calculateRelativePosition(settingsB, posB));
    //   calculateRelativePosition(settings, pos) = transform(pos, mirror, rotation, pivot);
    //
    // 也就是「父构件里锚点变换后的位置」减去「新构件里对应点变换后的位置」，返回的是**差值**，
    // 不是绝对坐标。两端都要用各自的旋转/镜像变换；此前这里只按父构件的旋转做了一次
    // 手工 switch，且把 newRotation 直接标了 MC_UNUSED——新构件的旋转被完全忽略，
    // 于是带旋转的桥段/房屋接上去就会错位。
    const BlockPos parentRelative = feature::template_::Template::transformBlockPos(
        localPos, m_settings.getMirror(), m_rotation, BlockPos(0, 0, 0));
    const BlockPos childRelative = feature::template_::Template::transformBlockPos(
        BlockPos(0, 0, 0), feature::template_::Mirror::None, newRotation, BlockPos(0, 0, 0));

    return BlockPos(
        parentRelative.x - childRelative.x, parentRelative.y - childRelative.y, parentRelative.z - childRelative.z);
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
    // 原版 EndCityPieces.addPiece：
    //
    //   EndCityPiece piece = new EndCityPiece(mgr, name, parent.templatePosition(), rot, overwrite);
    //   BlockPos rel = parent.template().calculateConnectedPosition(
    //       parent.placeSettings(), offset, piece.placeSettings(), BlockPos.ZERO);
    //   piece.move(rel.getX(), rel.getY(), rel.getZ());
    //
    // 注意新构件的**初始位置**是父构件的 templatePosition（不是零），
    // 然后按 calculateConnectedPosition 返回的**差值**平移。
    // 新片段同时从父片段继承本座城的生成器集合。
    auto piece = std::make_unique<CityTemplate>(templateName, parent.templatePosition(), rotation, overwrite);
    piece->shareGeneratorsFrom(parent);

    const BlockPos relative = parent.calculateConnectedPos(offset, rotation);
    piece->offset(relative.x, relative.y, relative.z);

    return piece;
}

bool recursiveChildren(feature::template_::TemplateManager& templateManager,
    IGenerator& generator,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng)
{
    // 最大深度为 8
    if (depth > 8) {
        return false;
    }

    // 临时列表存储新生成的片段
    std::vector<std::unique_ptr<StructurePiece>> newPieces;

    // 调用生成器。生成器状态（如「本座城是否已生成末地船」）在 `startHouseTower`
    // 开头统一 init() 重置，这里不重复。
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
    math::IRandom& rng)
{
    // 生成器实例挂在片段上，随结构起点一起销毁：用函数内 static 会让「已生成末地船」
    // 之类的一次性标记残留到下一座城（原版每次 generatePieces 都 init() 重置）。
    auto baseFloor = std::make_unique<CityTemplate>("base_floor", startPos, rotation, true);
    CityTemplate* current = addHelper(pieces, std::move(baseFloor));
    auto generators = std::make_shared<CityGenerators>();
    generators->houseTower.init();
    generators->tower.init();
    generators->bridge.init();
    generators->fatTower.init();
    current->setGenerators(std::move(generators));

    // second_floor_1 + third_floor_1 + third_roof
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 0, -1), "second_floor_1", rotation, false));
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "third_floor_1", rotation, false));
    current = addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 8, -1), "third_roof", rotation, true));

    // 递归生成塔
    recursiveChildren(templateManager, current->towerGen(), 1, *current, BlockPos(), pieces, rng);
}

// ============================================================================
// HouseTowerGenerator 实现
// ============================================================================

bool HouseTowerGenerator::generate(feature::template_::TemplateManager& templateManager,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng)
{
    if (depth > 8) {
        return false;
    }

    feature::template_::Rotation rotation = parent.rotation();

    // 放置 base_floor
    //
    // 原版用调用方传来的 offset 作连接锚点；此前这里写死成 BlockPos()，于是从桥段
    // 接过来的房屋会丢掉整个偏移量（实测 [10] 的旋转与位置同时错位）。
    CityTemplate* current = addHelper(pieces, addPiece(templateManager, parent, offset, "base_floor", rotation, true));

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
        recursiveChildren(templateManager, current->towerGen(), depth + 1, *current, BlockPos(), pieces, rng);
    } else {
        // 三层房屋
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(-1, 0, -1), "second_floor_2", rotation, false));
        current = addHelper(
            pieces, addPiece(templateManager, *current, BlockPos(-1, 4, -1), "third_floor_2", rotation, false));
        current =
            addHelper(pieces, addPiece(templateManager, *current, BlockPos(-1, 8, -1), "third_roof", rotation, true));

        // 递归生成塔
        recursiveChildren(templateManager, current->towerGen(), depth + 1, *current, BlockPos(), pieces, rng);
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
    math::IRandom& rng)
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
        TowerBridgeGenerator& bridgeGen = current->bridgeGen();

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
            return recursiveChildren(
                templateManager, current->fatTowerGen(), depth + 1, *current, BlockPos(), pieces, rng);
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
    math::IRandom& rng)
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
            // 带坡度的桥。连接偏移与模板名是**错配**的（陡坡走 -4，缓坡走 -8），
            // 原版 `TOWER_BRIDGE_GENERATOR` 就是这样写的，照抄即可——把它们"改正"会让整条
            // 桥链的构件位置整体偏移一个模板跨度。
            if (rng.nextBoolean()) {
                // 陡坡
                current = addHelper(pieces,
                    addPiece(
                        templateManager, *current, BlockPos(0, yOffset, -4), "bridge_steep_stairs", rotation, true));
            } else {
                // 缓坡
                current = addHelper(pieces,
                    addPiece(
                        templateManager, *current, BlockPos(0, yOffset, -8), "bridge_gentle_stairs", rotation, true));
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
        if (!recursiveChildren(templateManager,
                current->houseTowerGen(),
                depth + 1,
                *current,
                BlockPos(-3, yOffset + 1, -11),
                pieces,
                rng)) {
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
    math::IRandom& rng)
{
    feature::template_::Rotation rotation = parent.rotation();

    // 放置 fat_tower_base
    CityTemplate* current =
        addHelper(pieces, addPiece(templateManager, parent, BlockPos(-3, 4, -3), "fat_tower_base", rotation, true));

    // 放置 fat_tower_middle
    current =
        addHelper(pieces, addPiece(templateManager, *current, BlockPos(0, 4, 0), "fat_tower_middle", rotation, true));

    // 可能添加更多层和桥。桥生成器沿用本座城已有实例——原版胖塔分支同样直接复用
    // 同一个 TOWER_BRIDGE_GENERATOR 的「已生成末地船」状态，不在此处重置。
    // shipCreated 必须在整棵树范围内保持，重置会让末地船被重复生成。
    TowerBridgeGenerator& bridgeGen = current->bridgeGen();

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
