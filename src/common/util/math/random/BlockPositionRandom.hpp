#pragma once

#include "common/core/Types.hpp"

namespace mc::math {

/** @brief 生成随机刻、降水和闪电位置使用的打包坐标位，保留 Java int 回绕语义。 */
class BlockPositionRandom {
public:
    /** @brief 显式指定初始 32 位状态。 */
    explicit BlockPositionRandom(u32 seed)
        : m_state(seed)
    {}

    /** @brief 推进 LCG 并算术右移两位；调用方按位置掩码提取坐标。 */
    [[nodiscard]] i32 nextPositionBits()
    {
        // 无符号乘加在 32 位内回绕，再按 Java int 的符号进行右移。
        m_state = m_state * 3U + 1013904223U;
        return static_cast<i32>(m_state) >> 2;
    }

private:
    u32 m_state;
};

} // namespace mc::math
