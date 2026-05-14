#pragma once

#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 噪声缩放设置
 *
 * 参考 MC NoiseSettings.Scaling
 * 主世界默认值：xzScale=0.9999999814507745, yScale=0.9999999814507745
 *                xzFactor=80.0, yFactor=160.0
 *
 * @note 用于控制噪声在不同轴上的缩放比例
 */
struct ScalingSettings {
    f32 xzScale = 0.9999999814507745f; ///< XZ 平面缩放
    f32 yScale = 0.9999999814507745f;  ///< Y 轴缩放
    f32 xzFactor = 80.0f;              ///< XZ 因子
    f32 yFactor = 160.0f;              ///< Y 因子
};

} // namespace mc
