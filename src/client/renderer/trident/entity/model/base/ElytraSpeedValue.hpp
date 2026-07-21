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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::model {

/**
 * @brief 鞘翅飞行速度因子的常量与纯逻辑计算
 *
 * 对应 MC 1.21.11 `HumanoidMobRenderer.extractHumanoidRenderState` 中
 * `speedValue` 的填充逻辑：
 *
 * ```java
 * speedValue = 1.0F;
 * if (isFallFlying) {
 *     speedValue = (float)deltaMovement.lengthSqr();
 *     speedValue /= 0.2F;
 *     speedValue = speedValue * (speedValue * speedValue);  // 立方
 * }
 * if (speedValue < 1.0F) speedValue = 1.0F;
 * ```
 *
 * 抽取为自由函数便于在 GPU 管线路径（`EntityRendererManager::_applyBipedElytraState`）
 * 共用，并在单元测试中直接验证公式分支，无需依赖 Vulkan/`EntityRendererManager` 链接。
 */
namespace elytra {

/// MC 1.21.11 中的速度因子分母常量 `0.2F`。
constexpr f32 SPEED_DIVISOR = 0.2f;

/**
 * @brief 计算 BipedModel 的鞘翅飞行速度因子（speedValue）
 *
 * @param isFallFlying 实体是否处于鞘翅飞行状态
 * @param velocityLengthSquared 实体速度向量的长度平方
 *                               （`deltaMovement.lengthSqr()` 的等价物）
 *
 * @return 速度因子，永远 >= 1.0：
 *         - 非飞行时返回 1.0
 *         - 飞行时返回 `(velocityLengthSquared / 0.2)^3`，钳制到 [1.0, +∞)
 */
[[nodiscard]] f32 computeSpeedValue(bool isFallFlying, f32 velocityLengthSquared) noexcept;

} // namespace elytra

} // namespace mc::client::renderer::entity::model
