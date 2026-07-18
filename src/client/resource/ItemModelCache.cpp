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

#include "ItemModelCache.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client::resource {

ItemModelCache& ItemModelCache::instance()
{
    static ItemModelCache instance;
    return instance;
}

bool ItemModelCache::initialize(const std::vector<ResourcePackPtr>& resourcePacks)
{
    if (m_initialized) {
        return true;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ItemModelCache::initialize");

    MC_ASSERT(!resourcePacks.empty());

    m_loader = std::make_unique<ItemModelLoader>(resourcePacks);
    m_initialized = true;

    // 全量预加载所有资源包中的物品模型，避免运行时延迟加载造成的卡顿。
    // 失败不阻塞初始化 —— 单个模型烘焙失败已由 loader 内部记录警告。
    auto preloadResult = m_loader->loadAllModels();
    if (preloadResult.failed()) {
        spdlog::warn("ItemModelCache: loadAllModels failed: {}", preloadResult.error().message());
    }

    return true;
}

void ItemModelCache::cleanup()
{
    m_loader.reset();
    m_itemIdCache.clear();
    m_initialized = false;
}

const BakedItemModel* ItemModelCache::getItemModel(u32 itemId) const
{
    // 检查缓存
    auto cacheIt = m_itemIdCache.find(itemId);
    if (cacheIt != m_itemIdCache.end()) {
        return cacheIt->second;
    }

    // 从 ItemRegistry 获取物品信息
    auto* item = ItemRegistry::instance().getItem(itemId);
    if (item == nullptr) {
        m_itemIdCache[itemId] = nullptr;
        return nullptr;
    }

    // 构建模型位置
    const ResourceLocation& itemLoc = item->itemLocation();
    ResourceLocation modelLoc(itemLoc.namespace_(), "item/" + itemLoc.path());

    // 加载模型
    const BakedItemModel* model = getItemModel(modelLoc);

    // 缓存结果
    m_itemIdCache[itemId] = model;

    return model;
}

const BakedItemModel* ItemModelCache::getItemModel(const ResourceLocation& location) const
{
    if (!m_initialized) {
        return nullptr;
    }

    // 检查是否已加载
    const BakedItemModel* cached = m_loader->getModel(location);
    if (cached != nullptr) {
        return cached;
    }

    // 烘焙模型
    auto result = m_loader->bakeModel(location);
    if (!result.success()) {
        return nullptr;
    }

    return m_loader->getModel(location);
}

const BakedItemModel* ItemModelCache::getItemModel(const ::mc::Item& item) const
{
    return getItemModel(item.itemId());
}

glm::mat4 ItemModelCache::getTransform(u32 itemId, ItemDisplayContext context) const
{
    const BakedItemModel* model = getItemModel(itemId);
    if (model == nullptr) {
        return glm::mat4(1.0f);
    }
    return getTransform(*model, context);
}

glm::mat4 ItemModelCache::getTransform(const BakedItemModel& model, ItemDisplayContext context) const
{
    return model.getTransform(context).toMatrix();
}

void ItemModelCache::preloadModels()
{
    if (!m_initialized) {
        return;
    }

    // 遍历所有物品并预加载模型
    ItemRegistry::instance().forEachItem([this](::mc::Item& item) {
        u32 itemId = item.itemId();
        static_cast<void>(getItemModel(itemId));
    });
}

} // namespace mc::client::resource
