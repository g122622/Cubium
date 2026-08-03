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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 放置特征注册表
 *
 * 运行时 worldgen 注册表中的 placed_feature 注册表。
 * 从数据包 JSON 加载所有 PlacedFeature，按 ResourceLocation 索引。
 */
class PlacedFeatureRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static PlacedFeatureRegistry& instance();

    /**
     * @brief 注册放置特征
     * @param placedFeature 放置特征（转移所有权）
     */
    void registerPlacedFeature(std::unique_ptr<PlacedFeature> placedFeature);

    /**
     * @brief 按 ResourceLocation 查找放置特征
     * @return 放置特征指针，未注册返回 nullptr
     */
    [[nodiscard]] const PlacedFeature* get(const ResourceLocation& id) const noexcept;

    /**
     * @brief 是否包含指定 id
     */
    [[nodiscard]] bool has(const ResourceLocation& id) const noexcept;

    /**
     * @brief 获取所有已注册放置特征的数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_ownedPlacedFeatures.size(); }

    /**
     * @brief 清除所有放置特征
     */
    void clear();

private:
    PlacedFeatureRegistry() = default;
    ~PlacedFeatureRegistry() = default;
    PlacedFeatureRegistry(const PlacedFeatureRegistry&) = delete;
    PlacedFeatureRegistry& operator=(const PlacedFeatureRegistry&) = delete;

    std::vector<std::unique_ptr<PlacedFeature>> m_ownedPlacedFeatures;
    std::unordered_map<ResourceLocation, const PlacedFeature*> m_placedFeaturesById;
};

} // namespace mc
