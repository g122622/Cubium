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

#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/jigsaw/JigsawManager.hpp"
#include "common/world/gen/jigsaw/JigsawPattern.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string TrialChambersStructure::s_name = "trial_chambers";

// 试炼密室可在大多数主世界生物群系中生成
// 根据MC 1.21规范，使用 has_structure/trial_chambers 生物群系标签
// 深暗之域生成概率降低
const std::vector<BiomeId> TrialChambersStructure::s_validBiomes = {
    Plains,
    SunflowerPlains,
    Forest,
    FlowerForest,
    BirchForest,
    OldGrowthBirchForest,
    DarkForest,
    CherryGrove,
    Swamp,
    MangroveSwamp,
    Taiga,
    OldGrowthPineTaiga,
    OldGrowthSpruceTaiga,
    SnowyPlains,
    IceSpikes,
    SnowyTaiga,
    WindsweptHills,
    WindsweptForest,
    WindsweptGravellyHills,
    Jungle,
    SparseJungle,
    BambooJungle,
    Badlands,
    ErodedBadlands,
    WoodedBadlands,
    Meadow,
    Grove,
    SnowySlopes,
    FrozenPeaks,
    JaggedPeaks,
    StonyPeaks,
    River,
    FrozenRiver,
    Beach,
    SnowyBeach,
    StonyShore,
    WarmOcean,
    LukewarmOcean,
    DeepLukewarmOcean,
    Ocean,
    DeepOcean,
    ColdOcean,
    DeepColdOcean,
    FrozenOcean,
    DeepFrozenOcean,
    MushroomFields,
    DripstoneCaves,
    LushCaves,
    Savanna,
    SavannaPlateau,
    WindsweptSavanna,
    Desert,
    SparseJungle,
    // TODO(trial_chambers): 深暗之域应降低生成概率，需要特殊处理
};

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

    void generate(IWorldWriter& world, math::Random& rng, i32, i32, const StructureBoundingBox& chunkBounds) override
    {
        jigsaw::JigsawManager::placePieceRecursive(world, m_placed, rng, &chunkBounds);
    }

    [[nodiscard]] i32 getGroundLevelDelta() const override { return m_groundLevelDelta; }
    [[nodiscard]] const std::vector<jigsaw::JigsawJunction>& getJunctions() const override { return m_junctions; }
    [[nodiscard]] bool isJigsawPiece() const override { return true; }

private:
    jigsaw::PlacedPiece m_placed;
    i32 m_groundLevelDelta;
    std::vector<jigsaw::JigsawJunction> m_junctions;
};

} // anonymous namespace

TrialChambersStructure::TrialChambersStructure()
    : JigsawStructure(StructureType::TrialChambers,
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
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);
    if (!startPool || startPool->isEmpty()) {
        return false;
    }

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * CHUNK_WIDTH + 8, 0, chunkZ * CHUNK_WIDTH + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> TrialChambersStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);
    MC_UNUSED(generator);
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 计算起始高度：Y=-40 到 Y=-20 之间的均匀分布
    i32 startY = -30; // 默认高度
    if (m_config.startHeight) {
        valueprovider::WorldGenerationContext context(MIN_BUILD_HEIGHT, CHUNK_HEIGHT);
        startY = m_config.startHeight->sample(rng, context);
    }

    // 确保起始高度在有效范围内
    startY = std::clamp(startY, -40, -20);

    const BlockPos startPos(chunkX * CHUNK_WIDTH + 8, startY, chunkZ * CHUNK_WIDTH + 8);

    // TODO(trial_chambers): 应用池别名绑定到 JigsawManager::assemble
    // 当前 JigsawManager::assemble 尚不支持池别名参数，
    // 需要在组装前解析别名并替换模板池引用
    // auto resolvedAliases = m_config.poolAliases.resolveAllGroups(rng);

    // 使用 JigsawManager 组装试炼密室结构，maxDepth = 20
    auto placedPieces = jigsaw::JigsawManager::assemble(patternRegistry, *startPool, m_config.size, startPos, rng);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    for (auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<TrialChambersPlacedPieceAdapter>(std::move(placed)));
        }
    }

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
