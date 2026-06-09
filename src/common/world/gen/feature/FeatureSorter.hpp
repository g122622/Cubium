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

#pragma once

#include "ConfiguredFeature.hpp"
#include "DecorationStage.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

/**
 * @brief 特征拓扑排序器（MC 1.21 FeatureSorter）
 *
 * 对所有可能的生物群系中的特征进行拓扑排序，
 * 确保同一生物群系内相邻特征保持顺序依赖关系。
 *
 * MC 1.21 的特征放置使用拓扑排序后的索引作为 setFeatureSeed 的参数，
 * 而不是简单的递增 featureIndex。这确保了跨生物群系边界的特征放置
 * 与原版一致。
 *
 * 算法：
 * 1. 遍历所有可能的生物群系，收集每个 DecorationStage 的特征列表
 * 2. 为每个特征分配全局索引
 * 3. 建立有向图：同一生物群系同一步内，feature[k] → feature[k+1]
 * 4. DFS 拓扑排序（检测环）
 * 5. 按 step 分区，每个 step 产生 StepFeatureData
 * 6. indexMapping 将 featureId 映射到排序后索引
 */
class FeatureSorter {
public:
    /**
     * @brief 单个装饰步骤的特征数据
     *
     * 对应 Java FeatureSorter.StepFeatureData。
     * 包含拓扑排序后的特征列表和 featureId → sortedIndex 映射。
     */
    struct StepFeatureData {
        /** 拓扑排序后的特征指针列表 */
        std::vector<ConfiguredFeatureBase*> features;

        /** featureId → sortedIndex 映射（用于 setFeatureSeed） */
        std::unordered_map<u32, i32> indexMapping;

        StepFeatureData() = default;

        /** 从特征列表和对应的 featureId 列表构造，自动构建 indexMapping */
        StepFeatureData(std::vector<ConfiguredFeatureBase*> sortedFeatures, std::vector<u32> featureIds)
            : features(std::move(sortedFeatures))
        {
            for (i32 i = 0; i < static_cast<i32>(features.size()); ++i) {
                indexMapping[featureIds[static_cast<size_t>(i)]] = i;
            }
        }

        /** 获取特征在排序后的索引，不存在返回 -1 */
        [[nodiscard]] i32 getIndex(u32 featureId) const
        {
            auto it = indexMapping.find(featureId);
            return it != indexMapping.end() ? it->second : -1;
        }

        /** 是否为空 */
        [[nodiscard]] bool empty() const noexcept { return features.empty(); }
    };

    /**
     * @brief 构建每个装饰步骤的拓扑排序特征数据
     *
     * 对应 Java FeatureSorter.buildFeaturesPerStep()。
     *
     * @param possibleBiomes 所有可能出现的生物群系 ID 列表
     * @param getFeatures 获取指定生物群系指定阶段的特征 ID 列表
     * @param registry 特征注册表
     * @return 按 DecorationStage 索引的 StepFeatureData 列表
     */
    static std::vector<StepFeatureData> buildFeaturesPerStep(const std::vector<BiomeId>& possibleBiomes,
        const std::function<const std::vector<u32>&(BiomeId, DecorationStage)>& getFeatures,
        const FeatureRegistry& registry);

private:
    /**
     * @brief 内部特征数据节点
     *
     * 对应 Java FeatureSorter.FeatureData record。
     */
    struct FeatureData {
        i32 globalIndex;                ///< 全局特征索引（在所有特征中唯一）
        i32 step;                       ///< 装饰步骤索引
        u32 featureId;                  ///< 特征 ID（在注册表中的索引）
        ConfiguredFeatureBase* feature; ///< 特征指针

        bool operator<(const FeatureData& other) const
        {
            if (step != other.step) return step < other.step;
            return globalIndex < other.globalIndex;
        }
    };

    /**
     * @brief DFS 拓扑排序（检测环）
     *
     * 对应 Java Graph.depthFirstSearch。
     *
     * @param adj 邻接表（全局索引 → 邻居全局索引列表）
     * @param visited 已完成节点集合
     * @param inProgress 正在处理节点集合
     * @param result 拓扑排序结果（逆后序）
     * @param node 当前节点
     * @return 是否检测到环
     */
    static bool depthFirstSearch(const std::unordered_map<i32, std::vector<i32>>& adj,
        std::unordered_set<i32>& visited,
        std::unordered_set<i32>& inProgress,
        std::vector<i32>& result,
        i32 node);
};

} // namespace mc
