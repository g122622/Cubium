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

#include "EntityModel.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mc::client::renderer::entity::model {

/**
 * @brief 模型创建函数类型
 *
 * 返回一个新创建的 EntityModel 实例
 */
using ModelCreator = std::function<std::unique_ptr<EntityModel>()>;

/**
 * @brief 带动画上下文的模型创建函数类型
 *
 * 某些模型需要额外的动画状态设置，使用此类型
 * TODO: 尚未实现带上下文的模型注册与创建接口
 */
using ModelCreatorWithContext = std::function<std::unique_ptr<EntityModel>(const std::string& entityTypeId)>;

/**
 * @brief 实体模型工厂
 *
 * 统一管理所有实体模型的创建。使用注册表模式替代巨型 if-else 链。
 */
class ModelFactory {
public:
    /**
     * @brief 获取单例实例
     */
    static ModelFactory& instance();

    /**
     * @brief 注册模型创建器
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @param creator 模型创建函数
     */
    void registerModel(const std::string& entityTypeId, ModelCreator creator);

    /**
     * @brief 创建模型
     * @param entityTypeId 实体类型ID
     * @return 模型实例，如果类型未注册则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<EntityModel> createModel(const std::string& entityTypeId) const;

    /**
     * @brief 检查是否已注册
     * @param entityTypeId 实体类型ID
     * @return 是否已注册
     */
    [[nodiscard]] bool hasModel(const std::string& entityTypeId) const;

    /**
     * @brief 获取已注册的模型类型数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 标记为已初始化
     *
     * 由 initializeModelRegistration() 调用
     */
    static void markInitialized() { s_initialized = true; }

    // 禁止拷贝和移动
    ModelFactory(const ModelFactory&) = delete;
    ModelFactory& operator=(const ModelFactory&) = delete;
    ModelFactory(ModelFactory&&) = delete;
    ModelFactory& operator=(ModelFactory&&) = delete;

private:
    ModelFactory() = default;

    /**
     * @brief 规范化实体类型ID（确保有命名空间前缀）
     * @param entityTypeId 原始实体类型ID
     * @return 规范化后的实体类型ID
     */
    [[nodiscard]] static std::string _normalizeEntityTypeId(const std::string& entityTypeId);

    std::unordered_map<std::string, ModelCreator> m_creators;
    mutable std::mutex m_mutex;
    static bool s_initialized;
};

/**
 * @brief 便捷宏：注册实体模型
 *
 * 使用方式：
 * @code
 * REGISTER_ENTITY_MODEL("minecraft:pig", PigModel);
 * @endcode
 */
#define REGISTER_ENTITY_MODEL(typeId, ModelClass)                                  \
    ::mc::client::renderer::entity::model::ModelFactory::instance().registerModel( \
        typeId, []() { return std::make_unique<::mc::client::renderer::entity::model::ModelClass>(); })

/**
 * @brief 便捷宏：注册带缩放参数的实体模型
 *
 * 使用方式：
 * @code
 * REGISTER_ENTITY_MODEL_WITH_SCALE("minecraft:cat", CatModel, 0.0f);
 * @endcode
 */
#define REGISTER_ENTITY_MODEL_WITH_SCALE(typeId, ModelClass, scale)                \
    ::mc::client::renderer::entity::model::ModelFactory::instance().registerModel( \
        typeId, []() { return std::make_unique<::mc::client::renderer::entity::model::ModelClass>(scale); })

} // namespace mc::client::renderer::entity::model
