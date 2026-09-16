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

#include "BiomeGenerationSettings.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <cstddef>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// BiomeGenerationSettings 实现
// ============================================================================

BiomeGenerationSettings::BiomeGenerationSettings()
{
    // 预分配阶段数量，使 getFeatures 对任意 DecorationStage 都有合法下标
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

BiomeGenerationSettings::~BiomeGenerationSettings() = default;

BiomeGenerationSettings::BiomeGenerationSettings(BiomeGenerationSettings&& other) noexcept
    : m_featuresByStage(std::move(other.m_featuresByStage))
    , m_flowerFeatureIds(std::move(other.m_flowerFeatureIds))
    , m_carvers(std::move(other.m_carvers))
{}

BiomeGenerationSettings& BiomeGenerationSettings::operator=(BiomeGenerationSettings&& other) noexcept
{
    if (this != &other) {
        m_featuresByStage = std::move(other.m_featuresByStage);
        m_flowerFeatureIds = std::move(other.m_flowerFeatureIds);
        m_carvers = std::move(other.m_carvers);
    }
    return *this;
}

void BiomeGenerationSettings::addPlacedFeature(DecorationStage stage, ResourceLocation placedFeatureId)
{
    // 构造函数已 resize 至 DecorationStage::Count，下标必然合法
    m_featuresByStage[static_cast<size_t>(stage)].push_back(std::move(placedFeatureId));
}

const std::vector<ResourceLocation>& BiomeGenerationSettings::getFeatures(DecorationStage stage) const noexcept
{
    // 构造函数已 resize 至 DecorationStage::Count，下标必然合法
    return m_featuresByStage[static_cast<size_t>(stage)];
}

bool BiomeGenerationSettings::hasPlacedFeature(const ResourceLocation& placedFeatureId) const noexcept
{
    for (const auto& features : m_featuresByStage) {
        for (const auto& id : features) {
            if (id == placedFeatureId) {
                return true;
            }
        }
    }
    return false;
}

const std::vector<ResourceLocation>& BiomeGenerationSettings::getFlowerFeatureIds() const noexcept
{
    return m_flowerFeatureIds;
}

void BiomeGenerationSettings::addFlowerFeature(ResourceLocation placedFeatureId)
{
    // 仅追加到独立花卉列表；阶段通用列表由调用方通过 addPlacedFeature 维护，
    // 避免同一 placed_feature 在阶段列表中被登记两次导致世界生成时重复放置。
    m_flowerFeatureIds.push_back(std::move(placedFeatureId));
}

void BiomeGenerationSettings::clear() noexcept
{
    for (auto& features : m_featuresByStage) {
        features.clear();
    }
    m_flowerFeatureIds.clear();
    m_carvers.clear();
}

void BiomeGenerationSettings::addCarver(ResourceLocation carverId)
{
    m_carvers.push_back(std::move(carverId));
}

const std::vector<ResourceLocation>& BiomeGenerationSettings::getCarvers() const noexcept
{
    return m_carvers;
}

} // namespace biome
} // namespace world
} // namespace mc
