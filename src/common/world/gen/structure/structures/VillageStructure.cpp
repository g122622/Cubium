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

#include "VillageStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/jigsaw/AssemblyTypes.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/world/gen/jigsaw/JigsawPlacer.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

namespace {

// 村庄生成相关常量
constexpr i32 VILLAGE_MIN_TERRAIN_HEIGHT = 50;   // 村庄最低生成高度
constexpr i32 VILLAGE_MIN_SURFACE_HEIGHT = 60;   // 地面最低有效高度
constexpr i32 VILLAGE_DEFAULT_HEIGHT = 64;       // 默认村庄高度
constexpr i32 VILLAGE_MAX_HEIGHT_VARIATION = 12; // 村庄区域最大高差
constexpr i32 SAMPLE_OFFSET_DISTANCE = 8;        // 采样点偏移距离（区块半径的一半）

/**
 * @brief Jigsaw 结构片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class VillagePlacedPieceAdapter final : public StructurePiece {
public:
    explicit VillagePlacedPieceAdapter(jigsaw::PlacedPiece placed)
        : StructurePiece(90,
              placed.boundingBox.minX(),
              placed.boundingBox.minY(),
              placed.boundingBox.minZ(),
              placed.boundingBox.maxX(),
              placed.boundingBox.maxY(),
              placed.boundingBox.maxZ())
        , m_placed(std::move(placed))
        , m_groundLevelDelta(m_placed.groundLevelDelta)
        , m_junctions(m_placed.junctions)
    {}

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32,
        i32,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override
    {
        jigsaw::JigsawPlacer::placePiece(world, m_placed, rng, &chunkBounds, chunk, generator);
    }

    [[nodiscard]] i32 getGroundLevelDelta() const override { return m_groundLevelDelta; }

    [[nodiscard]] const std::vector<jigsaw::JigsawJunction>& getJunctions() const override { return m_junctions; }

    [[nodiscard]] bool isJigsawPiece() const override { return true; }

    [[nodiscard]] mc::StructurePieceProjection getProjection() const noexcept override
    {
        return (m_placed.projection == mc::world::gen::jigsaw::JigsawPlacementBehaviour::TerrainMatching)
            ? mc::StructurePieceProjection::TerrainMatching
            : mc::StructurePieceProjection::Rigid;
    }

private:
    jigsaw::PlacedPiece m_placed;
    i32 m_groundLevelDelta;
    std::vector<jigsaw::JigsawJunction> m_junctions;
};

} // anonymous namespace

using namespace mc::Biomes;

const std::string VillageStructure::m_name = "village";

VillageStructure::VillageStructure(ResourceLocation id, VillageType type)
    : Structure(std::move(id))
{
    m_config.type = type;
}

VillageStructure::VillageStructure(ResourceLocation id, const VillageConfig& config)
    : Structure(std::move(id))
    , m_config(config)
{}

const biome::BiomeTag* VillageStructure::defaultBiomeTag() const
{
    // 村庄有多个变体，每个变体对应不同的生物群系标签
    // 默认返回平原村庄标签，canGenerate() 中根据村庄类型进行详细检查
    return &biome::BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS();
}

bool VillageStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    (void)world;
    (void)rng;

    using namespace mc::world;

    const i32 centerX = chunkX * CHUNK_WIDTH + SAMPLE_OFFSET_DISTANCE;
    const i32 centerZ = chunkZ * CHUNK_WIDTH + SAMPLE_OFFSET_DISTANCE;

    // 检查生物群系
    const BiomeId biomeId = generator.getBiome(centerX, SEA_LEVEL, centerZ);
    if (!isValidBiome(biomeId)) {
        return false;
    }

    // 检查地形是否具备建造空间：中心与四角高差不能过大
    constexpr i32 SAMPLE_OFFSETS[5][2] = {{0, 0},
        {-SAMPLE_OFFSET_DISTANCE, -SAMPLE_OFFSET_DISTANCE},
        {-SAMPLE_OFFSET_DISTANCE, SAMPLE_OFFSET_DISTANCE},
        {SAMPLE_OFFSET_DISTANCE, -SAMPLE_OFFSET_DISTANCE},
        {SAMPLE_OFFSET_DISTANCE, SAMPLE_OFFSET_DISTANCE}};

    i32 minHeight = std::numeric_limits<i32>::max();
    i32 maxHeight = std::numeric_limits<i32>::min();

    for (const auto& offset : SAMPLE_OFFSETS) {
        const i32 sampleX = centerX + offset[0];
        const i32 sampleZ = centerZ + offset[1];
        const i32 h = generator.getHeight(sampleX, sampleZ, HeightmapType::WorldSurfaceWG);
        minHeight = std::min(minHeight, h);
        maxHeight = std::max(maxHeight, h);
    }

    if (minHeight < VILLAGE_MIN_TERRAIN_HEIGHT) {
        return false;
    }

    return (maxHeight - minHeight) <= VILLAGE_MAX_HEIGHT_VARIATION;
}

std::unique_ptr<StructureStart> VillageStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    using namespace mc::world;

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    ResourceLocation startPoolLocation = getStartPool(m_config.type);
    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(startPoolLocation);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 计算起始位置
    i32 startX = chunkX * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 startZ = chunkZ * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < VILLAGE_MIN_SURFACE_HEIGHT) startY = VILLAGE_DEFAULT_HEIGHT;

    BlockPos startPos(startX, startY, startZ);

    // 使用 JigsawAssembler 组装村庄结构
    // 组装获取 PlacedPiece 列表，包含 JigsawJunction 信息用于地形适配
    auto placedPieces = jigsaw::JigsawAssembler::assemble(
        patternRegistry, *startPool, m_config.size, startPos, rng, generator, nullptr, nullptr, nullptr);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    // 这样 NoiseChunkGenerator::collectStructureData 可以收集 Junction 信息
    for (auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<VillagePlacedPieceAdapter>(std::move(placed)));
        }
    }

    return start;
}

ResourceLocation VillageStructure::getStartPool(VillageType type)
{
    switch (type) {
        case VillageType::Plains:
            return ResourceLocation("minecraft", "village/plains/town_centers");
        case VillageType::Desert:
            return ResourceLocation("minecraft", "village/desert/town_centers");
        case VillageType::Savanna:
            return ResourceLocation("minecraft", "village/savanna/town_centers");
        case VillageType::Taiga:
            return ResourceLocation("minecraft", "village/taiga/town_centers");
        case VillageType::Snowy:
            return ResourceLocation("minecraft", "village/snowy/town_centers");
        case VillageType::Zombie:
            // 僵尸村庄使用独立的模板池（使用平原模板，但可扩展）
            return ResourceLocation("minecraft", "village/zombie/town_centers");
        default:
            return ResourceLocation("minecraft", "village/plains/town_centers");
    }
}

const char* VillageStructure::getVillageTypeName(VillageType type)
{
    switch (type) {
        case VillageType::Plains:
            return "plains";
        case VillageType::Desert:
            return "desert";
        case VillageType::Savanna:
            return "savanna";
        case VillageType::Taiga:
            return "taiga";
        case VillageType::Snowy:
            return "snowy";
        case VillageType::Zombie:
            return "zombie";
        default:
            spdlog::error("Unknown VillageType: {}", static_cast<u8>(type));
            return "unknown";
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
