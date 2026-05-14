#pragma once

#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 噪声生成器接口
 *
 * 所有噪声生成器的基类接口。
 */
class INoiseGenerator {
public:
    virtual ~INoiseGenerator() = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] virtual f32 noise(f32 x, f32 y, f32 z) const = 0;

    /**
     * @brief 采样 2D 噪声值
     */
    [[nodiscard]] virtual f32 noise2D(f32 x, f32 z) const { return noise(x, 0.0f, z); }
};

} // namespace mc
