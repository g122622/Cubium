#pragma once

#include "TransformerTraits.hpp"
#include "../BiomeValues.hpp"

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
