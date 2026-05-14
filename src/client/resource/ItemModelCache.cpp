#include "ItemModelCache.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::resource {

ItemModelCache& ItemModelCache::instance()
{
    static ItemModelCache instance;
    return instance;
}

bool ItemModelCache::initialize(const std::vector<IResourcePack*>& resourcePacks)
{
    if (m_initialized) {
        return true;
    }

    MC_ASSERT(!resourcePacks.empty());

    m_loader = std::make_unique<ItemModelLoader>(resourcePacks);
    m_initialized = true;

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
    if (!m_initialized || !m_loader) {
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
    if (!m_initialized || !m_loader) {
        return;
    }

    // 遍历所有物品并预加载模型
    ItemRegistry::instance().forEachItem([this](::mc::Item& item) {
        u32 itemId = item.itemId();
        static_cast<void>(getItemModel(itemId));
    });
}

} // namespace mc::client::resource
