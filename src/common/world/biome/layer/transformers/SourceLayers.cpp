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
    // 参考 MC IslandLayer.apply:
    // if (p_215735_2_ == 0 && p_215735_3_ == 0) {
    //     return 1;
    // } else {
    //     return p_215735_1_.random(10) == 0 ? 1 : 0;
    // }

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
    // 参考 MC OceanLayer.apply:
    // ImprovedNoiseGenerator improvednoisegenerator = p_215735_1_.getNoiseGenerator();
    // double d0 = improvednoisegenerator.func_215456_a((double)p_215735_2_ / 8.0D, (double)p_215735_3_ / 8.0D, 0.0D,
    // 0.0D, 0.0D); if (d0 > 0.4D) {
    //     return 44;  // warm_ocean
    // } else if (d0 > 0.2D) {
    //     return 45;  // lukewarm_ocean
    // } else if (d0 < -0.4D) {
    //     return 10;  // frozen_ocean
    // } else {
    //     return d0 < -0.2D ? 46 : 0;  // cold_ocean or ocean
    // }

    ImprovedNoiseGenerator* noise = ctx.getNoiseGenerator();
    if (!noise) {
        // 如果没有噪声生成器，返回普通海洋
        return BiomeValues::Ocean;
    }

    // 使用噪声值决定海洋温度
    // 缩放坐标到 1/8
    f32 value = noise->noise(static_cast<f32>(x) / 8.0f, 0.0f, static_cast<f32>(z) / 8.0f);

    if (value > 0.4f) {
        return BiomeValues::WarmOcean; // 暖海洋
    } else if (value > 0.2f) {
        return BiomeValues::LukewarmOcean; // 微温海洋
    } else if (value < -0.4f) {
        return BiomeValues::FrozenOcean; // 冻结海洋
    } else if (value < -0.2f) {
        return BiomeValues::ColdOcean; // 冷海洋
    } else {
        return BiomeValues::Ocean; // 普通海洋
    }
}

std::unique_ptr<IAreaFactory> OceanLayer::apply(IExtendedAreaContext& context)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<SourceFactory>(this, sharedContext);
}

} // namespace layer
} // namespace mc
