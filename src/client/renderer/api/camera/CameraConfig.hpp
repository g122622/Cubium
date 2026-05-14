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

namespace mc::client::renderer::api {

/**
 * @brief 相机投影模式
 */
enum class ProjectionMode : u8 {
    Perspective, // 透视投影
    Orthographic // 正交投影
};

/**
 * @brief 相机配置
 */
struct CameraConfig {
    // 视角设置
    f64 fov = 70.0f;                // 视野角度（度）
    f64 aspectRatio = 16.0f / 9.0f; // 宽高比
    f64 nearPlane = 0.1f;           // 近裁剪面
    f64 farPlane = 1000.0f;         // 远裁剪面

    // 正交投影设置
    f64 orthoSize = 10.0f; // 正交投影大小

    // 移动设置
    f64 moveSpeed = 5.0f;        // 移动速度（单位/秒）
    f64 sprintMultiplier = 2.0f; // 冲刺倍率
    f64 sneakMultiplier = 0.3f;  // 潜行倍率

    // 视角设置
    f64 mouseSensitivity = 0.1f; // 鼠标灵敏度
    f64 pitchLimit = 89.0f;      // 俯仰角限制（度）

    // 默认投影模式
    ProjectionMode projectionMode = ProjectionMode::Perspective;
};

} // namespace mc::client::renderer::api
