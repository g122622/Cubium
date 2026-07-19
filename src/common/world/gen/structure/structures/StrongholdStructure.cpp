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

#include "StrongholdStructure.hpp"

#include "../../../../core/Constants.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../biome/BiomeIds.hpp"
#include "../../../biome/BiomeTags.hpp"
#include "../../../block/BlockPos.hpp"
#include "../StructureBoundingBox.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <cmath>

namespace mc {
namespace world {
namespace gen {
namespace structure {

namespace {

// 要塞分布环配置
// 环 0: 3 个要塞，距离 1408-2688
// 环 1: 3 个要塞，距离 4480-5760
// 环 2: 3 个要塞，距离 7552-8832
// 环 3: 4 个要塞，距离 10624-11904
// 环 4: 6 个要塞，距离 13696-14976
// 环 5: 10 个要塞，距离 16768-18048
// 环 6: 15 个要塞，距离 19840-21120
// 环 7: 21 个要塞，距离 22912-24192
// 总计: 3+3+3+4+6+10+15+21 = 65 个要塞
constexpr i32 RING_COUNTS[] = {3, 3, 3, 4, 6, 10, 15, 21};
constexpr i32 RING_DISTANCES[] = {1408, 4480, 7552, 10624, 13696, 16768, 19840, 22912};
constexpr i32 RING_SPREADS[] = {1280, 1280, 1280, 1280, 1280, 1280, 1280, 1280};

} // namespace

using namespace mc::Biomes;

const std::string StrongholdStructure::m_name = "stronghold";

StrongholdStructure::StrongholdStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

StrongholdStructure::StrongholdStructure(ResourceLocation id, const Config& config)
    : Structure(std::move(id))
    , m_config(config)
{}

const biome::BiomeTag* StrongholdStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_STRONGHOLD();
}

bool StrongholdStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(generator);
    MC_UNUSED(rng);

    /**
     * @brief 检查指定区块是否命中预计算的要塞起点。
     *
     * TODO 当前实现仍未完成 Java 版 biome locate 校正，但至少必须保证：
     * 要塞不会在任意区块都返回 true，而是只在 65 个预计算环形位置上生成。
     */
    const i64 worldSeed = static_cast<i64>(world.seed());
    for (i32 index = 0; index < 65; ++index) {
        const auto [strongholdChunkX, strongholdChunkZ] = calculateStrongholdPos(index, worldSeed);
        if (strongholdChunkX == chunkX && strongholdChunkZ == chunkZ) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<StructureStart> StrongholdStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(generator);

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算起始位置
    i32 startX = chunkX * mc::world::CHUNK_WIDTH + 8;
    i32 startZ = chunkZ * mc::world::CHUNK_WIDTH + 8;

    // 要塞生成在地下 (Y 20-40)
    i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);

    BlockPos startPos(startX, startY, startZ);

    // 使用 StrongholdPieces 系统生成要塞
    _generateStrongholdPieces(rng, startPos, start->pieces());

    start->recalculateStructureSize();

    return start;
}

void StrongholdStructure::_generateStrongholdPieces(
    math::Random& rng, const BlockPos& startPos, std::vector<std::unique_ptr<StructurePiece>>& pieces) const
{
    // 生成起始楼梯
    auto startStairs = std::make_unique<StrongholdStartStairs>(rng, startPos.x, startPos.z);
    StrongholdStartStairs* startStairsPtr = startStairs.get();
    pieces.push_back(std::move(startStairs));

    // 初始化片段权重列表
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    StrongholdPieceWeight* lastPlaced = nullptr;

    // 递归生成走廊和房间
    _generateCorridor(pieces, rng, 0, startStairsPtr);
}

void StrongholdStructure::_generateCorridor(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 depth,
    StrongholdStartStairs* start) const
{
    // 递归生成走廊和房间，直到达到最大深度或无法生成更多片段

    if (depth > 50 || pieces.size() > 100) {
        // 防止无限递归
        return;
    }

    // 获取最后添加的片段
    if (pieces.empty()) {
        return;
    }

    StructurePiece* lastPiece = pieces.back().get();
    if (lastPiece == nullptr) {
        return;
    }

    // 初始化片段权重
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    // 随机打乱权重顺序
    for (size_t i = 0; i < weights.size(); ++i) {
        size_t j = rng.nextInt(static_cast<i32>(weights.size()));
        std::swap(weights[i], weights[j]);
    }

    Direction direction = lastPiece->getCoordBaseMode();
    i32 x, y, z;

    // 根据方向计算下一个片段的位置
    switch (direction) {
        case Direction::North:
            x = lastPiece->minX() + (lastPiece->maxX() - lastPiece->minX()) / 2;
            y = lastPiece->minY();
            z = lastPiece->minZ() - 1;
            break;
        case Direction::South:
            x = lastPiece->minX() + (lastPiece->maxX() - lastPiece->minX()) / 2;
            y = lastPiece->minY();
            z = lastPiece->maxZ() + 1;
            break;
        case Direction::West:
            x = lastPiece->minX() - 1;
            y = lastPiece->minY();
            z = lastPiece->minZ() + (lastPiece->maxZ() - lastPiece->minZ()) / 2;
            break;
        case Direction::East:
            x = lastPiece->maxX() + 1;
            y = lastPiece->minY();
            z = lastPiece->minZ() + (lastPiece->maxZ() - lastPiece->minZ()) / 2;
            break;
        default:
            return;
    }

    // 尝试生成一个片段
    // 简化实现：随机选择一个片段类型
    i32 totalWeight = 0;
    for (const auto& weight : weights) {
        if (weight.canSpawnMoreStructuresOfType(depth)) {
            totalWeight += weight.weight;
        }
    }

    if (totalWeight == 0) {
        // 所有片段都达到限制，生成传送门房间
        StrongholdPortalRoom* portalRoom = StrongholdPortalRoom::createPiece(pieces, x, y, z, direction, depth);
        if (portalRoom != nullptr) {
            pieces.emplace_back(portalRoom);
        }
        return;
    }

    // 随机选择片段类型
    i32 randomValue = rng.nextInt(totalWeight);
    i32 cumulativeWeight = 0;
    i32 selectedType = StrongholdPieceTypes::STRAIGHT;

    for (auto& weight : weights) {
        if (weight.canSpawnMoreStructuresOfType(depth)) {
            cumulativeWeight += weight.weight;
            if (randomValue < cumulativeWeight) {
                selectedType = weight.pieceType;
                weight.instancesSpawned++;
                break;
            }
        }
    }

    // 创建选中的片段
    StrongholdPiece* newPiece = createStrongholdPiece(selectedType, pieces, rng, x, y, z, direction, depth);

    if (newPiece != nullptr) {
        pieces.emplace_back(newPiece);

        // 10% 概率生成图书馆，传送门房间必须生成
        if (rng.nextInt(10) == 0 && depth < 30) {
            // 可能生成图书馆
            StrongholdLibrary* library = StrongholdLibrary::createPiece(
                pieces, rng, x + rng.nextInt(8), y - rng.nextInt(5), z + rng.nextInt(8), direction, depth + 1);
            if (library != nullptr) {
                pieces.emplace_back(library);
            }
        }

        // 继续生成走廊
        _generateCorridor(pieces, rng, depth + 1, start);
    } else {
        // 无法生成更多片段，强制生成传送门房间
        if (depth > 5) {
            StrongholdPortalRoom* portalRoom = StrongholdPortalRoom::createPiece(pieces, x, y, z, direction, depth);
            if (portalRoom != nullptr) {
                pieces.emplace_back(portalRoom);
            }
        }
    }
}

std::pair<i32, i32> StrongholdStructure::calculateStrongholdPos(i32 index, i64 worldSeed)
{
    // 要塞分布算法
    // 8 个环，每个环有不同数量的要塞
    i32 ring = getRing(index);
    i32 ringIndex = index;
    for (i32 i = 0; i < ring; ++i) {
        ringIndex -= RING_COUNTS[i];
    }

    i32 count = RING_COUNTS[ring];
    i32 distance = RING_DISTANCES[ring];
    i32 spread = RING_SPREADS[ring];

    // 计算角度
    math::Random rng(worldSeed);
    [[maybe_unused]] const i32 skippedValue0 = rng.nextInt(); // 跳过一些值
    [[maybe_unused]] const i32 skippedValue1 = rng.nextInt();

    // 计算该要塞的角度
    f64 angleStep = 2.0 * mc::math::PI_DOUBLE / count;
    f64 angle = angleStep * ringIndex;

    // 添加随机偏移
    f64 randomOffset = (rng.nextDouble() - 0.5) * angleStep * 0.5;
    angle += randomOffset;

    // 计算距离
    i32 actualDistance = distance + rng.nextInt(spread);

    // 计算坐标
    i32 x = static_cast<i32>(std::cos(angle) * actualDistance);
    i32 z = static_cast<i32>(std::sin(angle) * actualDistance);

    // 转换为区块坐标
    return {x >> mc::world::CHUNK_SHIFT, z >> mc::world::CHUNK_SHIFT};
}

i32 StrongholdStructure::getRing(i32 index) noexcept
{
    i32 cumulative = 0;
    for (i32 ring = 0; ring < 8; ++ring) {
        cumulative += RING_COUNTS[ring];
        if (index < cumulative) {
            return ring;
        }
    }
    return 7;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
