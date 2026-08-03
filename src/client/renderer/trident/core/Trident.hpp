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

/**
 * @file Trident.hpp
 * @brief Trident 渲染引擎统一头文件
 *
 * 包含所有 Trident 组件的头文件。
 * 使用命名空间 mc::client::renderer::trident
 */

// 核心组件
#include "TridentContext.hpp"
#include "TridentSwapchain.hpp"

// 渲染管理器
#include "render/DescriptorManager.hpp"
#include "render/FrameManager.hpp"
#include "render/RenderPassManager.hpp"
#include "render/UniformManager.hpp"

// API 接口
#include "client/renderer/api/TridentApi.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc::client::renderer::trident {

/**
 * @brief Trident 渲染引擎版本
 */
constexpr u32 TRIDENT_VERSION_MAJOR = 0;
constexpr u32 TRIDENT_VERSION_MINOR = 1;
constexpr u32 TRIDENT_VERSION_PATCH = 0;

/**
 * @brief 获取 Trident 版本字符串
 */
inline std::string getTridentVersion()
{
    return std::to_string(TRIDENT_VERSION_MAJOR) + "." + std::to_string(TRIDENT_VERSION_MINOR) + "." +
        std::to_string(TRIDENT_VERSION_PATCH);
}

} // namespace mc::client::renderer::trident
