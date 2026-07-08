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

#include "client/application/features/VibrationTargetResolver.hpp"

#include "client/world/entity/ClientEntity.hpp"

namespace mc::client::application::features {

[[nodiscard]] std::optional<Vector3d> resolveVibrationTargetPosition(VibrationTargetKind targetKind,
    f64 targetX,
    f64 targetY,
    f64 targetZ,
    EntityId targetEntityId,
    f32 yOffset,
    const std::function<const ClientEntity*(EntityId)>& entityLookup)
{
    // 实体来源：通过实体查找回调解析实体当前位置，叠加 Y 轴偏移
    // 对应 MC Java EntityPositionSource.getPosition(Level)
    if (targetKind == 1) {
        if (targetEntityId == INVALID_ENTITY_ID || !entityLookup) {
            return std::nullopt;
        }

        const ClientEntity* targetEntity = entityLookup(targetEntityId);
        if (targetEntity == nullptr) {
            // 实体不在客户端视野内，放弃生成粒子（对应 MC Java VibrationSignalParticle.tick 中
            // target.getPosition().isEmpty() 时 remove）
            return std::nullopt;
        }

        return Vector3d(static_cast<f64>(targetEntity->x()),
            static_cast<f64>(targetEntity->y()) + static_cast<f64>(yOffset),
            static_cast<f64>(targetEntity->z()));
    }

    // 方块来源及其它未知类型：直接使用解码后的坐标（方块中心已在 NetworkClient 中计算）
    return Vector3d(targetX, targetY, targetZ);
}

} // namespace mc::client::application::features
