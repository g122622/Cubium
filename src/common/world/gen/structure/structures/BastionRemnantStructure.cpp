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
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../../jigsaw/JigsawPattern.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string BastionRemnantStructure::s_name = "bastion_remnant";
// MC 1.16.5: 堡垒遗迹在除玄武岩三角洲外的所有下界生物群系中生成
const std::vector<BiomeId> BastionRemnantStructure::s_validBiomes = {
    NetherWastes, CrimsonForest, WarpedForest, SoulSandValley};

namespace {

/**
 * @brief 堡垒遗迹片段适配器
 *
 * 将 PlacedPiece 适配为 StructurePiece，用于存储到 StructureStart。
 * 存储 JigsawJunction 用于地形平滑计算。
 */
class BastionPlacedPieceAdapter final : public StructurePiece {
public:
    explicit BastionPlacedPieceAdapter(const jigsaw::PlacedPiece& placed)
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

    void generate(IWorldWriter&,
        math::Random&,
        i32,
        i32,
        const StructureBoundingBox&) override
    {
        // 实际方块放置已在 JigsawManager::placePieceRecursive 中完成
    }

    [[nodiscard]] i32 getGroundLevelDelta() const override { return m_groundLevelDelta; }

    [[nodiscard]] const std::vector<jigsaw::JigsawJunction>& getJunctions() const override
    {
        return m_junctions;
    }

    [[nodiscard]] bool isJigsawPiece() const override { return true; }

private:
    i32 m_groundLevelDelta;
    std::vector<jigsaw::JigsawJunction> m_junctions;
};

// MC 1.16.5: 堡垒遗迹4种类型的起始池路径
// 参考 BastionRemnantsPieces.java
constexpr const char* BASTION_START_POOLS[] = {
    "minecraft:bastion/units/start",     // 单元型 (权重最高)
    "minecraft:bastion/stables/start",   // 猪灵兽栏
    "minecraft:bastion/treasure/start",  // 宝藏型
    "minecraft:bastion/bridge/start"     // 桥梁型
};

// MC 1.16.5: 各类型的选择权重 (bastion/units 权重最高)
// 参考 BastionRemnantsPieces.PIECE_WEIGHTS
constexpr i32 BASTION_WEIGHTS[] = {4, 2, 2, 2}; // units, stables, treasure, bridge

} // anonymous namespace

BastionRemnantStructure::BastionRemnantStructure()
    : Structure(StructureType::Bastion)
{}

bool BastionRemnantStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系 - 堡垒遗迹在所有下界生物群系中生成（玄武岩三角洲除外）
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> BastionRemnantStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // MC 1.16.5: 随机选择堡垒类型
    // 参考 BastionRemnantsPieces.random()
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
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(startPoolLocation);

    if (!startPool || startPool->isEmpty()) {
        spdlog::debug("[BastionRemnantStructure] Start pool not found: {}", startPoolLocation.toString());
        return start;
    }

    // MC 1.16.5: 堡垒遗迹生成在下界的固定高度范围 (Y: 30-80)
    // 参考 BastionRemnantsStructure.java - 在下界生成，不需要地形高度查询
    const i32 startY = 60; // 中等高度

    BlockPos startPos(chunkX * 16 + 8, startY, chunkZ * 16 + 8);

    // MC 1.16.5: 使用 JigsawManager 组装堡垒结构
    // maxDepth = 7 是 MC 默认值
    auto placedPieces = jigsaw::JigsawManager::assemble(patternRegistry, *startPool, 7, startPos, rng);

    // 为每个 PlacedPiece 创建适配器并添加到 StructureStart
    for (const auto& placed : placedPieces) {
        if (placed.piece && !placed.piece->isEmpty()) {
            start->addPiece(std::make_unique<BastionPlacedPieceAdapter>(placed));
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
