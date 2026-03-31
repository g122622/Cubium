#pragma once

#include "../LayerContext.hpp"
#include "../BiomeValues.hpp"

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
