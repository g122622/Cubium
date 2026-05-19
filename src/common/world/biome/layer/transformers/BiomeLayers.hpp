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
 * @brief 生物群系分配层
 *
 * 将温度值转换为实际的生物群系 ID。
 * 参考 MC 1.16.5 BiomeLayer (IC0Transformer)
 *
 * 温度值：
 * - 0: 海洋（保持不变）
 * - 1: 温暖区域（沙漠、热带草原、平原）
 * - 2: 中等温度（丛林等）
 * - 3: 凉爽区域（森林、针叶林、山地等）
 * - 4: 冰冻区域（雪地）
 */
class BiomeLayer : public IC0Transformer {
public:
    /**
     * @brief 生物群系配置
     */
    struct Config {
        bool legacyDesertInit = false; // 是否使用旧的沙漠初始化
    };

    explicit BiomeLayer(const Config& config);

    using IC0Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;

private:
    Config m_config;

    // MC 1.16.5 温暖生物群系列表: {Desert, Desert, Desert, Savanna, Savanna, Plains}
    // field_202744_r = new int[]{2, 2, 2, 35, 35, 1};
    static constexpr i32 WARM_BIOMES[] = {BiomeValues::Desert,
        BiomeValues::Desert,
        BiomeValues::Desert,
        BiomeValues::Savanna,
        BiomeValues::Savanna,
        BiomeValues::Plains};

    // MC 1.16.5 凉爽生物群系列表: {Forest, GiantTreeTaigaHills, Mountains, Plains, BirchForest, Swamp}
    // field_202745_s = new int[]{4, 29, 3, 1, 27, 6};
    static constexpr i32 COOL_BIOMES[] = {BiomeValues::Forest,
        BiomeValues::GiantTreeTaigaHills,
        BiomeValues::Mountains,
        BiomeValues::Plains,
        BiomeValues::BirchForest,
        BiomeValues::Swamp};

    // MC 1.16.5 冰冻生物群系列表: {SnowyPlains, SnowyPlains, SnowyPlains, WoodedMountains}
    // field_202747_u = new int[]{12, 12, 12, 30};
    static constexpr i32 ICY_BIOMES[] = {
        BiomeValues::SnowyPlains, BiomeValues::SnowyPlains, BiomeValues::SnowyPlains, BiomeValues::WoodedMountains};
};

/**
 * @brief 稀有生物群系层
 *
 * 在基础生物群系中生成稀有变体。
 * 参考 MC RareBiomeLayer (IC1Transformer)
 */
class RareBiomeLayer : public IC1Transformer {
public:
    using IC1Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 海岸层
 *
 * 在陆地与海洋交界处生成海岸生物群系。
 * 参考 MC ShoreLayer (ICastleTransformer)
 */
class ShoreLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
    [[nodiscard]] bool usesRandom() const override { return false; }
};

/**
 * @brief 平滑层
 *
 * 平滑生物群系边界。
 * 参考 MC SmoothLayer (ICastleTransformer)
 */
class SmoothLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
};

} // namespace layer
} // namespace mc
