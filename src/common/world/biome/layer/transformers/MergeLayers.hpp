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
#include "../LayerContext.hpp"
#include "TransformerTraits.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace layer {

/**
 * @brief 蘑菇岛层
 *
 * 在被浅海包围的位置生成蘑菇岛。
 * 参考 MC AddMushroomIslandLayer (IBishopTransformer)
 */
class AddMushroomIslandLayer : public IBishopTransformer {
public:
    using IBishopTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) override;
};

/**
 * @brief 竹林层
 *
 * 在丛林中生成竹林。
 * 参考 MC AddBambooForestLayer (IC1Transformer)
 */
class AddBambooForestLayer : public IC1Transformer {
public:
    using IC1Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 河流起始层
 *
 * 为非海洋位置生成河流噪声值。
 * 参考 MC StartRiverLayer (IC0Transformer)
 */
class StartRiverLayer : public IC0Transformer {
public:
    using IC0Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 河流层
 *
 * 从河流噪声值生成河流通道。
 * 参考 MC RiverLayer (ICastleTransformer)
 */
class RiverLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }

private:
    /**
     * @brief 河流过滤函数
     */
    [[nodiscard]] static i32 riverFilter(i32 value);
};

/**
 * @brief 山丘层
 *
 * 合并生物群系层和河流噪声层，生成山丘变体。
 * 参考 MC HillsLayer (ITransformer2)
 */
class HillsLayer : public ITransformer2 {
public:
    using ITransformer2::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) override;

    // 偏移 +1 (类似 IDimOffset1Transformer)
    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x + 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z + 1; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(IExtendedAreaContext& context,
        std::unique_ptr<IAreaFactory> input1,
        std::unique_ptr<IAreaFactory> input2) override;
};

/**
 * @brief 河流混合层
 *
 * 将河流与生物群系层合并。
 * 参考 MC MixRiverLayer (ITransformer2)
 */
class MixRiverLayer : public ITransformer2 {
public:
    using ITransformer2::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) override;
    [[nodiscard]] bool usesRandom() const override { return false; }

    // 无偏移 (类似 IDimOffset0Transformer)
    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(IExtendedAreaContext& context,
        std::unique_ptr<IAreaFactory> input1,
        std::unique_ptr<IAreaFactory> input2) override;
};

/**
 * @brief 海洋混合层
 *
 * 将海洋温度与生物群系层合并。
 * 参考 MC MixOceansLayer (ITransformer2)
 */
class MixOceansLayer : public ITransformer2 {
public:
    using ITransformer2::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& oceanArea, i32 x, i32 z) override;
    [[nodiscard]] bool usesRandom() const override { return false; }

    // 无偏移 (类似 IDimOffset0Transformer)
    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(IExtendedAreaContext& context,
        std::unique_ptr<IAreaFactory> input1,
        std::unique_ptr<IAreaFactory> input2) override;
};

} // namespace layer
} // namespace mc
