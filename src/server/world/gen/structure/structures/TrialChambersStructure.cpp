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

#include "TrialChambersStructure.hpp"

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
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/jigsaw/AssemblyTypes.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/jigsaw/JigsawPlacer.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include "server/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "server/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "server/world/gen/structure/JigsawStructure.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/valueprovider/HeightProvider.hpp"
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

const std::string TrialChambersStructure::s_name = "trial_chambers";

namespace {

/// 试炼密室片段适配器
class TrialChambersPlacedPieceAdapter final : public StructurePiece {
public:
    explicit TrialChambersPlacedPieceAdapter(jigsaw::PlacedPiece placed)
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
        math::IRandom& rng,
        i32,
        i32,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override
    {
        jigsaw::JigsawPlacer::placePiece(world, m_placed, rng, &chunkBounds, chunk, generator);
    }

    [[nodiscard]] i32 getGroundLevelDelta() const override
    {
        return m_groundLevelDelta;
    }

    [[nodiscard]] const std::vector<jigsaw::JigsawJunction>& getJunctions() const override
    {
        return m_junctions;
    }

    [[nodiscard]] bool isJigsawPiece() const override
    {
        return true;
    }

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

TrialChambersStructure::TrialChambersStructure(ResourceLocation id)
    : JigsawStructure(std::move(id),
          JigsawConfig(ResourceLocation("minecraft", "trial_chambers/chamber/end"),
              20, // maxDepth = 20
              valueprovider::UniformHeight::create(
                  valueprovider::VerticalAnchor::absolute(-40), valueprovider::VerticalAnchor::absolute(-20)),
              createPoolAliases(),               // 池别名绑定
              MaxDistance(116),                  // 最大距离116格
              DimensionPadding(10, 10),          // 维度填充10
              LiquidSettings::IgnoreWaterlogging // 忽略含水方块
              ),
          0,                             // startY 由 HeightProvider 决定
          false,                         // nearTerrain
          false,                         // adjustForTerrain
          TerrainAdaptation::Encapsulate // 用凝灰岩砖完全包裹
      )
{}

const biome::BiomeTag* TrialChambersStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_TRIAL_CHAMBERS();
}

jigsaw::PoolAliasBindings TrialChambersStructure::createPoolAliases()
{
    using AliasBinding = jigsaw::RandomPoolAliasBinding;

    jigsaw::PoolAliasBindings bindings;

    // ========== 远程型刷怪笼别名 ==========
    // ranged：skeleton / stray / poison_skeleton（各1/3概率）
    auto rangedBinding =
        std::make_unique<AliasBinding>(ResourceLocation("minecraft", "trial_chambers/spawner/contents/ranged"),
            std::vector<AliasBinding::WeightedTarget>{
                {ResourceLocation("minecraft", "trial_chambers/spawner/ranged/skeleton"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/ranged/stray"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/ranged/poison_skeleton"), 1},
            });
    bindings.addBinding(std::move(rangedBinding));

    // ========== 近战型刷怪笼别名 ==========
    // melee：zombie / husk / spider（各1/3概率）
    auto meleeBinding =
        std::make_unique<AliasBinding>(ResourceLocation("minecraft", "trial_chambers/spawner/contents/melee"),
            std::vector<AliasBinding::WeightedTarget>{
                {ResourceLocation("minecraft", "trial_chambers/spawner/melee/zombie"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/melee/husk"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/melee/spider"), 1},
            });
    bindings.addBinding(std::move(meleeBinding));

    // ========== 小型近战型刷怪笼别名 ==========
    // small_melee：slime / cave_spider / silverfish / baby_zombie（各1/4概率）
    auto smallMeleeBinding =
        std::make_unique<AliasBinding>(ResourceLocation("minecraft", "trial_chambers/spawner/contents/small_melee"),
            std::vector<AliasBinding::WeightedTarget>{
                {ResourceLocation("minecraft", "trial_chambers/spawner/small_melee/slime"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/small_melee/cave_spider"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/small_melee/silverfish"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/small_melee/baby_zombie"), 1},
            });
    bindings.addBinding(std::move(smallMeleeBinding));

    // ========== 低刷怪频率远程型别名 ==========
    // slow_ranged：与 ranged 相同的选项但使用低刷怪频率变体
    auto slowRangedBinding =
        std::make_unique<AliasBinding>(ResourceLocation("minecraft", "trial_chambers/spawner/contents/slow_ranged"),
            std::vector<AliasBinding::WeightedTarget>{
                {ResourceLocation("minecraft", "trial_chambers/spawner/slow_ranged/skeleton"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/slow_ranged/stray"), 1},
                {ResourceLocation("minecraft", "trial_chambers/spawner/slow_ranged/poison_skeleton"), 1},
            });
    bindings.addBinding(std::move(slowRangedBinding));

    return bindings;
}

bool TrialChambersStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);

    auto& patternRegistry = jigsaw::TemplatePoolRegistry::instance();
    const jigsaw::TemplatePool* startPool = patternRegistry.getPool(m_config.startPool);
    if (!startPool || startPool->isEmpty()) {
        return false;
    }

    // 生物群系校验的采样点是候选生成点（起始块中心、投影后的地面线），不是区块中心：
    // 区块中心取 y=0 会落到洞穴/深板岩群系上，把整片合法区块误判为群系不符。候选点依赖
    // 起始块，只能由基类在装配后校验（JigsawStructure::generate + start_jigsaw_name）。
    MC_UNUSED(generator);
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    return true;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
