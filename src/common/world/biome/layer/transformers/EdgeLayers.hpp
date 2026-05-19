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
 * @brief 冷暖边缘层
 *
 * 防止温暖区域直接接触冰冻区域。
 * 参考 MC EdgeLayer.CoolWarm (ICastleTransformer)
 */
class CoolWarmEdgeLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }
};

/**
 * @brief 热冰边缘层
 *
 * 防止炎热区域直接接触冰冻区域。
 * 参考 MC EdgeLayer.HeatIce (ICastleTransformer)
 */
class HeatIceEdgeLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }
};

/**
 * @brief 特殊变体层
 *
 * 为非海洋区域添加特殊变体位。
 * 参考 MC EdgeLayer.Special (IC0Transformer)
 */
class SpecialEdgeLayer : public IC0Transformer {
public:
    using IC0Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 生物群系边缘层
 *
 * 处理生物群系之间的过渡边缘。
 * 参考 MC EdgeBiomeLayer (ICastleTransformer)
 */
class BiomeEdgeLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }
};

} // namespace layer
} // namespace mc
