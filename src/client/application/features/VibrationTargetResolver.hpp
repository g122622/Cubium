/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

#include <functional>
#include <optional>

namespace mc::client {
class ClientEntity;
} // namespace mc::client

namespace mc::client::application::features {

/**
 * @brief 振动粒子目标位置来源类型
 *
 * 与 ParticlePacket::VibrationTarget::Kind 一致：
 *   0 = 方块位置源（targetX/Y/Z 已为方块中心）
 *   1 = 实体位置源（需要通过 entityLookup 解析实体当前位置）
 */
using VibrationTargetKind = u8;

/**
 * @brief 解析振动粒子目标位置
 *
 * 将 ClientApplicationNetwork 的振动粒子回调中"根据目标来源类型解析目标坐标"的逻辑
 * 抽取为纯函数，便于单元测试覆盖，且不依赖 ParticleManager / ClientWorld。
 *
 * 行为：
 * - targetKind == 0（方块来源）：直接返回 (targetX, targetY, targetZ)。
 * - targetKind == 1（实体来源）且 targetEntityId 有效：
 *   * 调用 entityLookup 查找实体，找到则返回
 *     (entity.x, entity.y + yOffset, entity.z)。
 *   * 找不到则返回 std::nullopt（对应 MC Java VibrationSignalParticle.tick 中
 *     target.getPosition().isEmpty() 时 remove 的行为）。
 * - targetKind == 1 但 targetEntityId == INVALID_ENTITY_ID：返回 std::nullopt。
 * - 其它未知 targetKind：直接返回 (targetX, targetY, targetZ)。
 *
 * @param targetKind 目标来源类型（0=Block, 1=Entity）
 * @param targetX 方块来源时的目标 X 坐标
 * @param targetY 方块来源时的目标 Y 坐标
 * @param targetZ 方块来源时的目标 Z 坐标
 * @param targetEntityId 实体来源时的目标实体 ID
 * @param yOffset 实体来源时的 Y 轴偏移（如眼睛高度）
 * @param entityLookup 实体查找回调，返回 nullptr 表示实体不存在
 * @return 解析成功返回目标位置，否则返回 std::nullopt
 */
[[nodiscard]] std::optional<Vector3d> resolveVibrationTargetPosition(VibrationTargetKind targetKind,
    f64 targetX,
    f64 targetY,
    f64 targetZ,
    EntityId targetEntityId,
    f32 yOffset,
    const std::function<const ClientEntity*(EntityId)>& entityLookup);

} // namespace mc::client::application::features
