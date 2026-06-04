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

#include "SourceLayers.hpp"
#include <memory>

namespace mc {
namespace layer {

// ============================================================================
// IslandLayer 实现
// ============================================================================

i32 IslandLayer::apply(IAreaContext& ctx, i32 x, i32 z)
{
    // 原点固定为陆地（玩家出生点）
    if (x == 0 && z == 0) {
        return 1;
    }

    // 其他位置 10% 概率为陆地
    return ctx.nextInt(10) == 0 ? 1 : 0;
}

std::unique_ptr<IAreaFactory> IslandLayer::apply(IExtendedAreaContext& context)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<SourceFactory>(this, sharedContext);
}

// ============================================================================
// OceanLayer 实现
// ============================================================================

i32 OceanLayer::apply(IAreaContext& ctx, i32 x, i32 z)
{
    ImprovedNoiseGenerator* noise = ctx.getNoiseGenerator();
    if (!noise) {
        // 如果没有噪声生成器，返回普通海洋
        return BiomeValues::Ocean;
    }

    // 使用噪声值决定海洋温度
    // 缩放坐标到 1/8，生成大范围的温度带
    constexpr f32 COORD_SCALE = 8.0f;
    constexpr f32 WARM_OCEAN_THRESHOLD = 0.4f;
    constexpr f32 LUKEWARM_OCEAN_THRESHOLD = 0.2f;
    constexpr f32 COLD_OCEAN_THRESHOLD = -0.2f;
    constexpr f32 FROZEN_OCEAN_THRESHOLD = -0.4f;

    f32 value = noise->noise(static_cast<f32>(x) / COORD_SCALE, 0.0f, static_cast<f32>(z) / COORD_SCALE);

    if (value > WARM_OCEAN_THRESHOLD) {
        return BiomeValues::WarmOcean;
    } else if (value > LUKEWARM_OCEAN_THRESHOLD) {
        return BiomeValues::LukewarmOcean;
    } else if (value < FROZEN_OCEAN_THRESHOLD) {
        return BiomeValues::FrozenOcean;
    } else if (value < COLD_OCEAN_THRESHOLD) {
        return BiomeValues::ColdOcean;
    } else {
        return BiomeValues::Ocean;
    }
}

std::unique_ptr<IAreaFactory> OceanLayer::apply(IExtendedAreaContext& context)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<SourceFactory>(this, sharedContext);
}

} // namespace layer
} // namespace mc
