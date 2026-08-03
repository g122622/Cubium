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

#include "ClientPlayerPredictor.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace mc::client {

// 常量
static constexpr Size MAX_PENDING_INPUTS = 100;          // 最大待确认输入数量
static constexpr f32 ROTATION_INTERPOLATION_RATE = 0.3f; // 旋转插值速率

ClientPlayerPredictor::ClientPlayerPredictor()
    : m_predictedPosition(0.0f, 0.0f, 0.0f)
    , m_serverPosition(0.0f, 0.0f, 0.0f)
    , m_correctionStart(0.0f, 0.0f, 0.0f)
    , m_correctionTarget(0.0f, 0.0f, 0.0f)
{}

void ClientPlayerPredictor::handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking)
{
    // 保存输入状态
    m_inputForward = forward;
    m_inputStrafe = strafe;
    m_inputJumping = jumping;
    m_inputSneaking = sneaking;

    // 创建待确认输入
    PendingInput input;
    input.sequence = ++m_inputSequence;

    // 计算移动增量（简化版，实际应考虑物理碰撞）
    // 这里只做简单的预测，实际物理由物理引擎处理
    f32 length = std::sqrt(forward * forward + strafe * strafe);
    if (length > 0.0f) {
        // 归一化并应用速度
        f32 normalizedForward = forward / length;
        f32 normalizedStrafe = strafe / length;

        // 根据偏航角计算移动方向
        f32 yawRad = mc::math::toRadians(m_predictedYaw);
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // 计算世界坐标系中的移动向量
        input.delta.x = (normalizedForward * -sinYaw + normalizedStrafe * cosYaw) * m_movementSpeed * 0.05f;
        input.delta.z = (normalizedForward * cosYaw + normalizedStrafe * sinYaw) * m_movementSpeed * 0.05f;
    } else {
        input.delta = Vector3(0.0f, 0.0f, 0.0f);
    }

    input.yaw = m_predictedYaw;
    input.pitch = m_predictedPitch;
    input.jumping = jumping;
    input.sneaking = sneaking;

    // 添加到待确认队列
    m_pendingInputs.push_back(input);

    // 限制队列大小
    while (m_pendingInputs.size() > MAX_PENDING_INPUTS) {
        m_pendingInputs.pop_front();
    }

    // 立即更新预测位置
    _updatePrediction(0.05f); // 假设 50ms 延迟
}

void ClientPlayerPredictor::handleRotationInput(f32 deltaYaw, f32 deltaPitch)
{
    m_predictedYaw += deltaYaw;
    m_predictedPitch += deltaPitch;

    // 限制俯仰角范围
    m_predictedPitch = std::clamp(m_predictedPitch, -90.0f, 90.0f);

    // 归一化偏航角到 [-180, 180]
    while (m_predictedYaw > 180.0f)
        m_predictedYaw -= 360.0f;
    while (m_predictedYaw < -180.0f)
        m_predictedYaw += 360.0f;
}

void ClientPlayerPredictor::receiveServerPosition(const Vector3& position, f32 yaw, f32 pitch)
{
    m_serverPosition = position;
    m_serverYaw = yaw;
    m_serverPitch = pitch;
    m_hasServerPosition = true;

    // 计算预测位置与服务端位置的偏差
    Vector3 diff = m_predictedPosition - position;
    f32 distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

    // 如果偏差超过阈值，启动校正
    if (distance > m_correctionThreshold) {
        // 开始平滑校正
        m_isCorrecting = true;
        m_correctionStart = m_predictedPosition;
        m_correctionTarget = position;
        m_correctionProgress = 0.0f;

        // 直接使用服务端位置作为新的预测位置
        // 这样可以避免累积误差
        m_predictedPosition = position;
    }

    // 旋转插值（更平滑）
    m_predictedYaw = yaw;
    m_predictedPitch = pitch;
}

void ClientPlayerPredictor::acknowledgeInput(u32 lastAckSequence)
{
    _prunePendingInputs(lastAckSequence);
}

void ClientPlayerPredictor::tick(f32 deltaTime)
{
    // 如果正在校正，进行插值
    if (m_isCorrecting) {
        m_correctionProgress += m_correctionRate;

        if (m_correctionProgress >= 1.0f) {
            // 校正完成
            m_isCorrecting = false;
            m_correctionProgress = 0.0f;
            m_predictedPosition = m_correctionTarget;
        } else {
            // 平滑插值
            f32 t = m_correctionProgress;
            m_predictedPosition = m_correctionStart + (m_correctionTarget - m_correctionStart) * t;
        }
    }

    // 应用当前输入预测
    _updatePrediction(deltaTime);
}

Vector3 ClientPlayerPredictor::predictedPosition() const
{
    return m_predictedPosition;
}

std::pair<f32, f32> ClientPlayerPredictor::predictedRotation() const
{
    return {m_predictedYaw, m_predictedPitch};
}

Vector3 ClientPlayerPredictor::serverPosition() const
{
    return m_serverPosition;
}

std::pair<f32, f32> ClientPlayerPredictor::serverRotation() const
{
    return {m_serverYaw, m_serverPitch};
}

bool ClientPlayerPredictor::hasServerPosition() const
{
    return m_hasServerPosition;
}

void ClientPlayerPredictor::reset(const Vector3& position, f32 yaw, f32 pitch)
{
    m_predictedPosition = position;
    m_predictedYaw = yaw;
    m_predictedPitch = pitch;
    m_serverPosition = position;
    m_serverYaw = yaw;
    m_serverPitch = pitch;
    m_hasServerPosition = true;
    m_isCorrecting = false;
    m_correctionProgress = 0.0f;

    // 清除所有待确认输入
    clearPendingInputs();
}

void ClientPlayerPredictor::clearPendingInputs()
{
    m_pendingInputs.clear();
    m_inputSequence = 0;
}

u32 ClientPlayerPredictor::currentSequence() const
{
    return m_inputSequence;
}

void ClientPlayerPredictor::setMovementSpeed(f32 speed)
{
    m_movementSpeed = speed;
}

void ClientPlayerPredictor::setCorrectionThreshold(f32 threshold)
{
    m_correctionThreshold = threshold;
}

void ClientPlayerPredictor::_applyCorrection()
{
    if (!m_hasServerPosition) {
        return;
    }

    // 直接跳转到服务端位置（硬校正）
    m_predictedPosition = m_serverPosition;
    m_predictedYaw = m_serverYaw;
    m_predictedPitch = m_serverPitch;
    m_isCorrecting = false;
}

void ClientPlayerPredictor::_updatePrediction(f32 deltaTime)
{
    // 如果有移动输入，更新预测位置
    f32 length = std::sqrt(m_inputForward * m_inputForward + m_inputStrafe * m_inputStrafe);
    if (length > 0.0f) {
        // 归一化
        f32 normalizedForward = m_inputForward / length;
        f32 normalizedStrafe = m_inputStrafe / length;

        // 根据偏航角计算移动方向
        f32 yawRad = mc::math::toRadians(m_predictedYaw);
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // 计算世界坐标系中的移动向量
        f32 moveX = (normalizedForward * -sinYaw + normalizedStrafe * cosYaw) * m_movementSpeed * deltaTime;
        f32 moveZ = (normalizedForward * cosYaw + normalizedStrafe * sinYaw) * m_movementSpeed * deltaTime;

        // 应用移动
        m_predictedPosition.x += moveX;
        m_predictedPosition.z += moveZ;

        // 注意：跳跃和重力由物理引擎处理，这里不预测
    }

    // 如果正在校正，进行平滑插值
    if (m_isCorrecting) {
        m_correctionProgress += deltaTime * 5.0f; // 0.2秒完成校正

        if (m_correctionProgress >= 1.0f) {
            m_isCorrecting = false;
            m_correctionProgress = 0.0f;
            m_predictedPosition = m_correctionTarget;
        } else {
            // 使用 smoothstep 插值
            f32 t = m_correctionProgress;
            t = t * t * (3.0f - 2.0f * t); // smoothstep
            m_predictedPosition = m_correctionStart + (m_correctionTarget - m_correctionStart) * t;
        }
    }
}

void ClientPlayerPredictor::_prunePendingInputs(u32 lastAckSequence)
{
    // 移除已确认的输入
    while (!m_pendingInputs.empty() && m_pendingInputs.front().sequence <= lastAckSequence) {
        m_pendingInputs.pop_front();
    }
}

} // namespace mc::client
