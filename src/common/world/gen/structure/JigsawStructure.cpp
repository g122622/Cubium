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

#include "JigsawStructure.hpp"

#include "../../../core/Constants.hpp"
#include "../../../util/assert/AssertMacros.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../WorldConstants.hpp"
#include "../../block/BlockPos.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../jigsaw/JigsawManager.hpp"
#include "../jigsaw/JigsawPattern.hpp"

namespace {

/// Jigsaw 结构生成所需的最小地形高度
constexpr i32 MIN_TERRAIN_HEIGHT = 20;

/// Jigsaw 结构生成所需的最大地形高度
constexpr i32 MAX_TERRAIN_HEIGHT = 220;

/// 默认起始高度（接近海平面）
constexpr i32 DEFAULT_START_Y = 64;

/**
 * @brief Jigsaw 结构片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class JigsawPlacedPieceAdapter final : public mc::world::gen::structure::StructurePiece {
public:
    explicit JigsawPlacedPieceAdapter(const mc::world::gen::jigsaw::PlacedPiece& placed) noexcept
        : StructurePiece(90,
              placed.boundingBox.minX(),
              placed.boundingBox.minY(),
              placed.boundingBox.minZ(),
              placed.boundingBox.maxX(),
              placed.boundingBox.maxY(),
              placed.boundingBox.maxZ())
        , m_groundLevelDelta(placed.groundLevelDelta)
        , m_junctions(placed.junctions)
    {}

    void generate(mc::IWorldWriter&,
        mc::math::Random&,
        mc::i32,
        mc::i32,
        const mc::world::gen::structure::StructureBoundingBox&) override
    {
        // 实际方块放置已在 JigsawManager::assembleAndPlace 中完成
    }

    [[nodiscard]] mc::i32 getGroundLevelDelta() const noexcept override { return m_groundLevelDelta; }

    [[nodiscard]] const std::vector<mc::world::gen::jigsaw::JigsawJunction>& getJunctions() const noexcept override
    {
        return m_junctions;
    }

    [[nodiscard]] bool isJigsawPiece() const noexcept override { return true; }

private:
    mc::i32 m_groundLevelDelta;
    std::vector<mc::world::gen::jigsaw::JigsawJunction> m_junctions;
};

} // namespace

namespace mc {
namespace world {
namespace gen {
namespace structure {

const std::string JigsawStructure::m_name = "jigsaw";
const std::vector<BiomeId> JigsawStructure::m_validBiomes;

JigsawStructure::JigsawStructure(const JigsawConfig& config, i32 startY, bool nearTerrain, bool adjustForTerrain)
    : Structure(StructureType::Village)
    , m_config(config)
    , m_startY(startY)
    , m_nearTerrain(nearTerrain)
    , m_adjustForTerrain(adjustForTerrain)
{}

bool JigsawStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);
    if (!startPool || startPool->isEmpty()) {
        return false;
    }

    if (m_nearTerrain) {
        // 计算区块中心坐标
        const i32 centerX = chunkX * CHUNK_WIDTH + CHUNK_WIDTH / 2;
        const i32 centerZ = chunkZ * CHUNK_WIDTH + CHUNK_WIDTH / 2;
        const i32 topY = generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG);

        // 检查地形高度是否在有效范围内
        if (topY < MIN_TERRAIN_HEIGHT || topY > MAX_TERRAIN_HEIGHT) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<StructureStart> JigsawStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 计算起始位置
    i32 startY = m_startY;
    if (m_nearTerrain) {
        // 如果需要贴合地形，查询地面高度
        const i32 centerX = chunkX * CHUNK_WIDTH + CHUNK_WIDTH / 2;
        const i32 centerZ = chunkZ * CHUNK_WIDTH + CHUNK_WIDTH / 2;
        startY = generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG);

        // 地形太低时使用默认高度
        if (startY < MIN_TERRAIN_HEIGHT) {
            startY = m_startY > 0 ? m_startY : DEFAULT_START_Y;
        }
    }

    // 计算结构起始位置（区块中心）
    const BlockPos startPos(chunkX * CHUNK_WIDTH + CHUNK_WIDTH / 2, startY, chunkZ * CHUNK_WIDTH + CHUNK_WIDTH / 2);

    // 使用 JigsawManager 组装结构，获取 PlacedPiece 列表
    // PlacedPiece 包含 JigsawJunction 信息用于地形适配
    auto placedPieces = jigsaw::JigsawManager::assemble(patternRegistry, *startPool, m_config.size, startPos, rng);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    // 这样 NoiseChunkGenerator::collectStructureData 可以收集 Junction 信息
    for (const auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<JigsawPlacedPieceAdapter>(placed));
        }
    }

    // 放置方块到世界
    for (const auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            jigsaw::JigsawManager::placePieceRecursive(world, placed, rng);
        }
    }

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
