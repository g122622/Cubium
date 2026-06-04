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

#include "common/world/biome/layer/BiomeValues.hpp"
#include "common/world/biome/layer/LayerContext.hpp"
#include <memory>

namespace mc {
namespace layer {

/**
 * @brief 岛屿层变换器
 *
 * 初始岛屿生成层。生成初始的陆地/海洋分布。
 * 参考 MC IslandLayer
 *
 * 规则：
 * - 原点 (0, 0) 固定为陆地（玩家出生点）
 * - 其他位置 10% 概率为陆地
 *
 * 输出值：
 * - 0: 海洋
 * - 1: 陆地
 */
class IslandLayer : public ITransformer0 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 x, i32 z) override;

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(IExtendedAreaContext& context) override;
};

/**
 * @brief 海洋温度层变换器
 *
 * 使用 Perlin 噪声生成海洋温度分布。
 * 参考 MC OceanLayer
 *
 * 输出值：
 * - 44: 暖海洋 (warm_ocean)
 * - 45: 微温海洋 (lukewarm_ocean)
 * - 0: 普通海洋 (ocean)
 * - 46: 冷海洋 (cold_ocean)
 * - 10: 冻结海洋 (frozen_ocean)
 */
class OceanLayer : public ITransformer0 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 x, i32 z) override;
    [[nodiscard]] bool usesRandom() const override { return false; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(IExtendedAreaContext& context) override;
};

} // namespace layer
} // namespace mc
