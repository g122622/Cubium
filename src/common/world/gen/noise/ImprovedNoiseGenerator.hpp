#pragma once

#include "INoiseGenerator.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <array>

namespace mc {

/**
 * @brief 改进的 Perlin 噪声生成器
 *
 * 参考 MC ImprovedNoiseGenerator，实现标准的 3D Perlin 噪声。
 *
 * 使用方法：
 * @code
 * ImprovedNoiseGenerator noise(seed);
 * f64 value = noise.noise(x, y, z);
 * @endcode
 *
 * @note 噪声值范围约为 [-1, 1]
 * @note 参考 MC 1.16.5 的实现
 */
class ImprovedNoiseGenerator : public INoiseGenerator {
public:
    /**
     * @brief 使用种子构造噪声生成器
     * @param seed 随机种子
     */
    explicit ImprovedNoiseGenerator(u64 seed);

    /**
     * @brief 使用随机生成器构造
     * @param rng 随机数生成器接口
     */
    explicit ImprovedNoiseGenerator(math::IRandom& rng);

    ~ImprovedNoiseGenerator() override = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const override;

    /**
     * @brief 采样 3D 噪声值（带 Y 轴缩放）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param yScale Y 轴缩放因子
     * @param yBound Y 轴边界
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z, f32 yScale, f32 yBound) const;

    /**
     * @brief 原始采样（使用整数坐标和偏移）
     *
     * 参考 MC 的 func_215459_a 方法
     */
    [[nodiscard]] f32 noiseRaw(i32 x, i32 y, i32 z,
                                f32 deltaX, f32 deltaY, f32 deltaZ,
                                f32 fadeX, f32 fadeY, f32 fadeZ) const;

    // 坐标偏移（参考 MC 的公开字段）
    [[nodiscard]] f32 xOffset() const { return m_xOffset; }
    [[nodiscard]] f32 yOffset() const { return m_yOffset; }
    [[nodiscard]] f32 zOffset() const { return m_zOffset; }

private:
    // 排列表（256 字节，复制一份用于快速查找）
    std::array<u8, 256> m_permutation;
    // 工作数组（用于避免每次调用时的临时分配）
    mutable std::array<u8, 512> m_p;

    // 坐标偏移
    f32 m_xOffset = 0.0f;
    f32 m_yOffset = 0.0f;
    f32 m_zOffset = 0.0f;

    /**
     * @brief 初始化排列数组
     */
    void initPermutation(math::IRandom& rng);

    /**
     * @brief 获取排列值
     */
    [[nodiscard]] u8 getPermut(i32 index) const {
        return m_p[index & 255];
    }

    /**
     * @brief 梯度计算
     */
    [[nodiscard]] static f32 grad(i32 hash, f32 x, f32 y, f32 z);

    /**
     * @brief 平滑插值（Perlin 的 fade 函数）
     */
    [[nodiscard]] static f32 fade(f32 t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    /**
     * @brief 线性插值
     */
    [[nodiscard]] static f32 lerp(f32 a, f32 b, f32 t) {
        return a + t * (b - a);
    }
};

// ============================================================================
// 梯度向量表 (参考 SimplexNoiseGenerator.GRADS)
// ============================================================================

/**
 * @brief Perlin 噪声梯度向量
 *
 * 格式: {x, y, z}
 */
constexpr f32 PERLIN_GRADIENTS[16][3] = {
    { 1.0f,  1.0f,  0.0f}, {-1.0f,  1.0f,  0.0f}, { 1.0f, -1.0f,  0.0f}, {-1.0f, -1.0f,  0.0f},
    { 1.0f,  0.0f,  1.0f}, {-1.0f,  0.0f,  1.0f}, { 1.0f,  0.0f, -1.0f}, {-1.0f,  0.0f, -1.0f},
    { 0.0f,  1.0f,  1.0f}, { 0.0f, -1.0f,  1.0f}, { 0.0f,  1.0f, -1.0f}, { 0.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f,  0.0f}, {-1.0f,  1.0f,  0.0f}, { 0.0f, -1.0f,  1.0f}, { 0.0f,  1.0f, -1.0f}
};

} // namespace mc
