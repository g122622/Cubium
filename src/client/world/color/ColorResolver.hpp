#pragma once

#include "common/core/Types.hpp"
#include "common/world/biome/Biome.hpp"
#include <algorithm>
#include <array>
#include <cmath>

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
    [[nodiscard]] virtual u32 getColor(const Biome& biome, f64 x, f64 z) const = 0;

    /**
     * @brief 获取指定位置的颜色（带 colormap 支持）
     *
     * 当生物群系没有覆盖颜色时，使用 colormap 计算颜色。
     *
     * @param biome 生物群系引用
     * @param x X坐标（方块坐标，用于噪声计算）
     * @param z Z坐标（方块坐标，用于噪声计算）
     * @param colorMap colormap 数据 (256x256)，可以为 nullptr
     * @param defaultColor 默认颜色（当 colormap 不可用时使用）
     * @return RGB颜色值 (0xRRGGBB)
     */
    [[nodiscard]] virtual u32 getColorWithColorMap(
        const Biome& biome, f64 x, f64 z, const std::array<u32, 65536>* colorMap, u32 defaultColor) const
    {
        // 默认实现：调用 getColor，如果不返回标记值则使用返回的颜色
        const u32 color = getColor(biome, x, z);
        if (color != 0xFFFFFFFF) {
            return color;
        }

        // 如果 colormap 可用，从 colormap 计算
        if (colorMap) {
            const f32 temperature = std::clamp(biome.temperature(), 0.0f, 1.0f);
            const f32 humidity = std::clamp(biome.humidity(), 0.0f, 1.0f) * temperature;
            const i32 tempIndex = static_cast<i32>((1.0f - temperature) * 255.0f);
            const i32 humidityIndex = static_cast<i32>((1.0f - humidity) * 255.0f);
            const i32 colorIndex = (humidityIndex << 8) | tempIndex;
            return (*colorMap)[static_cast<size_t>(colorIndex)];
        }

        return defaultColor;
    }
};

} // namespace client
} // namespace mc
