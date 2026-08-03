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

#include "CameraConfig.hpp"
#include "common/core/Types.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/glm.hpp>

namespace mc::client::renderer::api {

/**
 * @brief 相机接口
 *
 * 平台无关的相机抽象接口。
 * 提供视图和投影矩阵计算。
 */
class ICamera {
public:
    virtual ~ICamera() = default;

    // 更新
    virtual void update(f64 deltaTime) = 0;

    // 位置
    virtual void setPosition(const glm::dvec3& position) = 0;
    virtual void setPosition(f64 x, f64 y, f64 z) = 0;
    [[nodiscard]] virtual const glm::dvec3& position() const = 0;

    // 旋转（欧拉角，度）
    virtual void setRotation(const glm::dvec3& rotation) = 0;
    virtual void setRotation(f64 pitch, f64 yaw, f64 roll = 0.0f) = 0;
    [[nodiscard]] virtual const glm::dvec3& rotation() const = 0;

    // 俯仰和偏航
    [[nodiscard]] virtual f64 pitch() const = 0;
    [[nodiscard]] virtual f64 yaw() const = 0;
    [[nodiscard]] virtual f64 roll() const = 0;

    virtual void setPitch(f64 pitch) = 0;
    virtual void setYaw(f64 yaw) = 0;
    virtual void setRoll(f64 roll) = 0;

    // 方向向量
    [[nodiscard]] virtual glm::dvec3 forward() const = 0;
    [[nodiscard]] virtual glm::dvec3 right() const = 0;
    [[nodiscard]] virtual glm::dvec3 up() const = 0;

    // 移动
    virtual void moveForward(f64 distance) = 0;
    virtual void moveRight(f64 distance) = 0;
    virtual void moveUp(f64 distance) = 0;

    // 旋转
    virtual void rotate(f64 pitchDelta, f64 yawDelta) = 0;
    virtual void look(f64 mouseDeltaX, f64 mouseDeltaY) = 0;

    // 投影
    virtual void setProjectionMode(ProjectionMode mode) = 0;
    virtual void setFOV(f64 fov) = 0;
    virtual void setAspectRatio(f64 aspectRatio) = 0;
    virtual void setNearFar(f64 nearPlane, f64 farPlane) = 0;
    virtual void setOrthoSize(f64 size) = 0;

    [[nodiscard]] virtual ProjectionMode projectionMode() const = 0;
    [[nodiscard]] virtual f64 fov() const = 0;
    [[nodiscard]] virtual f64 aspectRatio() const = 0;
    [[nodiscard]] virtual f64 nearPlane() const = 0;
    [[nodiscard]] virtual f64 farPlane() const = 0;

    // 矩阵
    [[nodiscard]] virtual const glm::mat4& viewMatrix() const = 0;
    [[nodiscard]] virtual const glm::mat4& projectionMatrix() const = 0;
    [[nodiscard]] virtual const glm::mat4& viewProjectionMatrix() const = 0;

    // 配置
    virtual void setConfig(const CameraConfig& config) = 0;
    [[nodiscard]] virtual const CameraConfig& config() const = 0;

    // 移动速度
    virtual void setMoveSpeed(f64 speed) = 0;
    [[nodiscard]] virtual f64 moveSpeed() const = 0;

    // 鼠标灵敏度
    virtual void setMouseSensitivity(f64 sensitivity) = 0;
    [[nodiscard]] virtual f64 mouseSensitivity() const = 0;

    // 脏标记
    [[nodiscard]] virtual bool isDirty() const = 0;
    virtual void markClean() = 0;
};

} // namespace mc::client::renderer::api
