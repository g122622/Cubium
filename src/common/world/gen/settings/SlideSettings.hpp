#pragma once

#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 滑动设置（用于地形边界平滑）
 *
 * 参考 MC NoiseSettings.Slide
 * 用于在顶部和底部创建平滑的地形边界
 *
 * @note 这使得地形在接近世界顶部和底部时逐渐变得平坦
 */
struct SlideSettings {
    i32 target = 0; ///< 目标值
    i32 size = 0;   ///< 大小（影响范围）
    i32 offset = 0; ///< 偏移

    SlideSettings() = default;
    SlideSettings(i32 target_, i32 size_, i32 offset_)
        : target(target_)
        , size(size_)
        , offset(offset_)
    {}
};

} // namespace mc
