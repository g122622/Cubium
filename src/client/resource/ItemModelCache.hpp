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

#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>

#include "ItemModelLoader.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"

namespace mc::client::resource {

/**
 * @brief 物品模型缓存
 *
 * 单例模式，缓存所有已加载的物品模型
 */
class ItemModelCache {
public:
    /**
     * @brief 获取单例实例
     */
    static ItemModelCache& instance();

    /**
     * @brief 初始化缓存
     * @param resourcePacks 资源包列表（按优先级从高到低）
     * @return 是否成功
     */
    bool initialize(const std::vector<ResourcePackPtr>& resourcePacks);

    /**
     * @brief 清理缓存
     */
    void cleanup();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取物品模型（通过物品ID）
     * @param itemId 物品ID
     * @return 模型指针，如果不存在返回nullptr
     */
    [[nodiscard]] const BakedItemModel* getItemModel(u32 itemId) const;

    /**
     * @brief 获取物品模型（通过资源位置）
     * @param location 模型资源位置
     * @return 模型指针，如果不存在返回nullptr
     */
    [[nodiscard]] const BakedItemModel* getItemModel(const ResourceLocation& location) const;

    /**
     * @brief 获取物品模型（通过Item引用）
     * @param item 物品引用
     * @return 模型指针，如果不存在返回nullptr
     */
    [[nodiscard]] const BakedItemModel* getItemModel(const ::mc::Item& item) const;

    /**
     * @brief 获取指定显示上下文的变换矩阵
     * @param itemId 物品ID
     * @param context 显示上下文
     * @return 变换矩阵
     */
    [[nodiscard]] glm::mat4 getTransform(u32 itemId, ItemDisplayContext context) const;

    /**
     * @brief 获取指定显示上下文的变换矩阵
     * @param model 物品模型
     * @param context 显示上下文
     * @return 变换矩阵
     */
    [[nodiscard]] glm::mat4 getTransform(const BakedItemModel& model, ItemDisplayContext context) const;

    /**
     * @brief 预加载物品模型
     * @param itemRegistry 物品注册表
     */
    void preloadModels();

    /**
     * @brief 获取模型加载器
     */
    [[nodiscard]] ItemModelLoader& loader() { return *m_loader; }
    [[nodiscard]] const ItemModelLoader& loader() const { return *m_loader; }

private:
    ItemModelCache() = default;
    ~ItemModelCache() = default;
    ItemModelCache(const ItemModelCache&) = delete;
    ItemModelCache& operator=(const ItemModelCache&) = delete;

    bool m_initialized = false;
    std::unique_ptr<ItemModelLoader> m_loader;

    // 物品ID到模型的快速查找缓存
    mutable std::unordered_map<u32, const BakedItemModel*> m_itemIdCache;
};

} // namespace mc::client::resource
