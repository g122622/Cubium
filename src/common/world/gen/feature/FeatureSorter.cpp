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

#include "FeatureSorter.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include "common/world/gen/placement/PlacedFeatureRegistry.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc {

std::vector<FeatureSorter::StepFeatureData> FeatureSorter::buildFeaturesPerStep(
    const std::vector<BiomeId>& possibleBiomes,
    const std::function<const std::vector<ResourceLocation>&(BiomeId, DecorationStage)>& getFeatures,
    const PlacedFeatureRegistry& registry)
{
    // Step 1: 为每个 placed_feature 分配全局索引，并记录所属 step
    // 使用 const PlacedFeature* 作为 key 确保跨 stage 的相同 id 不会冲突
    // （不同 stage 的 placed_feature 拥有不同的 ResourceLocation，天然不冲突）
    std::unordered_map<const PlacedFeature*, i32> featureToGlobalIndex;
    i32 nextGlobalIndex = 0;

    // 收集每个生物群系的特征序列并建立依赖图
    std::map<FeatureData, std::set<FeatureData>> adjacencyList;

    // 成环诊断用：保留每个生物群系按生成顺序的特征序列，用于在检测到环后
    // 反查"是哪些生物群系把环中相邻特征串成了 feature[k]→feature[k+1] 的依赖"。
    // key 为生物群系在 possibleBiomes 中的下标，value 为该生物群系按生成顺序的特征节点。
    std::vector<std::pair<BiomeId, std::vector<FeatureData>>> biomeFeatureSequences;
    biomeFeatureSequences.reserve(possibleBiomes.size());

    i32 maxStep = 0;

    for (BiomeId biomeId : possibleBiomes) {
        std::vector<FeatureData> biomeFeatures;

        for (i32 stepIndex = 0; stepIndex < static_cast<i32>(DecorationStage::Count); ++stepIndex) {
            DecorationStage stage = static_cast<DecorationStage>(stepIndex);
            const auto& featureIds = getFeatures(biomeId, stage);
            if (featureIds.empty()) {
                continue;
            }

            maxStep = std::max(maxStep, stepIndex);

            for (const ResourceLocation& placedFeatureId : featureIds) {
                const PlacedFeature* feature = registry.get(placedFeatureId);
                if (feature == nullptr) {
                    // 该 placed_feature 未注册（数据驱动缺口），跳过但不中断排序
                    continue;
                }

                // 分配全局索引（如果尚未分配）——以 PlacedFeature 指针为 key
                auto it = featureToGlobalIndex.find(feature);
                if (it == featureToGlobalIndex.end()) {
                    it = featureToGlobalIndex.emplace(feature, nextGlobalIndex++).first;
                }

                biomeFeatures.push_back({it->second, stepIndex, placedFeatureId, feature});
            }
        }

        // Step 2: 建立依赖图
        // 同一生物群系内，feature[k] 必须在 feature[k+1] 之前放置
        // operator[] 为每个节点（含无后继的叶子节点）创建邻接集条目，
        // 保证后续 Step 3 遍历能覆盖全部节点
        for (size_t k = 0; k < biomeFeatures.size(); ++k) {
            const FeatureData& node = biomeFeatures[k];
            auto& neighbors = adjacencyList[node];
            if (k + 1 < biomeFeatures.size()) {
                neighbors.insert(biomeFeatures[k + 1]);
            }
        }

        // 仅保留非空序列，用于后续成环反查；空序列对溯源无意义
        if (!biomeFeatures.empty()) {
            biomeFeatureSequences.emplace_back(biomeId, std::move(biomeFeatures));
        }
    }

    // Step 3: DFS 拓扑排序
    // 将 map<FeatureData, set<FeatureData>> 转换为 map<int, vector<int>> 用于 DFS
    std::unordered_map<i32, std::vector<i32>> adjByIndex;
    std::unordered_map<i32, FeatureData> indexToData;

    for (const auto& [node, neighbors] : adjacencyList) {
        indexToData[node.globalIndex] = node;
        auto& idxNeighbors = adjByIndex[node.globalIndex];
        for (const auto& neighbor : neighbors) {
            idxNeighbors.push_back(neighbor.globalIndex);
            indexToData[neighbor.globalIndex] = neighbor;
        }
    }

    std::unordered_set<i32> visited;
    std::unordered_set<i32> inProgress;
    std::vector<i32> topoOrder;
    std::vector<i32> dfsPath;      // 当前 DFS 递归路径（用于回溯成环节点）
    std::vector<CycleInfo> cycles; // 检测到的所有环

    bool hasCycle = false;

    for (const auto& [nodeIdx, _] : adjByIndex) {
        if (visited.find(nodeIdx) == visited.end()) {
            if (depthFirstSearch(adjByIndex, visited, inProgress, topoOrder, dfsPath, cycles, nodeIdx)) {
                hasCycle = true;
            }
        }
    }

    if (hasCycle) {
        // 输出友好的成环诊断：展示每条环的参与者（特征名@阶段#全局索引 链）
        // 以及把这些特征串成依赖的生物群系来源（成因）。
        // 汇总所有环的诊断信息，统一断言中断（与原版抛 IllegalStateException 行为一致）。
        std::string allChains;
        std::string allSources;

        for (const CycleInfo& cycle : cycles) {
            // 环节点链：A -> B -> C -> A
            std::string chain;
            for (size_t i = 0; i < cycle.cycleNodes.size(); ++i) {
                auto it = indexToData.find(cycle.cycleNodes[i]);
                if (it != indexToData.end()) {
                    chain += _formatNode(it->second);
                } else {
                    chain += fmt::format("(#{})", cycle.cycleNodes[i]);
                }
                if (i + 1 < cycle.cycleNodes.size()) {
                    chain += " -> ";
                }
            }

            // 反查成因：环中每条相邻依赖 (cycle[k] -> cycle[k+1]) 是由哪些生物群系
            // 在其生成序列中以相邻 feature[k]→feature[k+1] 的形式建立的。
            std::string sources;
            for (size_t k = 0; k + 1 < cycle.cycleNodes.size(); ++k) {
                i32 fromIdx = cycle.cycleNodes[k];
                i32 toIdx = cycle.cycleNodes[k + 1];
                for (const auto& [biomeId, features] : biomeFeatureSequences) {
                    for (size_t j = 0; j + 1 < features.size(); ++j) {
                        if (features[j].globalIndex == fromIdx && features[j + 1].globalIndex == toIdx) {
                            if (!sources.empty()) {
                                sources += ", ";
                            }
                            sources += fmt::format("{} ({} -> {})",
                                _formatBiome(biomeId),
                                _formatNode(features[j]),
                                _formatNode(features[j + 1]));
                            break; // 同一生物群系内同一条边只记一次
                        }
                    }
                }
            }

            if (!allChains.empty()) {
                allChains += " | ";
            }
            allChains += chain;
            if (!sources.empty()) {
                if (!allSources.empty()) {
                    allSources += " | ";
                }
                allSources += sources;
            }

            spdlog::error("[FeatureSorter]   cycle: {}", chain);
            if (!sources.empty()) {
                spdlog::error("[FeatureSorter]   involved biomes (edge source): {}", sources);
            } else {
                spdlog::error("[FeatureSorter]   involved biomes (edge source): <unresolved>");
            }
        }

        spdlog::error("[FeatureSorter] Feature order cycle detected, aborting generation.");
        // 成环属于数据包配置错误（feature 依赖关系存在循环），必须中断生成。
        // 原版在 FeatureSorter.java 中抛 IllegalStateException 中断，项目沿用此严格语义，
        // 避免在拓扑顺序不完整的情况下继续生成导致不可预期的世界状态。
        const std::string assertMsg =
            fmt::format("[FeatureSorter] Feature order cycle detected. Cycles: [{}]. Involved biomes: [{}]",
                allChains,
                allSources.empty() ? std::string("<unresolved>") : allSources);
        MC_ASSERT_RELEASE_MSG(false, assertMsg.c_str());
    }

    // 反转得到拓扑排序（DFS 后序的逆序）
    std::reverse(topoOrder.begin(), topoOrder.end());

    // Step 4: 按 step 分组
    std::vector<StepFeatureData> result;
    result.resize(static_cast<size_t>(maxStep + 1));

    for (i32 stepIndex = 0; stepIndex <= maxStep; ++stepIndex) {
        std::vector<const PlacedFeature*> stepFeatures;
        std::vector<ResourceLocation> stepFeatureIds;

        for (i32 globalIdx : topoOrder) {
            auto dataIt = indexToData.find(globalIdx);
            if (dataIt != indexToData.end() && dataIt->second.step == stepIndex) {
                stepFeatures.push_back(dataIt->second.feature);
                stepFeatureIds.push_back(dataIt->second.featureId);
            }
        }

        result[static_cast<size_t>(stepIndex)] = StepFeatureData(std::move(stepFeatures), std::move(stepFeatureIds));
    }

    return result;
}

bool FeatureSorter::depthFirstSearch(const std::unordered_map<i32, std::vector<i32>>& adj,
    std::unordered_set<i32>& visited,
    std::unordered_set<i32>& inProgress,
    std::vector<i32>& result,
    std::vector<i32>& path,
    std::vector<CycleInfo>& cycles,
    i32 node)
{
    if (visited.count(node)) {
        return false;
    }
    if (inProgress.count(node)) {
        // 检测到回边：当前 node 仍在递归栈中，path 中从首次出现 node 处到栈顶构成一条环。
        CycleInfo info;
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == node) {
                // path[i..end] 即为环上节点，补回起点形成闭合
                for (size_t j = i; j < path.size(); ++j) {
                    info.cycleNodes.push_back(path[j]);
                }
                info.cycleNodes.push_back(node);
                break;
            }
        }
        if (!info.cycleNodes.empty()) {
            cycles.push_back(std::move(info));
        }
        return true;
    }

    inProgress.insert(node);
    path.push_back(node);

    bool hasCycle = false;
    auto it = adj.find(node);
    if (it != adj.end()) {
        for (i32 neighbor : it->second) {
            if (depthFirstSearch(adj, visited, inProgress, result, path, cycles, neighbor)) {
                hasCycle = true;
            }
        }
    }

    inProgress.erase(node);
    visited.insert(node);
    path.pop_back();
    result.push_back(node);
    return hasCycle;
}

std::string FeatureSorter::_formatNode(const FeatureData& data)
{
    // 显示 placed_feature 的 ResourceLocation @ 装饰阶段 (#全局索引)
    const std::string featureName = data.feature ? data.feature->id().toString() : std::string("<null>");
    const char* stageName = DecorationStages::getName(static_cast<DecorationStage>(data.step));
    return fmt::format("{}@{}(#{})", featureName, stageName, data.globalIndex);
}

std::string FeatureSorter::_formatBiome(BiomeId biomeId)
{
    const world::biome::Biome& biome = world::biome::BiomeRegistry::instance().get(biomeId);
    return fmt::format("{}({})", biome.name(), static_cast<u32>(biomeId));
}

} // namespace mc
