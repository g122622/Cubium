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

#include "PanelStatsCollector.hpp"

#include "common/profiler/TraceEvents.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/dimension/Dimension.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::server::console {

using namespace mc::trace;

namespace {

/// 统计方块/群系时纵向抽样的步长（区块 16x16 水平面，纵向按 4 格一采样）。
/**
 * 逐方块统计的代价 = 区块数 × 384 层 × 256 列，对多维度会到千万级；面板只需要"概貌"，
 * 故按固定步长抽样：水平方向全采（16x16 的分布最有意义），纵向每 4 格取一层，
 * 且只统计非空气方块。抽样比例在输出里明示，避免读者误以为是精确计数。
 */
constexpr i32 VERTICAL_SAMPLE_STEP = 4;

/// 维度标签：去掉 "minecraft:" 前缀，节省面板宽度。
[[nodiscard]] std::string _dimensionLabel(const Dimension& dimension)
{
    return stripNamespace(dimension.type().name());
}

[[nodiscard]] std::string _biomeLabel(BiomeId id)
{
    return stripNamespace(world::biome::BiomeRegistry::instance().get(id).name());
}

} // namespace

PanelStatsCollector::PanelStatsCollector(MinecraftServer& server, PanelStatsOptions options)
    : m_server(server)
    , m_options(options)
{}

std::vector<std::string> PanelStatsCollector::collect()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "PanelStatsCollector::collect");

    std::vector<std::string> lines;
    m_server.dimensionManager().forEachDimension([this, &lines](Dimension& dimension) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "PanelStatsCollector::scanDimension");
        auto* serverDimension = static_cast<ServerDimension*>(&dimension);
        ServerWorld* world = serverDimension->world();
        if (world == nullptr) {
            return;
        }

        const std::string label = _dimensionLabel(dimension);
        lines.push_back(" [" + label + "]");

        const ScanResult result = _scanWorld(*world);

        lines.push_back("  chunks");
        for (auto& line : formatChunkStatusLines(result.status)) {
            lines.push_back(" " + line);
        }
        if (result.totalChunks > result.scannedChunks) {
            lines.push_back(fmt::format("  (sampled {} of {} chunks)",
                formatThousands(static_cast<i64>(result.scannedChunks)),
                formatThousands(static_cast<i64>(result.totalChunks))));
        }

        lines.push_back("  blocks (y-step " + std::to_string(VERTICAL_SAMPLE_STEP) + ", non-air)");
        for (auto& line : formatRankedCounts(result.blockCounts, m_options.topEntries, 0)) {
            lines.push_back(" " + line);
        }

        lines.push_back("  biomes");
        for (auto& line : formatRankedCounts(result.biomeCounts, m_options.topEntries, 0)) {
            lines.push_back(" " + line);
        }
    });

    if (lines.empty()) {
        lines.push_back(" no dimension available");
    }
    return lines;
}

PanelStatsCollector::ScanResult PanelStatsCollector::_scanWorld(ServerWorld& world)
{
    ScanResult result;

    auto* chunkManager = world.chunkManager();
    if (chunkManager == nullptr) {
        return result;
    }

    const i32 simulationDistance = world.entityManager().simulationDistance();
    std::unordered_map<std::string, i64> blockCounts;
    std::unordered_map<std::string, i64> biomeCounts;

    // 先在锁外拿到区块列表，再逐个扫描：forEachLoadedChunk 内部已做快照拷贝。
    chunkManager->forEachLoadedChunk([&](const ChunkData& chunk) {
        ++result.totalChunks;
        if (result.scannedChunks >= m_options.maxChunksScanned) {
            return true;
        }
        ++result.scannedChunks;

        ++result.status.loadedChunks;
        if (chunkManager->ticketManager().isTickingChunk(chunk.x(), chunk.z(), simulationDistance)) {
            ++result.status.tickingChunks;
        }
        if (chunk.isPostProcessingDone()) {
            ++result.status.postProcessedChunks;
        }
        result.status.loadedEntities += chunk.loadedEntities().size();

        // 方块分布：固定抽样网格 + 非空气过滤。
        const i32 baseX = chunk.x() * world::CHUNK_WIDTH;
        const i32 baseZ = chunk.z() * world::CHUNK_WIDTH;
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; y += VERTICAL_SAMPLE_STEP) {
                    const auto* state = chunk.getBlockState(baseX + x, y, baseZ + z);
                    if (state == nullptr || state->isAir()) {
                        continue;
                    }
                    ++blockCounts[describeBlockState(state)];
                }
            }
        }

        // 群系分布：每个象限中心各取一次，兼顾同区块内跨群系的情况。
        for (i32 quadrantX = 0; quadrantX < 2; ++quadrantX) {
            for (i32 quadrantZ = 0; quadrantZ < 2; ++quadrantZ) {
                const BiomeId biome =
                    chunk.getBiomeAtBlock(baseX + quadrantX * world::CHUNK_WIDTH / 2 + world::CHUNK_WIDTH / 4,
                        64,
                        baseZ + quadrantZ * world::CHUNK_WIDTH / 2 + world::CHUNK_WIDTH / 4);
                ++biomeCounts[_biomeLabel(biome)];
            }
        }
        return true;
    });

    result.blockCounts.reserve(blockCounts.size());
    for (auto& [name, count] : blockCounts) {
        result.blockCounts.emplace_back(name, count);
    }
    result.biomeCounts.reserve(biomeCounts.size());
    for (auto& [name, count] : biomeCounts) {
        result.biomeCounts.emplace_back(name, count);
    }
    return result;
}

} // namespace mc::server::console
