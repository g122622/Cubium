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

#pragma once

#include "DecorationStage.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class PlacedFeature;
class PlacedFeatureRegistry;

/**
 * @brief 特征拓扑排序器
 *
 * 对所有可能的生物群系中的 placed_feature 进行拓扑排序，
 * 确保同一生物群系内相邻特征保持顺序依赖关系。
 *
 * 特征放置使用拓扑排序后的索引作为 setFeatureSeed 的参数，
 * 而不是简单的递增 featureIndex。这确保了跨生物群系边界的特征放置
 * 与原版一致。
 *
 * 标识体系：生物群系的 BiomeGenerationSettings 存储 placed_feature 的
 * ResourceLocation id；本排序器经 PlacedFeatureRegistry 解析 id → const PlacedFeature*，
 * 拓扑排序以 PlacedFeature* 指针为 key。
 *
 * 算法：
 * 1. 遍历所有可能的生物群系，收集每个 DecorationStage 的 placed_feature id 列表
 * 2. 为每个 placed_feature 分配全局索引
 * 3. 建立有向图：同一生物群系同一步内，feature[k] → feature[k+1]
 * 4. DFS 拓扑排序（检测环）
 * 5. 按 step 分区，每个 step 产生 StepFeatureData
 * 6. indexMapping 将 placed_feature id 映射到排序后索引
 */
class FeatureSorter {
public:
    /**
     * @brief 单个装饰步骤的特征数据
     *
     * 包含拓扑排序后的 PlacedFeature 列表和 id → sortedIndex 映射。
     */
    struct StepFeatureData {
        /** 拓扑排序后的 PlacedFeature 指针列表 */
        std::vector<const PlacedFeature*> features;

        /** placed_feature id → sortedIndex 映射（用于 setFeatureSeed） */
        std::unordered_map<ResourceLocation, i32> indexMapping;

        StepFeatureData() = default;

        /** 从特征列表和对应的 id 列表构造，自动构建 indexMapping */
        StepFeatureData(std::vector<const PlacedFeature*> sortedFeatures, std::vector<ResourceLocation> featureIds)
            : features(std::move(sortedFeatures))
        {
            for (i32 i = 0; i < static_cast<i32>(features.size()); ++i) {
                indexMapping[featureIds[static_cast<size_t>(i)]] = i;
            }
        }

        /** 获取 placed_feature 在排序后的索引，不存在返回 -1 */
        [[nodiscard]] i32 getIndex(const ResourceLocation& placedFeatureId) const
        {
            auto it = indexMapping.find(placedFeatureId);
            return it != indexMapping.end() ? it->second : -1;
        }

        /** 是否为空 */
        [[nodiscard]] bool empty() const noexcept { return features.empty(); }
    };

    /**
     * @brief 构建每个装饰步骤的拓扑排序特征数据
     *
     * @param possibleBiomes 所有可能出现的生物群系 ID 列表
     * @param getFeatures 获取指定生物群系指定阶段的 placed_feature id 列表
     * @param registry 放置特征注册表（id → const PlacedFeature*）
     * @return 按 DecorationStage 索引的 StepFeatureData 列表
     */
    static std::vector<StepFeatureData> buildFeaturesPerStep(const std::vector<BiomeId>& possibleBiomes,
        const std::function<const std::vector<ResourceLocation>&(BiomeId, DecorationStage)>& getFeatures,
        const PlacedFeatureRegistry& registry);

private:
    /**
     * @brief 内部特征数据节点
     */
    struct FeatureData {
        i32 globalIndex;              ///< 全局特征索引（在所有特征中唯一）
        i32 step;                     ///< 装饰步骤索引
        ResourceLocation featureId;   ///< placed_feature 的 ResourceLocation
        const PlacedFeature* feature; ///< PlacedFeature 指针

        bool operator<(const FeatureData& other) const
        {
            if (step != other.step) return step < other.step;
            return globalIndex < other.globalIndex;
        }
    };

    /**
     * @brief 一条检测到的环
     *
     * cycleNodes 按环的有向顺序给出参与成环的特征全局索引（首尾相同，便于阅读）。
     */
    struct CycleInfo {
        std::vector<i32> cycleNodes; ///< 环中节点（globalIndex），首尾相同
    };

    /**
     * @brief DFS 拓扑排序（检测环）
     */
    static bool depthFirstSearch(const std::unordered_map<i32, std::vector<i32>>& adj,
        std::unordered_set<i32>& visited,
        std::unordered_set<i32>& inProgress,
        std::vector<i32>& result,
        std::vector<i32>& path,
        std::vector<CycleInfo>& cycles,
        i32 node);

    /**
     * @brief 把单个特征节点格式化为人类可读描述
     *
     * 输出形如 "monster_room@underground_structures(#42)"。
     */
    static std::string _formatNode(const FeatureData& data);

    /**
     * @brief 把生物群系 ID 格式化为 "name(id)"
     */
    static std::string _formatBiome(BiomeId biomeId);
};

} // namespace mc
