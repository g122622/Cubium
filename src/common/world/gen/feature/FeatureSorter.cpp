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
 */

#include "FeatureSorter.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

std::vector<FeatureSorter::StepFeatureData> FeatureSorter::buildFeaturesPerStep(
    const std::vector<BiomeId>& possibleBiomes,
    const std::function<const std::vector<u32>&(BiomeId, DecorationStage)>& getFeatures,
    const FeatureRegistry& registry)
{
    // Step 1: 为每个 (biome, stage) 中的特征分配全局索引，并记录所属 step
    // 对应 Java: object2intmap.computeIfAbsent(placedfeature, p -> mutableint.getAndIncrement())
    std::unordered_map<u32, i32> featureIdToGlobalIndex;
    i32 nextGlobalIndex = 0;

    // 收集每个生物群系的特征序列并建立依赖图
    // 对应 Java: TreeMap<FeatureData, Set<FeatureData>> adjacencyList
    std::map<FeatureData, std::set<FeatureData>> adjacencyList;

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

            const auto& allFeatures = registry.getFeatures(stage);

            for (u32 fid : featureIds) {
                if (fid >= allFeatures.size() || allFeatures[fid] == nullptr) {
                    continue;
                }

                // 分配全局索引（如果尚未分配）
                auto it = featureIdToGlobalIndex.find(fid);
                if (it == featureIdToGlobalIndex.end()) {
                    it = featureIdToGlobalIndex.emplace(fid, nextGlobalIndex++).first;
                }

                biomeFeatures.push_back({it->second, stepIndex, fid, allFeatures[fid]});
            }
        }

        // Step 2: 建立依赖图
        // 对应 Java: for (int k = 0; k < list.size(); k++) { set2.add(list.get(k + 1)); }
        // 同一生物群系内，feature[k] 必须在 feature[k+1] 之前放置
        for (size_t k = 0; k < biomeFeatures.size(); ++k) {
            const FeatureData& node = biomeFeatures[k];
            // 确保节点在邻接表中
            if (adjacencyList.find(node) == adjacencyList.end()) {
                adjacencyList[node] = {};
            }

            if (k + 1 < biomeFeatures.size()) {
                adjacencyList[node].insert(biomeFeatures[k + 1]);
            }
        }
    }

    // Step 3: DFS 拓扑排序
    // 将 map<FeatureData, set<FeatureData>> 转换为 map<int, vector<int>> 用于 DFS
    std::unordered_map<i32, std::vector<i32>> adjByIndex;
    std::unordered_map<i32, FeatureData> indexToData;

    for (const auto& [node, neighbors] : adjacencyList) {
        indexToData[node.globalIndex] = node;
        if (adjByIndex.find(node.globalIndex) == adjByIndex.end()) {
            adjByIndex[node.globalIndex] = {};
        }
        for (const auto& neighbor : neighbors) {
            adjByIndex[node.globalIndex].push_back(neighbor.globalIndex);
            indexToData[neighbor.globalIndex] = neighbor;
        }
    }

    std::unordered_set<i32> visited;
    std::unordered_set<i32> inProgress;
    std::vector<i32> topoOrder;

    for (const auto& [nodeIdx, _] : adjByIndex) {
        if (visited.find(nodeIdx) == visited.end()) {
            if (depthFirstSearch(adjByIndex, visited, inProgress, topoOrder, nodeIdx)) {
                spdlog::warn("[FeatureSorter] Feature order cycle detected, sorting may be incomplete");
            }
        }
    }

    // 反转得到拓扑排序（DFS 后序的逆序）
    std::reverse(topoOrder.begin(), topoOrder.end());

    // Step 4: 按 step 分组
    // 对应 Java: for (int l = 0; l < i; l++) { builder.add(new StepFeatureData(list4)); }
    std::vector<StepFeatureData> result;
    result.resize(static_cast<size_t>(maxStep + 1));

    for (i32 stepIndex = 0; stepIndex <= maxStep; ++stepIndex) {
        std::vector<ConfiguredFeatureBase*> stepFeatures;
        std::vector<u32> stepFeatureIds;

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
    i32 node)
{
    if (visited.count(node)) {
        return false;
    }
    if (inProgress.count(node)) {
        // 检测到环
        return true;
    }

    inProgress.insert(node);

    auto it = adj.find(node);
    if (it != adj.end()) {
        for (i32 neighbor : it->second) {
            if (depthFirstSearch(adj, visited, inProgress, result, neighbor)) {
                return true;
            }
        }
    }

    inProgress.erase(node);
    visited.insert(node);
    result.push_back(node);
    return false;
}

} // namespace mc
