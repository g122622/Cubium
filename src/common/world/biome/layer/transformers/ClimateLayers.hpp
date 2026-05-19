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

#include "../BiomeValues.hpp"
#include "TransformerTraits.hpp"

namespace mc {
namespace layer {

/**
 * @brief 添加岛屿层
 *
 * 在海洋中扩展陆地。
 * 参考 MC AddIslandLayer (IBishopTransformer)
 *
 * 采样模式：四对角 + 中心
 */
class AddIslandLayer : public IBishopTransformer {
public:
    using IBishopTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) override;
};

/**
 * @brief 添加雪地层
 *
 * 为陆地分配温度区域。
 * 参考 MC AddSnowLayer (IC1Transformer)
 *
 * 输出值：
 * - 0: 海洋（保持不变）
 * - 1: 温暖（沙漠、热带草原等）
 * - 2: 中等（平原、森林等）
 * - 3: 凉爽（针叶林等）
 * - 4: 冰冻（雪地）
 */
class AddSnowLayer : public IC1Transformer {
public:
    using IC1Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 移除过多海洋层
 *
 * 如果周围都是浅海，有一定概率变成陆地。
 * 参考 MC RemoveTooMuchOceanLayer (ICastleTransformer)
 */
class RemoveTooMuchOceanLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
};

/**
 * @brief 深海层
 *
 * 将被浅海包围的海洋变成深海。
 * 参考 MC DeepOceanLayer (ICastleTransformer)
 */
class DeepOceanLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }
};

} // namespace layer
} // namespace mc
