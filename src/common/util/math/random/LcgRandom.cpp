#include "LcgRandom.hpp"

namespace mc::math {

LcgRandom::LcgRandom(u64 seed)
    : m_state(seed)
{
    // 确保初始状态不为零（零会导致所有输出都相同）
    if (m_state == 0) {
        m_state = 1;
    }
}

void LcgRandom::setSeed(u64 seed)
{
    m_state = seed;
    if (m_state == 0) {
        m_state = 1;
    }
    m_hasGaussian = false;
}

u64 LcgRandom::nextU64()
{
    // 线性同余生成器
    // X_{n+1} = (a * X_n + c) mod 2^64
    m_state = A * m_state + C;
    return m_state;
}

} // namespace mc::math
