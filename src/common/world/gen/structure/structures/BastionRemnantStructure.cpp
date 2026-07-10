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

#include "BastionRemnantStructure.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/JigsawPlacer.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string BastionRemnantStructure::s_name = "bastion_remnant";

const SpawnOverrides BastionRemnantStructure::s_spawnOverrides = {
    SpawnOverrideType::Piece, {SpawnOverrideEntry{"monster", 2, 4}, SpawnOverrideEntry{"creature", 2, 4}}};

namespace {

/**
 * @brief 堡垒遗迹片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class BastionPlacedPieceAdapter final : public StructurePiece {
public:
    explicit BastionPlacedPieceAdapter(jigsaw::PlacedPiece placed)
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

// 堡垒遗迹4种类型的起始池路径
constexpr const char* BASTION_START_POOLS[] = {
    "minecraft:bastion/units/start",    // 单元型 (权重最高)
    "minecraft:bastion/stables/start",  // 猪灵兽栏
    "minecraft:bastion/treasure/start", // 宝藏型
    "minecraft:bastion/bridge/start"    // 桥梁型
};

// 各类型的选择权重 (units 权重最高)
constexpr i32 BASTION_WEIGHTS[] = {4, 2, 2, 2}; // units, stables, treasure, bridge

} // anonymous namespace

BastionRemnantStructure::BastionRemnantStructure()
    : Structure(ResourceLocation("minecraft", "bastion_remnant"))
{}

const biome::BiomeTag* BastionRemnantStructure::biomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_BASTION_REMNANT();
}

bool BastionRemnantStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系 - 堡垒遗迹在所有下界生物群系中生成（玄武岩三角洲除外）
    BiomeId biome = generator.getBiome(chunkX * world::CHUNK_WIDTH + 8, 64, chunkZ * world::CHUNK_WIDTH + 8);
    if (!isValidBiome(biome)) {
        return false;
    }
    return true;
}

std::unique_ptr<StructureStart> BastionRemnantStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 随机选择堡垒类型
    i32 totalWeight = 0;
    for (i32 w : BASTION_WEIGHTS) {
        totalWeight += w;
    }

    i32 randomValue = rng.nextInt(totalWeight);
    i32 accumulated = 0;
    i32 selectedIndex = 0;

    for (size_t i = 0; i < 4; ++i) {
        accumulated += BASTION_WEIGHTS[i];
        if (randomValue < accumulated) {
            selectedIndex = static_cast<i32>(i);
            break;
        }
    }

    ResourceLocation startPoolLocation(BASTION_START_POOLS[selectedIndex]);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(startPoolLocation);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 堡垒遗迹生成在下界的固定高度
    constexpr i32 startY = 60;

    BlockPos startPos(chunkX * world::CHUNK_WIDTH + 8, startY, chunkZ * world::CHUNK_WIDTH + 8);

    // 使用 JigsawAssembler 组装堡垒结构，maxDepth = 7
    auto placedPieces = jigsaw::JigsawAssembler::assemble(
        patternRegistry, *startPool, 7, startPos, rng, generator, nullptr, nullptr, nullptr);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    for (auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<BastionPlacedPieceAdapter>(std::move(placed)));
        }
    }

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
