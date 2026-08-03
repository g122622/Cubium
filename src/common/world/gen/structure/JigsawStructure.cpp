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

#include "../../../util/assert/AssertMacros.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../WorldConstants.hpp"
#include "../../block/BlockPos.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../jigsaw/AssemblyTypes.hpp"
#include "../jigsaw/JigsawAssembler.hpp"
#include "../jigsaw/JigsawJunction.hpp"
#include "../jigsaw/JigsawPlacer.hpp"
#include "../jigsaw/TemplatePoolRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using mc::i32;

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
    explicit JigsawPlacedPieceAdapter(mc::world::gen::jigsaw::PlacedPiece placed) noexcept
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

    void generate(mc::IWorldWriter& world,
        mc::math::Random& rng,
        mc::i32 chunkX,
        mc::i32 chunkZ,
        const mc::world::gen::structure::StructureBoundingBox& chunkBounds,
        mc::ChunkPrimer* chunk,
        mc::IChunkGenerator* generator) override
    {
        // MC 1.21.11: 方块放置在 FEATURES 阶段通过 placeInChunk → generate 调用
        if (m_placed.piece && !m_placed.piece->isEmpty()) {
            mc::world::gen::jigsaw::JigsawPlacer::placePiece(world, m_placed, rng, &chunkBounds, chunk, generator);
        }
    }

    [[nodiscard]] mc::i32 getGroundLevelDelta() const noexcept override { return m_groundLevelDelta; }

    [[nodiscard]] const std::vector<mc::world::gen::jigsaw::JigsawJunction>& getJunctions() const noexcept override
    {
        return m_junctions;
    }

    [[nodiscard]] bool isJigsawPiece() const noexcept override { return true; }

    [[nodiscard]] mc::StructurePieceProjection getProjection() const noexcept override
    {
        return (m_placed.projection == mc::world::gen::jigsaw::JigsawPlacementBehaviour::TerrainMatching)
            ? mc::StructurePieceProjection::TerrainMatching
            : mc::StructurePieceProjection::Rigid;
    }

private:
    mc::world::gen::jigsaw::PlacedPiece m_placed;
    mc::i32 m_groundLevelDelta;
    std::vector<mc::world::gen::jigsaw::JigsawJunction> m_junctions;
};

} // namespace

namespace mc {
namespace world {
namespace gen {
namespace structure {

const std::string JigsawStructure::m_name = "jigsaw";

JigsawStructure::JigsawStructure(ResourceLocation id,
    JigsawConfig config,
    i32 startY,
    bool nearTerrain,
    bool adjustForTerrain,
    TerrainAdaptation terrainAdaptation)
    : Structure(std::move(id))
    , m_config(std::move(config))
    , m_startY(startY)
    , m_nearTerrain(nearTerrain)
    , m_adjustForTerrain(adjustForTerrain)
{
    // 写入基类 m_terrainAdaptation：非 None 时由基类非虚 terrainAdaptation() 直接返回；
    // None 时回退 defaultTerrainAdaptation()（同样返回基类成员，为 None）。
    m_terrainAdaptation = terrainAdaptation;
}

bool JigsawStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(m_config.startPool);
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
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(m_config.startPool);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 计算起始位置
    i32 startY = m_startY;
    if (m_config.startHeight) {
        // 使用高度提供者计算起始高度
        valueprovider::WorldGenerationContext context(MIN_BUILD_HEIGHT, CHUNK_HEIGHT);
        startY = m_config.startHeight->sample(rng, context);
    } else if (m_nearTerrain) {
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

    // 预解析池别名绑定（试炼密室等结构的池随机化）。
    // 无别名时传 nullptr，JigsawAssembler 使用空查找表（恒等映射）。
    const jigsaw::PoolAliasBindings* aliases = m_config.poolAliases.empty() ? nullptr : &m_config.poolAliases;

    // 使用 JigsawAssembler 组装结构，获取 PlacedPiece 列表
    // PlacedPiece 包含 JigsawJunction 信息用于地形适配
    // maxDistanceFromCenter 用于初始化 freeShape 可放置空间（VoxelShape 空间追踪），
    //   缺省时 JigsawAssembler 内部使用 MC 默认值 MaxDistance(80)。
    const structure::MaxDistance* maxDistance =
        m_config.maxDistanceFromCenter.has_value() ? &(*m_config.maxDistanceFromCenter) : nullptr;
    auto placedPieces = jigsaw::JigsawAssembler::assemble(patternRegistry,
        *startPool,
        m_config.size,
        startPos,
        rng,
        generator,
        aliases,
        maxDistance,
        &m_config.dimensionPadding);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    // MC 1.21.11: 结构起点只创建片段，不写入方块
    // 方块放置延迟到 FEATURES 阶段，由 placeInChunk() 调用 generate() 执行
    for (auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<JigsawPlacedPieceAdapter>(std::move(placed)));
        }
    }

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
