#pragma once

#include "TransformerTraits.hpp"
#include "../BiomeValues.hpp"

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
