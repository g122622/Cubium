#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::ecs {

/**
 * @brief 风弹状态组件
 *
 * 承载 WindChargeEntity 的 3 字段：是否已爆裂 / 爆裂中心 / 是否已设爆裂中心。
 * 对齐 vanilla WindCharge 命中时的风爆效果数据。
 *
 * 仅 WindChargeEntity attach。风弹命中时触发风爆（推开实体/破坏可破坏方块），
 * 爆裂中心用于风爆作用范围计算。
 *
 * 字段语义：
 * - m_hasBurst：是否已触发爆裂（防重复触发）。
 * - m_burstCenter：爆裂中心坐标（命中点）。
 * - m_hasBurstCenter：是否已设置爆裂中心（区分「未命中」与「中心(0,0,0)」）。
 */
struct WindChargeStateComponent {
    bool m_hasBurst{false};
    Vector3 m_burstCenter{0.0f, 0.0f, 0.0f};
    bool m_hasBurstCenter{false};
};

} // namespace mc::ecs
