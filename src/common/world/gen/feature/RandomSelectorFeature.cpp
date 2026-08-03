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

#include "RandomSelectorFeature.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include "common/world/gen/placement/PlacedFeatureRegistry.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature {

namespace {
/// @brief 解析 random_selector 子特征引用并委派放置。
///
/// MC 1.21.11 中 RandomFeatureConfiguration.features[]/default 均为 PlacedFeature 引用，
/// 命中时调用其 place(origin)——即先走该 PlacedFeature 自带的 placement 链，再放置其配置化特征。
///
/// 项目数据包存在两种写法（均为合法 vanilla 形式）：
///  - 字符串 id 指向已注册的 placed_feature（如 "minecraft:spruce_checked"、"minecraft:oak_checked"）；
///  - 内联对象 {"feature":"<configured_id>","placement":[]}，加载器提取出 configured id
///    （如 "minecraft:oak_bees_005"，无对应 placed_feature 文件）。
/// 因此这里先查 PlacedFeatureRegistry（vanilla 主路径，会重跑子链），未命中再查
/// ConfiguredFeatureRegistry（兼容内联 configured 形式，空 placement 等价直接放置）。
/// 两者都未命中视为引用错误，返回 false。
bool dispatchChildFeature(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const ResourceLocation& id)
{
    if (const PlacedFeature* placed = PlacedFeatureRegistry::instance().get(id); placed != nullptr) {
        return placed->place(region, chunk, generator, random, pos);
    }
    if (const ConfiguredFeatureBase* configured = ConfiguredFeatureRegistry::instance().get(id);
        configured != nullptr) {
        return configured->place(region, chunk, generator, random, pos);
    }
    // 引用的子特征既不在 placed 也不在 configured 注册表中：数据错误，静默失败。
    // （历史原因：此处曾只查 configured 注册表，导致所有以 placed_feature id 作 default
    //  的 trees_*（snowy/taiga/jungle/savanna/water/...）全部 default-miss，树木不生成。）
    return false;
}
} // namespace

// ============================================================================
// RandomSelectorFeature
// ============================================================================

bool RandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RandomSelectorFeatureConfig& config)
{
    // 顺序概率检查：遍历 features，每项 nextFloat() < chance 即命中委派并返回。
    // nextFloat() ∈ [0.0, 1.0)，故 chance=1.0 必触发，chance=0.0 必不触发。
    for (const auto& entry : config.features) {
        if (random.nextFloat() < entry.chance) {
            return dispatchChildFeature(region, chunk, generator, random, pos, entry.featureId);
        }
    }

    // 全部未命中，走 default
    return dispatchChildFeature(region, chunk, generator, random, pos, config.defaultFeatureId);
}

// ============================================================================
// ConfiguredRandomSelectorFeature
// ============================================================================

ConfiguredRandomSelectorFeature::ConfiguredRandomSelectorFeature(
    std::unique_ptr<RandomSelectorFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredRandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (!m_config) {
        return false;
    }
    return RandomSelectorFeature::place(region, chunk, generator, random, pos, *m_config);
}

} // namespace mc::world::gen::feature
