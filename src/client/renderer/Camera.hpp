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

#include "api/camera/ICamera.hpp"
#include "client/renderer/api/camera/CameraConfig.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mc::client {

// 相机投影模式 (使用 api 命名空间中的枚举)
using ProjectionMode = renderer::api::ProjectionMode;

// 相机配置 (使用 api 命名空间中的配置)
using CameraConfig = renderer::api::CameraConfig;

// 相机类 - 第一人称视角，实现 ICamera 接口
class Camera : public renderer::api::ICamera {
public:
    Camera() = default;
    explicit Camera(const CameraConfig& config);
    ~Camera() override = default;

    // ========================================================================
    // ICamera 接口实现
    // ========================================================================

    // 更新
    void update(f64 deltaTime) override;

    // 位置
    void setPosition(const glm::dvec3& position) override;
    void setPosition(f64 x, f64 y, f64 z) override;
    [[nodiscard]] const glm::dvec3& position() const override { return m_position; }

    // 旋转（欧拉角，度）
    void setRotation(const glm::dvec3& rotation) override;
    void setRotation(f64 pitch, f64 yaw, f64 roll = 0.0f) override;
    [[nodiscard]] const glm::dvec3& rotation() const override { return m_rotation; }

    // 俯仰和偏航（更常用）
    [[nodiscard]] f64 pitch() const override { return m_rotation.x; }
    [[nodiscard]] f64 yaw() const override { return m_rotation.y; }
    [[nodiscard]] f64 roll() const override { return m_rotation.z; }

    void setPitch(f64 pitch) override;
    void setYaw(f64 yaw) override;
    void setRoll(f64 roll) override;

    // 方向向量
    [[nodiscard]] glm::dvec3 forward() const override;
    [[nodiscard]] glm::dvec3 right() const override;
    [[nodiscard]] glm::dvec3 up() const override;

    // 移动
    void moveForward(f64 distance) override;
    void moveRight(f64 distance) override;
    void moveUp(f64 distance) override;

    // 旋转
    void rotate(f64 pitchDelta, f64 yawDelta) override;
    void look(f64 mouseDeltaX, f64 mouseDeltaY) override;

    // 投影
    void setProjectionMode(ProjectionMode mode) override;
    void setFOV(f64 fov) override;
    void setAspectRatio(f64 aspectRatio) override;
    void setNearFar(f64 nearPlane, f64 farPlane) override;
    void setOrthoSize(f64 size) override;

    [[nodiscard]] ProjectionMode projectionMode() const override { return m_config.projectionMode; }
    [[nodiscard]] f64 fov() const override { return m_config.fov; }
    [[nodiscard]] f64 aspectRatio() const override { return m_config.aspectRatio; }
    [[nodiscard]] f64 nearPlane() const override { return m_config.nearPlane; }
    [[nodiscard]] f64 farPlane() const override { return m_config.farPlane; }

    // 矩阵
    [[nodiscard]] const glm::mat4& viewMatrix() const override { return m_viewMatrix; }
    [[nodiscard]] const glm::mat4& projectionMatrix() const override { return m_projectionMatrix; }
    [[nodiscard]] const glm::mat4& viewProjectionMatrix() const override { return m_viewProjectionMatrix; }

    /**
     * @brief 设置渲染视图附加变换
     */
    void setViewTransform(const glm::mat4& transform);

    /**
     * @brief 清除渲染视图附加变换
     */
    void clearViewTransform();

    // 配置
    void setConfig(const CameraConfig& config) override;
    [[nodiscard]] const CameraConfig& config() const override { return m_config; }

    // 移动速度
    void setMoveSpeed(f64 speed) override { m_config.moveSpeed = speed; }
    [[nodiscard]] f64 moveSpeed() const override { return m_config.moveSpeed; }

    // 鼠标灵敏度
    void setMouseSensitivity(f64 sensitivity) override { m_config.mouseSensitivity = sensitivity; }
    [[nodiscard]] f64 mouseSensitivity() const override { return m_config.mouseSensitivity; }

    // 脏标记
    [[nodiscard]] bool isDirty() const override { return m_dirty; }
    void markClean() override { m_dirty = false; }

private:
    void _updateVectors();
    void _updateViewMatrix();
    void _updateProjectionMatrix();
    void _updateViewProjectionMatrix();

    CameraConfig m_config;

    // 位置和旋转
    glm::dvec3 m_position{0.0, 0.0, 0.0};
    glm::dvec3 m_rotation{0.0, 0.0, 0.0}; // pitch, yaw, roll (度)

    // 方向向量
    glm::dvec3 m_forward{0.0, 0.0, -1.0};
    glm::dvec3 m_right{1.0, 0.0, 0.0};
    glm::dvec3 m_up{0.0, 1.0, 0.0};

    // 矩阵
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_baseViewMatrix{1.0f};
    glm::mat4 m_viewTransform{1.0f};
    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewProjectionMatrix{1.0f};

    // 脏标记
    bool m_dirty = true;
    bool m_viewDirty = true;
    bool m_projectionDirty = true;
};

// 相机控制器 - 处理输入
class CameraController {
public:
    CameraController() = default;
    explicit CameraController(Camera* camera);

    // 设置控制的相机
    void setCamera(Camera* camera);
    [[nodiscard]] Camera* camera() { return m_camera; }

    // 输入处理
    void handleKeyboardInput(i32 key, i32 action);
    void handleMouseMove(f64 deltaX, f64 deltaY);
    void handleScroll(f64 deltaY);

    // 更新
    void update(f64 deltaTime);

    // 设置按键映射
    void setMoveForwardKey(i32 key) { m_moveForwardKey = key; }
    void setMoveBackwardKey(i32 key) { m_moveBackwardKey = key; }
    void setMoveLeftKey(i32 key) { m_moveLeftKey = key; }
    void setMoveRightKey(i32 key) { m_moveRightKey = key; }
    void setMoveUpKey(i32 key) { m_moveUpKey = key; }
    void setMoveDownKey(i32 key) { m_moveDownKey = key; }
    void setSprintKey(i32 key) { m_sprintKey = key; }
    void setSneakKey(i32 key) { m_sneakKey = key; }

    // 状态
    [[nodiscard]] bool isMoving() const { return m_moving; }
    [[nodiscard]] bool isSprinting() const { return m_sprinting; }
    [[nodiscard]] bool isSneaking() const { return m_sneaking; }

private:
    Camera* m_camera = nullptr;

    // 按键状态
    bool m_moveForward = false;
    bool m_moveBackward = false;
    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_moveUp = false;
    bool m_moveDown = false;
    bool m_sprinting = false;
    bool m_sneaking = false;

    // 按键映射 (默认WASD + Space + Shift)
    i32 m_moveForwardKey = 87;  // W
    i32 m_moveBackwardKey = 83; // S
    i32 m_moveLeftKey = 65;     // A
    i32 m_moveRightKey = 68;    // D
    i32 m_moveUpKey = 32;       // Space
    i32 m_moveDownKey = 340;    // Left Shift
    i32 m_sprintKey = 340;      // Left Shift (与moveDown共享)
    i32 m_sneakKey = 341;       // Left Ctrl

    // 状态
    bool m_moving = false;
};

} // namespace mc::client
