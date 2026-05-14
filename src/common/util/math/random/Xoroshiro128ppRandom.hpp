#pragma once

#include "IRandom.hpp"

namespace mc::math {

/**
 * @brief xoroshiro128++ 随机数生成器
 *
 * 高性能、小状态的随机数生成器。
 * 具有以下特点：
 * - 周期：2^128 - 1
 * - 状态小：仅 16 字节
 * - 速度快：比 Mersenne Twister 快约 2-3 倍
 * - 质量高：通过了 BigCrush 测试
 *
 * 使用方法：
 * @code
 * Xoroshiro128ppRandom rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 适合需要高性能和小状态的场景
 * @note 参考 http://xoroshiro.di.unimi.it/
 */
class Xoroshiro128ppRandom : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     *
     * @note 种子通过 SplitMix64 算法扩展为 128 位状态
     */
    explicit Xoroshiro128ppRandom(u64 seed = 0);

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

    /**
     * @brief 跳过指定数量的随机数
     * @param count 要跳过的随机数数量
     *
     * xoroshiro128++ 支持快速跳转，时间复杂度 O(log count)
     */
    void skip(u64 count) override;

private:
    u64 m_state[2];

    /**
     * @brief 左旋转
     */
    [[nodiscard]] static u64 rotl(u64 x, int k) { return (x << k) | (x >> (64 - k)); }

    /**
     * @brief 使用 SplitMix64 扩展种子
     */
    static u64 splitMix64(u64& state);
};

} // namespace mc::math
