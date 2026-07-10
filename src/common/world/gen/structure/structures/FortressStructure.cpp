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

#include "FortressStructure.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/world/gen/jigsaw/JigsawPlacer.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <cmath>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

// ============================================================================
// 匿名命名空间：FortressPlacedPieceAdapter
// ============================================================================

namespace {

/**
 * @brief 下界要塞 Jigsaw 片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class FortressPlacedPieceAdapter final : public StructurePiece {
public:
    explicit FortressPlacedPieceAdapter(jigsaw::PlacedPiece placed)
        : StructurePiece(FortressPieceTypes::START,
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

} // namespace

// ============================================================================
// FortressFallbackPiece
// ============================================================================

FortressFallbackPiece::FortressFallbackPiece(const BlockPos& pos)
    : StructurePiece(
          StructurePieceTypes::FORTRESS_FALLBACK, pos.x - 8, pos.y - 1, pos.z - 8, pos.x + 37, pos.y + 14, pos.z + 28)
    , m_startPos(pos)
{}

void FortressFallbackPiece::generate(IWorldWriter& world,
    math::Random& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (!getBoundingBox().intersects(chunkBounds)) {
        return;
    }
    _generateFallbackFortress(world, rng, chunkBounds);
}

void FortressFallbackPiece::_generateFallbackFortress(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    // 获取方块状态
    const BlockState* netherBricks = VanillaBlocks::getState(VanillaBlocks::NETHERRACK); // 使用下界岩替代
    const BlockState* netherrack = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
    const BlockState* soulSand = VanillaBlocks::getState(VanillaBlocks::SOUL_SAND);
    const BlockState* netherWart = VanillaBlocks::getState(VanillaBlocks::NETHER_WART);
    const BlockState* netherWartBlock = VanillaBlocks::getState(VanillaBlocks::NETHER_WART_BLOCK);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);

    i32 baseX = m_startPos.x;
    i32 baseY = m_startPos.y;
    i32 baseZ = m_startPos.z;

    // 生成主桥 (直线段)
    // 尺寸: 5x10x19
    for (i32 z = 0; z < 19; ++z) {
        // 桥面
        for (i32 x = 0; x < 5; ++x) {
            i32 wx = baseX + x, wy = baseY, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
        // 围栏
        {
            i32 wx = baseX, wy = baseY + 1, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
        {
            i32 wx = baseX + 4, wy = baseY + 1, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
        {
            i32 wx = baseX, wy = baseY + 2, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
        {
            i32 wx = baseX + 4, wy = baseY + 2, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }

        // 中间是空气
        for (i32 y = 1; y <= 8; ++y) {
            {
                i32 wx = baseX + 1, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, air, 18);
                }
            }
            {
                i32 wx = baseX + 2, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, air, 18);
                }
            }
            {
                i32 wx = baseX + 3, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, air, 18);
                }
            }
        }
    }

    // 生成王座房间（带烈焰人刷怪笼）
    // 尺寸: 7x8x9
    i32 throneX = baseX + 10;
    i32 throneY = baseY;
    i32 throneZ = baseZ + 20;

    // 地板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 9; ++z) {
            i32 wx = throneX + x, wy = throneY, wz = throneZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
    }

    // 墙壁
    for (i32 y = 1; y <= 7; ++y) {
        for (i32 x = 0; x < 7; ++x) {
            {
                i32 wx = throneX + x, wy = throneY + y, wz = throneZ;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
            {
                i32 wx = throneX + x, wy = throneY + y, wz = throneZ + 8;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
        }
        for (i32 z = 0; z < 9; ++z) {
            {
                i32 wx = throneX, wy = throneY + y, wz = throneZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
            {
                i32 wx = throneX + 6, wy = throneY + y, wz = throneZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
        }
    }

    // 天花板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 9; ++z) {
            i32 wx = throneX + x, wy = throneY + 8, wz = throneZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
    }

    // 烈焰人刷怪点标记（中心偏移）
    {
        i32 wx = throneX + 3, wy = throneY + 5, wz = throneZ + 4;
        if (bounds.contains(wx, wy, wz)) {
            world.setBlockState(wx, wy, wz, netherWartBlock ? netherWartBlock : netherBricks, 18);
        }
    }

    // 生成地狱疣房间
    // 尺寸: 13x14x13
    i32 wartRoomX = baseX + 25;
    i32 wartRoomY = baseY;
    i32 wartRoomZ = baseZ + 10;

    // 地板（灵魂沙）
    for (i32 x = 0; x < 13; ++x) {
        for (i32 z = 0; z < 13; ++z) {
            i32 wx = wartRoomX + x, wy = wartRoomY, wz = wartRoomZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, soulSand, 18);
            }
        }
    }

    // 墙壁
    for (i32 y = 1; y <= 13; ++y) {
        for (i32 x = 0; x < 13; ++x) {
            {
                i32 wx = wartRoomX + x, wy = wartRoomY + y, wz = wartRoomZ;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
            {
                i32 wx = wartRoomX + x, wy = wartRoomY + y, wz = wartRoomZ + 12;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
        }
        for (i32 z = 0; z < 13; ++z) {
            {
                i32 wx = wartRoomX, wy = wartRoomY + y, wz = wartRoomZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
            {
                i32 wx = wartRoomX + 12, wy = wartRoomY + y, wz = wartRoomZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherBricks, 18);
                }
            }
        }
    }

    // 天花板
    for (i32 x = 0; x < 13; ++x) {
        for (i32 z = 0; z < 13; ++z) {
            i32 wx = wartRoomX + x, wy = wartRoomY + 14, wz = wartRoomZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, netherBricks, 18);
            }
        }
    }

    // 灵魂沙平台上的地狱疣
    // 平台位置: (3-4, 4, 4-8) 和 (8-9, 4, 4-8)
    for (i32 x = 3; x <= 4; ++x) {
        for (i32 z = 4; z <= 8; ++z) {
            if (netherWart) {
                i32 wx = wartRoomX + x, wy = wartRoomY + 1, wz = wartRoomZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherWart, 18);
                }
            }
        }
    }
    for (i32 x = 8; x <= 9; ++x) {
        for (i32 z = 4; z <= 8; ++z) {
            if (netherWart) {
                i32 wx = wartRoomX + x, wy = wartRoomY + 1, wz = wartRoomZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, netherWart, 18);
                }
            }
        }
    }

    // 入口（带岩浆）
    // 尺寸: 13x14x13
    i32 entranceX = baseX - 8;
    i32 entranceY = baseY;
    i32 entranceZ = baseZ - 8;

    // 简化入口：岩浆坑
    for (i32 x = 0; x < 5; ++x) {
        for (i32 z = 0; z < 5; ++z) {
            i32 wx = entranceX + x, wy = entranceY - 1, wz = entranceZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, lava, 18);
            }
        }
    }
}

// ============================================================================
// FortressStructure
// ============================================================================

const std::string FortressStructure::m_name = "fortress";

const SpawnOverrides FortressStructure::s_spawnOverrides = {
    SpawnOverrideType::Piece, {SpawnOverrideEntry{"monster", 2, 4}}};

FortressStructure::FortressStructure()
    : Structure(ResourceLocation("minecraft", "fortress"))
{}

FortressStructure::FortressStructure(const Config& config)
    : Structure(ResourceLocation("minecraft", "fortress"))
    , m_config(config)
{}

const biome::BiomeTag* FortressStructure::biomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_FORTRESS();
}

bool FortressStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 下界要塞只检查概率，不检查生物群系
    // 生物群系检查由维度的 BiomeGenerationSettings 决定

    // 40% 概率生成
    return rng.nextInt(5) < 2;
}

std::unique_ptr<StructureStart> FortressStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算起始位置
    i32 startX = chunkX * world::CHUNK_WIDTH + 2;
    i32 startZ = chunkZ * world::CHUNK_WIDTH + 2;

    // Y 坐标在配置范围内随机选择
    i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);

    BlockPos startPos(startX, startY, startZ);

    // 使用 Jigsaw 系统生成要塞
    ResourceLocation startPoolLocation("minecraft", "nether_fortress/start");
    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(startPoolLocation);

    if (startPool && !startPool->isEmpty()) {
        // 使用 Jigsaw 系统组装
        auto placedPieces = jigsaw::JigsawAssembler::assemble(
            patternRegistry, *startPool, 10, startPos, rng, generator, nullptr, nullptr, nullptr);

        // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
        for (auto& placed : placedPieces) {
            if (placed.piece && !placed.piece->isEmpty()) {
                start->addPiece(std::make_unique<FortressPlacedPieceAdapter>(std::move(placed)));
            }
        }
    } else {
        // 回退：创建简单的要塞结构片段
        auto piece = std::make_unique<FortressFallbackPiece>(startPos);
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();
    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
