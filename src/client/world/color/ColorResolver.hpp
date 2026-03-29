#pragma once

#include "../../../common/core/Types.hpp"

namespace mc {
class Biome;  // 前向声明 - Biome 在 mc 命名空间中
}

namespace mc {
namespace client {

/**
 * @brief 颜色解析器接口
 *
 * 函数式接口，根据生物群系和位置返回颜色值。
 * 位置参数用于支持基于噪声的颜色计算（如沼泽的双色混合）。
 *
 * 参考 MC 1.16.5 ColorResolver 接口
 *
 * @note 所有颜色使用 RGB 格式 (0xRRGGBB)，alpha 通道由调用方处理
 */
class ColorResolver {
public:
    virtual ~ColorResolver() = default;

    /**
     * @brief 获取指定位置的颜色
     *
     * @param biome 生物群系引用
     * @param x X坐标（方块坐标，用于噪声计算）
     * @param z Z坐标（方块坐标，用于噪声计算）
     * @return RGB颜色值 (0xRRGGBB)
     */
    [[nodiscard]] virtual u32 getColor(
        const Biome& biome,
        f64 x,
        f64 z
    ) const = 0;
};

} // namespace client
} // namespace mc
