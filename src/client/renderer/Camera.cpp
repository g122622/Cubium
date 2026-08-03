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

#include "Camera.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

namespace mc::client {

// ============================================================================
// Camera 实现
// ============================================================================

Camera::Camera(const CameraConfig& config)
    : m_config(config)
{
    _updateVectors();
    _updateProjectionMatrix();
}

void Camera::update(f64 deltaTime)
{
    (void)deltaTime; // 暂时不使用

    if (m_viewDirty) {
        _updateViewMatrix();
    }
    if (m_projectionDirty) {
        _updateProjectionMatrix();
    }
    if (m_viewDirty || m_projectionDirty) {
        _updateViewProjectionMatrix();
        m_dirty = true;
        m_viewDirty = false;
        m_projectionDirty = false;
    }
}

void Camera::setPosition(const glm::dvec3& position)
{
    m_position = position;
    m_viewDirty = true;
}

void Camera::setPosition(f64 x, f64 y, f64 z)
{
    m_position = glm::dvec3(x, y, z);
    m_viewDirty = true;
}

void Camera::setRotation(const glm::dvec3& rotation)
{
    m_rotation = rotation;
    // 限制俯仰角
    m_rotation.x = math::clamp<f64>(m_rotation.x, -m_config.pitchLimit, m_config.pitchLimit);
    _updateVectors();
    m_viewDirty = true;
}

void Camera::setRotation(f64 pitch, f64 yaw, f64 roll)
{
    m_rotation = glm::dvec3(pitch, yaw, roll);
    m_rotation.x = math::clamp<f64>(m_rotation.x, -m_config.pitchLimit, m_config.pitchLimit);
    _updateVectors();
    m_viewDirty = true;
}

void Camera::setPitch(f64 pitch)
{
    m_rotation.x = math::clamp<f64>(pitch, -m_config.pitchLimit, m_config.pitchLimit);
    _updateVectors();
    m_viewDirty = true;
}

void Camera::setYaw(f64 yaw)
{
    m_rotation.y = yaw;
    _updateVectors();
    m_viewDirty = true;
}

void Camera::setRoll(f64 roll)
{
    m_rotation.z = roll;
    _updateVectors();
    m_viewDirty = true;
}

glm::dvec3 Camera::forward() const
{
    return m_forward;
}

glm::dvec3 Camera::right() const
{
    return m_right;
}

glm::dvec3 Camera::up() const
{
    return m_up;
}

void Camera::moveForward(f64 distance)
{
    // 水平移动（忽略Y分量）
    m_position.x += m_forward.x * distance;
    m_position.z += m_forward.z * distance;
    m_viewDirty = true;
}

void Camera::moveRight(f64 distance)
{
    m_position += glm::dvec3(m_right.x * distance, m_right.y * distance, m_right.z * distance);
    m_viewDirty = true;
}

void Camera::moveUp(f64 distance)
{
    m_position.y += distance;
    m_viewDirty = true;
}

void Camera::rotate(f64 pitchDelta, f64 yawDelta)
{
    m_rotation.x = math::clamp<f64>(m_rotation.x + pitchDelta, -m_config.pitchLimit, m_config.pitchLimit);
    m_rotation.y += yawDelta;
    _updateVectors();
    m_viewDirty = true;
}

void Camera::look(f64 mouseDeltaX, f64 mouseDeltaY)
{
    // 应用鼠标灵敏度和方向
    // 鼠标右移 -> yaw 增大 -> 视角右转
    f64 yawDelta = mouseDeltaX * m_config.mouseSensitivity;
    // 鼠标上移 -> pitch 增大 -> 视角上抬
    f64 pitchDelta = -mouseDeltaY * m_config.mouseSensitivity;

    rotate(pitchDelta, yawDelta);
}

void Camera::setViewTransform(const glm::mat4& transform)
{
    m_viewTransform = transform;
    m_viewDirty = true;
}

void Camera::clearViewTransform()
{
    setViewTransform(glm::mat4(1.0f));
}

void Camera::setProjectionMode(ProjectionMode mode)
{
    m_config.projectionMode = mode;
    m_projectionDirty = true;
}

void Camera::setFOV(f64 fov)
{
    m_config.fov = fov;
    m_projectionDirty = true;
}

void Camera::setAspectRatio(f64 aspectRatio)
{
    m_config.aspectRatio = aspectRatio;
    m_projectionDirty = true;
}

void Camera::setNearFar(f64 nearPlane, f64 farPlane)
{
    m_config.nearPlane = nearPlane;
    m_config.farPlane = farPlane;
    m_projectionDirty = true;
}

void Camera::setOrthoSize(f64 size)
{
    m_config.orthoSize = size;
    m_projectionDirty = true;
}

void Camera::setConfig(const CameraConfig& config)
{
    m_config = config;
    m_viewDirty = true;
    m_projectionDirty = true;
}

/**
 * @brief 更新方向向量
 *
 * 使用Minecraft坐标系约定：
 * - yaw=0: 看向 +Z 方向
 * - yaw=90: 看向 -X 方向
 * - yaw=180: 看向 -Z 方向
 * - yaw=270: 看向 +X 方向
 */
void Camera::_updateVectors()
{
    // 从欧拉角计算方向向量
    const f64 pitchRad = m_rotation.x * math::PI_DOUBLE / 180.0;
    const f64 yawRad = m_rotation.y * math::PI_DOUBLE / 180.0;

    // 前向向量 - MC坐标系
    m_forward.x = -std::sin(yawRad) * std::cos(pitchRad);
    m_forward.y = std::sin(pitchRad);
    m_forward.z = std::cos(yawRad) * std::cos(pitchRad);
    m_forward = glm::normalize(m_forward);

    // 右向量和上向量
    // 假设世界上方向为Y轴正方向
    glm::dvec3 worldUp(0.0, 1.0, 0.0);
    m_right = glm::normalize(glm::cross(m_forward, worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::_updateViewMatrix()
{
    // 视图矩阵：将世界坐标转换到相机空间
    m_baseViewMatrix = glm::mat4(glm::lookAt(m_position, m_position + m_forward, m_up));
    m_viewMatrix = m_viewTransform * m_baseViewMatrix;
}

void Camera::_updateProjectionMatrix()
{
    if (m_config.projectionMode == ProjectionMode::Perspective) {
        m_projectionMatrix = glm::perspective(static_cast<f32>(m_config.fov * math::PI_DOUBLE / 180.0),
            static_cast<f32>(m_config.aspectRatio),
            static_cast<f32>(m_config.nearPlane),
            static_cast<f32>(m_config.farPlane));
    } else {
        // 正交投影
        const f64 halfWidth = m_config.orthoSize * m_config.aspectRatio * 0.5;
        const f64 halfHeight = m_config.orthoSize * 0.5;
        m_projectionMatrix = glm::ortho(static_cast<f32>(-halfWidth),
            static_cast<f32>(halfWidth),
            static_cast<f32>(-halfHeight),
            static_cast<f32>(halfHeight),
            static_cast<f32>(m_config.nearPlane),
            static_cast<f32>(m_config.farPlane));
    }

    // Vulkan 使用右手坐标系，Y 轴向下，需要翻转投影矩阵的 Y 轴
    m_projectionMatrix[1][1] *= -1.0f;
}

void Camera::_updateViewProjectionMatrix()
{
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

// ============================================================================
// CameraController 实现
// ============================================================================

CameraController::CameraController(Camera* camera)
    : m_camera(camera)
{}

void CameraController::setCamera(Camera* camera)
{
    m_camera = camera;
}

void CameraController::handleKeyboardInput(i32 key, i32 action)
{
    if (!m_camera) return;

    bool pressed = (action == 1); // GLFW_PRESS

    if (key == m_moveForwardKey) {
        m_moveForward = pressed;
    } else if (key == m_moveBackwardKey) {
        m_moveBackward = pressed;
    } else if (key == m_moveLeftKey) {
        m_moveLeft = pressed;
    } else if (key == m_moveRightKey) {
        m_moveRight = pressed;
    } else if (key == m_moveUpKey) {
        m_moveUp = pressed;
    } else if (key == m_moveDownKey) {
        m_moveDown = pressed;
        m_sprinting = pressed;
    } else if (key == m_sneakKey) {
        m_sneaking = pressed;
    }
}

void CameraController::handleMouseMove(f64 deltaX, f64 deltaY)
{
    if (!m_camera) return;

    m_camera->look(static_cast<f64>(deltaX), static_cast<f64>(deltaY));
}

void CameraController::handleScroll(f64 deltaY)
{
    if (!m_camera) return;

    // 滚轮可以用来调整FOV或缩放
    f64 fov = m_camera->fov();
    fov -= deltaY * 2.0;
    fov = math::clamp<f64>(fov, 10.0, 120.0);
    m_camera->setFOV(fov);
}

void CameraController::update(f64 deltaTime)
{
    if (!m_camera) return;

    // 计算移动速度
    f64 speed = m_camera->moveSpeed();
    if (m_sprinting) {
        speed *= m_camera->config().sprintMultiplier;
    }
    if (m_sneaking) {
        speed *= m_camera->config().sneakMultiplier;
    }

    f64 distance = speed * deltaTime;

    // 移动
    if (m_moveForward) {
        m_camera->moveForward(distance);
    }
    if (m_moveBackward) {
        m_camera->moveForward(-distance);
    }
    if (m_moveRight) {
        m_camera->moveRight(distance);
    }
    if (m_moveLeft) {
        m_camera->moveRight(-distance);
    }
    if (m_moveUp) {
        m_camera->moveUp(distance);
    }
    if (m_moveDown) {
        m_camera->moveUp(-distance);
    }

    // 更新相机
    m_camera->update(deltaTime);

    // 更新状态
    m_moving = m_moveForward || m_moveBackward || m_moveLeft || m_moveRight || m_moveUp || m_moveDown;
}

} // namespace mc::client
