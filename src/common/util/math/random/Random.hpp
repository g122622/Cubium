#pragma once

/**
 * @file Random.hpp
 * @brief 统一的随机数生成器封装
 *
 * 通过编译宏选择底层随机数算法：
 * - MC_RANDOM_XOROSHIRO128PP: xoroshiro128++ (小状态，高性能)
 * - MC_RANDOM_XOSHIRO256PP: xoshiro256++ (高质量)
 * - MC_RANDOM_LCG: 线性同余 (最小状态)
 * - 默认: Mersenne Twister (最高兼容性)
 *
 * 使用方法：
 * @code
 * #include "math/random/Random.hpp"
 *
 * mc::math::Random rng(seed);
 * i32 value = rng.nextInt(100);  // [0, 100)
 * f32 f = rng.nextFloat();        // [0.0, 1.0)
 * bool b = rng.nextBoolean();     // true/false
 * @endcode
 */

#define MC_RANDOM_XOROSHIRO128PP

// 通过宏选择底层算法
#if defined(MC_RANDOM_XOROSHIRO128PP)
#include "Xoroshiro128ppRandom.hpp"
namespace mc::math {
using Random = Xoroshiro128ppRandom;
}
#elif defined(MC_RANDOM_XOSHIRO256PP)
#include "Xoshiro256ppRandom.hpp"
namespace mc::math {
using Random = Xoshiro256ppRandom;
}
#elif defined(MC_RANDOM_LCG)
#include "LcgRandom.hpp"
namespace mc::math {
using Random = LcgRandom;
}
#else
// 默认使用 Mersenne Twister
#include "Mt19937Random.hpp"
namespace mc::math {
using Random = Mt19937Random;
}
#endif
