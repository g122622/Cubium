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

#include "BlockColumnFeature.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature::cave {

// ============================================================================
// BlockColumnFeature
// ============================================================================

bool BlockColumnFeature::place(
    WorldGenRegion& region, math::Random& random, const BlockPos& pos, const BlockColumnConfig& config)
{
    if (config.layers.empty()) {
        return false;
    }

    // 计算每层高度
    i32 totalHeight = 0;
    std::vector<i32> layerHeights;
    layerHeights.reserve(config.layers.size());

    for (const auto& layer : config.layers) {
        i32 height = layer.height ? layer.height->sample(random) : 1;
        layerHeights.push_back(height);
        totalHeight += height;
    }

    if (totalHeight == 0) {
        return false;
    }

    // 检查放置位置
    Direction dir = config.direction;
    BlockPos current = pos;
    i32 failPosition = -1; // -1 表示没有失败

    // 先验证路径是否可放置
    if (config.allowedPlacement) {
        BlockPos checkPos = pos;
        for (i32 i = 0; i < totalHeight; ++i) {
            if (!config.allowedPlacement->test(region, checkPos)) {
                failPosition = i;
                break;
            }
            checkPos = checkPos.offset(dir);
        }
    }

    // 如果第一个位置就不允许放置，直接返回
    if (failPosition == 0) {
        return false;
    }

    // 截断超出可放置范围的层
    if (failPosition > 0) {
        i32 toRemove = totalHeight - failPosition;
        if (config.prioritizeTip) {
            // 从底部删除
            for (i32 i = 0; i < static_cast<i32>(layerHeights.size()) && toRemove > 0; ++i) {
                i32 removeFromLayer = std::min(layerHeights[i], toRemove);
                layerHeights[i] -= removeFromLayer;
                toRemove -= removeFromLayer;
            }
        } else {
            // 从顶部删除
            for (i32 i = static_cast<i32>(layerHeights.size()) - 1; i >= 0 && toRemove > 0; --i) {
                i32 removeFromLayer = std::min(layerHeights[i], toRemove);
                layerHeights[i] -= removeFromLayer;
                toRemove -= removeFromLayer;
            }
        }
    }

    // 放置方块
    current = pos;
    bool placedAny = false;

    for (size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
        // 每层采样一次方块状态（坐标取该层起始格）
        const BlockState* layerState =
            config.layers[layerIdx].getState(region, random, current.x, current.y, current.z);
        i32 height = layerHeights[layerIdx];

        for (i32 h = 0; h < height; ++h) {
            if (layerState != nullptr) {
                const BlockState* existing = region.getBlockState(current);
                if (existing == nullptr || existing->canBeReplaced()) {
                    region.setBlockState(current, layerState, 3);
                    placedAny = true;
                }
            }
            current = current.offset(dir);
        }
    }

    return placedAny;
}

// ============================================================================
// ConfiguredBlockColumnFeature
// ============================================================================

ConfiguredBlockColumnFeature::ConfiguredBlockColumnFeature(
    std::unique_ptr<BlockColumnConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBlockColumnFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);

    if (!m_config) {
        return false;
    }

    return BlockColumnFeature::place(region, random, pos, *m_config);
}

} // namespace mc::world::gen::feature::cave
