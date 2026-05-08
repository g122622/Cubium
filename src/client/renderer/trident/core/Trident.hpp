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
#include "render/RenderPassManager.hpp"
#include "render/FrameManager.hpp"
#include "render/DescriptorManager.hpp"
#include "render/UniformManager.hpp"

// API 接口
#include "../../api/TridentApi.hpp"

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
inline std::string getTridentVersion() {
    return std::to_string(TRIDENT_VERSION_MAJOR) + "." +
           std::to_string(TRIDENT_VERSION_MINOR) + "." +
           std::to_string(TRIDENT_VERSION_PATCH);
}

} // namespace mc::client::renderer::trident
