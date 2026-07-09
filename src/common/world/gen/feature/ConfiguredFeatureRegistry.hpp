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
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 配置化特征注册表
 *
 * 运行时 worldgen 注册表中的 configured_feature 注册表。
 * 从数据包 JSON 加载所有 ConfiguredFeatureBase，按 ResourceLocation 索引。
 *
 * 取代旧的 FeatureRegistry（按 u32 featureId 索引、按 stage 组织）。
 */
class ConfiguredFeatureRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static ConfiguredFeatureRegistry& instance();

    /**
     * @brief 注册配置化特征
     * @param feature 特征（转移所有权）
     * @param id 特征的 ResourceLocation（对应 configured_feature JSON 文件名）
     */
    void registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, ResourceLocation id);

    /**
     * @brief 按 ResourceLocation 查找配置化特征
     * @return 特征指针，未注册返回 nullptr
     */
    [[nodiscard]] const ConfiguredFeatureBase* get(const ResourceLocation& id) const noexcept;

    /**
     * @brief 是否包含指定 id
     */
    [[nodiscard]] bool has(const ResourceLocation& id) const noexcept;

    /**
     * @brief 获取所有已注册特征的数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_ownedFeatures.size(); }

    /**
     * @brief 清除所有特征
     */
    void clear();

private:
    ConfiguredFeatureRegistry() = default;
    ~ConfiguredFeatureRegistry() = default;
    ConfiguredFeatureRegistry(const ConfiguredFeatureRegistry&) = delete;
    ConfiguredFeatureRegistry& operator=(const ConfiguredFeatureRegistry&) = delete;

    // 拥有所有权的特征对象
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> m_ownedFeatures;

    // ResourceLocation → 特征指针
    std::unordered_map<ResourceLocation, const ConfiguredFeatureBase*> m_featuresById;
};

} // namespace mc
