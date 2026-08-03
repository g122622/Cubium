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

#include "server/world/structure/StructureLocator.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/StructureCheck.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/StructureTags.hpp"
#include "common/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "common/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

#include <limits>
#include <optional>

namespace mc::server::structure {

std::optional<BlockPos> StructureLocator::findNearestStructure(const ServerWorld& world,
    const BlockPos& center,
    const ResourceLocation& structureId,
    i32 maxDistance,
    bool skipExisting)
{
    // 通过结构 ID 查找所属的 StructureSet，获取放置规则
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();
    const world::gen::structure::StructureSet* structureSet = structureSetRegistry.findByStructure(structureId);
    if (structureSet == nullptr) {
        return std::nullopt;
    }

    const auto& placement = structureSet->placement();
    const i64 worldSeed = static_cast<i64>(world.seed());

    // 获取 StructureCheck 缓存（用于快速跳过不含结构的区块）
    const world::gen::structure::StructureCheck* structureCheck = nullptr;
    if (const auto* cm = world.chunkManager()) {
        if (const auto* gen = cm->generator()) {
            structureCheck = gen->structureCheck();
        }
    }

    // 将方块坐标转换为区块坐标
    const i32 centerChunkX = center.x >> 4;
    const i32 centerChunkZ = center.z >> 4;

    // 将最大搜索距离转换为区块范围
    const i32 chunkRadius = (maxDistance + 15) >> 4; // 向上取整到区块

    std::optional<BlockPos> nearestPos;
    f64 nearestDistSq = static_cast<f64>(maxDistance * maxDistance) + 1.0;

    // 根据放置策略类型使用不同的搜索算法
    const auto* randomSpread =
        dynamic_cast<const world::gen::structure::placement::RandomSpreadStructurePlacement*>(&placement);
    const auto* concentricRings =
        dynamic_cast<const world::gen::structure::placement::ConcentricRingsStructurePlacement*>(&placement);

    if (randomSpread != nullptr) {
        // RandomSpread：网格搜索，使用 getPotentialStructureChunk 计算候选区块
        const i32 spacing = randomSpread->spacing();

        const i32 minGridX = (centerChunkX - chunkRadius) / spacing - 1;
        const i32 maxGridX = (centerChunkX + chunkRadius) / spacing + 1;
        const i32 minGridZ = (centerChunkZ - chunkRadius) / spacing - 1;
        const i32 maxGridZ = (centerChunkZ + chunkRadius) / spacing + 1;

        for (i32 gridX = minGridX; gridX <= maxGridX; ++gridX) {
            for (i32 gridZ = minGridZ; gridZ <= maxGridZ; ++gridZ) {
                const i32 baseChunkX = gridX * spacing;
                const i32 baseChunkZ = gridZ * spacing;

                // 使用放置规则计算此网格中的候选区块
                const auto candidate = randomSpread->getPotentialStructureChunk(worldSeed, baseChunkX, baseChunkZ);

                // 检查候选区块距离是否在搜索范围内
                const i32 dx = candidate.x - centerChunkX;
                const i32 dz = candidate.z - centerChunkZ;
                if (dx * dx + dz * dz > chunkRadius * chunkRadius) {
                    continue;
                }

                // 验证此候选区块是否真正生成结构（频率缩减 + 排斥区检查）
                if (!placement.isStructureChunk(worldSeed, candidate.x, candidate.z)) {
                    continue;
                }

                // StructureCheck 缓存快速判断区块是否包含目标结构
                if (structureCheck != nullptr) {
                    const u64 chunkPosId = (static_cast<u64>(static_cast<u32>(candidate.x)) << 32) |
                        static_cast<u64>(static_cast<u32>(candidate.z));
                    const auto result = structureCheck->checkStart(chunkPosId, structureId, skipExisting);

                    if (result == world::gen::structure::StructureCheckResult::StartPresent) {
                        // 精确缓存命中：结构存在于该区块，直接返回位置
                        const BlockPos locatePos = placement.getLocatePos(candidate);
                        const i32 posDx = locatePos.x - center.x;
                        const i32 posDz = locatePos.z - center.z;
                        const f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                        if (distSq < nearestDistSq) {
                            nearestDistSq = distSq;
                            nearestPos = locatePos;
                        }
                        continue;
                    }

                    if (result == world::gen::structure::StructureCheckResult::StartNotPresent) {
                        // 精确缓存或近似缓存确认该区块不含目标结构，跳过
                        continue;
                    }

                    // ChunkLoadNeeded：缓存未命中，继续执行当前逻辑（基于放置规则判断）
                    // 将放置规则检查结果写入近似缓存，供后续查询使用
                    structureCheck->setFeatureCheckResult(chunkPosId, true);
                }

                // 使用放置规则的定位偏移计算最终方块位置
                const BlockPos locatePos = placement.getLocatePos(candidate);
                const i32 posDx = locatePos.x - center.x;
                const i32 posDz = locatePos.z - center.z;
                const f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    nearestPos = locatePos;
                }
            }
        }
    } else if (concentricRings != nullptr) {
        // ConcentricRings（要塞）：直接获取所有预计算位置，找最近的
        const auto& ringPositions = concentricRings->getRingPositions(worldSeed);
        for (const auto& chunkPos : ringPositions) {
            const i32 dx = chunkPos.x - centerChunkX;
            const i32 dz = chunkPos.z - centerChunkZ;
            if (dx * dx + dz * dz > chunkRadius * chunkRadius) {
                continue;
            }

            if (structureCheck != nullptr) {
                const u64 chunkPosId = (static_cast<u64>(static_cast<u32>(chunkPos.x)) << 32) |
                    static_cast<u64>(static_cast<u32>(chunkPos.z));
                const auto result = structureCheck->checkStart(chunkPosId, structureId, skipExisting);

                if (result == world::gen::structure::StructureCheckResult::StartPresent) {
                    const BlockPos locatePos = placement.getLocatePos(chunkPos);
                    const i32 posDx = locatePos.x - center.x;
                    const i32 posDz = locatePos.z - center.z;
                    const f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                    if (distSq < nearestDistSq) {
                        nearestDistSq = distSq;
                        nearestPos = locatePos;
                    }
                    continue;
                }

                if (result == world::gen::structure::StructureCheckResult::StartNotPresent) {
                    continue;
                }
            }

            const BlockPos locatePos = placement.getLocatePos(chunkPos);
            const i32 posDx = locatePos.x - center.x;
            const i32 posDz = locatePos.z - center.z;
            const f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestPos = locatePos;
            }
        }
    }

    return nearestPos;
}

std::optional<BlockPos> StructureLocator::findNearestMapStructure(
    const ServerWorld& world, const BlockPos& center, const ResourceLocation& tagId, i32 maxDistance, bool skipExisting)
{
    // 通过结构标签 ID 查找标签，遍历标签中的所有结构 ID，对每个结构调用 findNearestStructure，
    // 返回所有候选中距离最近的位置。
    const auto* tag = world::gen::structure::StructureTags::getTag(tagId);
    if (tag == nullptr) {
        spdlog::warn("findNearestMapStructure: unknown structure tag '{}', returning empty", tagId.toString());
        return std::nullopt;
    }

    if (tag->getStructureIds().empty()) {
        return std::nullopt;
    }

    std::optional<BlockPos> nearestPos;
    f64 nearestDistSq = std::numeric_limits<f64>::max();

    for (const auto& structureId : tag->getStructureIds()) {
        auto candidatePos = findNearestStructure(world, center, structureId, maxDistance, skipExisting);
        if (!candidatePos.has_value()) {
            continue;
        }

        const i32 dx = candidatePos->x - center.x;
        const i32 dz = candidatePos->z - center.z;
        const f64 distSq = static_cast<f64>(dx * dx + dz * dz);

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestPos = candidatePos;
        }
    }

    return nearestPos;
}

} // namespace mc::server::structure
