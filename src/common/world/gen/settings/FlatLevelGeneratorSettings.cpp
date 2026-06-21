/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software by
 * furnished to do so, subject to the following conditions:
 *
 * THE ABOVE copyright notice and this permission notice shall be included in all
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

#include "FlatLevelGeneratorSettings.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {

void FlatLevelGeneratorSettings::updateLayers()
{
    m_layers.clear();

    // 预计算总高度以避免重复分配
    i32 totalHeight = 0;
    for (const auto& layerInfo : m_layersInfo) {
        totalHeight += layerInfo.height();
    }
    m_layers.reserve(static_cast<size_t>(totalHeight));

    for (const auto& layerInfo : m_layersInfo) {
        const BlockState* state = layerInfo.blockState();
        const i32 height = layerInfo.height();

        for (i32 i = 0; i < height; ++i) {
            // 不阻挡运动的方块（如水）替换为 null，由特性系统放置
            // 判断标准: isSolid() || isLiquid() → motion-blocking → 保留
            // 否则（非固体、非液体的非空气方块如草径）→ 设为 null
            if (state != nullptr && !state->isAir() && !state->owner().isSolid(*state) && !state->isLiquid()) {
                // 非运动阻挡方块：由特性系统放置
                m_layers.push_back(nullptr);
            } else {
                m_layers.push_back(state);
            }
        }
    }

    // voidGen 判断：仅当所有展开后的方块都是空气时才为 void
    m_voidGen =
        std::all_of(m_layers.begin(), m_layers.end(), [](const BlockState* s) { return s == nullptr || s->isAir(); });
}

FlatLevelGeneratorSettings FlatLevelGeneratorSettings::createDefault()
{
    FlatLevelGeneratorSettings settings(Biomes::Plains);

    // 默认平坦世界配置：1x Bedrock + 2x Dirt + 1x Grass Block
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));

    settings.updateLayers();
    return settings;
}

} // namespace mc
