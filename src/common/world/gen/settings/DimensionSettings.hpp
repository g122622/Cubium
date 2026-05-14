#pragma once

#include "../../block/Block.hpp"
#include "NoiseSettings.hpp"

// 前向声明
namespace mc {
class BlockState;
}

namespace mc {

/**
 * @brief 维度生成设置
 *
 * 参考 MC DimensionSettings，包含维度级别的生成配置。
 * 使用 BlockState* 替代固定 BlockId，支持动态方块注册。
 *
 * @note 参考 MC 1.16.5 DimensionSettings
 */
struct DimensionSettings {
    NoiseSettings noise;
    const BlockState* defaultBlock = nullptr; ///< 默认方块（石头等）
    const BlockState* defaultFluid = nullptr; ///< 默认流体（水/熔岩）
    i32 seaLevel = 63;
    i32 bedrockRoof = -10; ///< 基岩顶部（下界用）
    i32 bedrockFloor = 0;  ///< 基岩底部

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static DimensionSettings overworld();

    /**
     * @brief 下界设置
     */
    static DimensionSettings nether();

    /**
     * @brief 末地设置
     */
    static DimensionSettings end();

    /**
     * @brief 平坦世界设置
     */
    static DimensionSettings flat();
};

} // namespace mc
