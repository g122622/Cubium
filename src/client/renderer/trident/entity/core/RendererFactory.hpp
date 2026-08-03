/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mc::client::renderer::entity {

class EntityRendererManager;

namespace core {

/**
 * @brief 渲染器创建函数类型
 *
 * 返回一个新创建的 EntityRenderer 实例
 */
using RendererCreator = std::function<std::unique_ptr<EntityRenderer>()>;

/**
 * @brief 实体渲染器工厂
 *
 * 统一管理所有实体渲染器的创建。使用注册表模式替代巨型 if-else 链。
 * 与 ModelFactory 设计保持一致。
 *
 * 使用方式：
 * @code
 * // 注册渲染器（通常在初始化时调用）
 * RendererFactory::instance().registerRenderer("minecraft:pig", []() {
 *     return std::make_unique<PigRenderer>();
 * });
 *
 * // 创建渲染器
 * auto renderer = RendererFactory::instance().createRenderer("minecraft:pig");
 * if (renderer) {
 *     // 使用渲染器...
 * }
 * @endcode
 */
class RendererFactory {
public:
    /**
     * @brief 获取单例实例
     */
    static RendererFactory& instance();

    /**
     * @brief 注册渲染器创建器
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @param creator 渲染器创建函数
     */
    void registerRenderer(const std::string& entityTypeId, RendererCreator creator);

    /**
     * @brief 创建渲染器
     * @param entityTypeId 实体类型ID
     * @return 渲染器实例，如果类型未注册则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<EntityRenderer> createRenderer(const std::string& entityTypeId) const;

    /**
     * @brief 检查是否已注册
     * @param entityTypeId 实体类型ID
     * @return 是否已注册
     */
    [[nodiscard]] bool hasRenderer(const std::string& entityTypeId) const;

    /**
     * @brief 获取已注册的渲染器类型数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 标记为已初始化
     *
     * 由 initializeRendererRegistration() 调用
     */
    static void markInitialized() { s_initialized = true; }

    // 禁止拷贝和移动
    RendererFactory(const RendererFactory&) = delete;
    RendererFactory& operator=(const RendererFactory&) = delete;
    RendererFactory(RendererFactory&&) = delete;
    RendererFactory& operator=(RendererFactory&&) = delete;

private:
    RendererFactory() = default;

    /**
     * @brief 规范化实体类型ID
     *
     * 确保ID带有命名空间前缀（如 "minecraft:"），缺失时自动补全
     */
    static std::string _normalizeEntityTypeId(const std::string& entityTypeId);

    std::unordered_map<std::string, RendererCreator> m_creators;
    mutable std::mutex m_mutex;
    static bool s_initialized;
};

/**
 * @brief 便捷宏：注册实体渲染器
 *
 * 使用方式：
 * @code
 * REGISTER_ENTITY_RENDERER("minecraft:pig", PigRenderer);
 * @endcode
 */
#define REGISTER_ENTITY_RENDERER(typeId, RendererClass)                                 \
    ::mc::client::renderer::entity::core::RendererFactory::instance().registerRenderer( \
        typeId, []() { return std::make_unique<::mc::client::renderer::entity::renderer::RendererClass>(); })

} // namespace core
} // namespace mc::client::renderer::entity
