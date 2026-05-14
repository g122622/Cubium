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

#include "ItemTags.hpp"
#include "../core/ItemRegistry.hpp"

namespace mc {
namespace item::tag {

bool ItemTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& ItemTags::tags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>> s_tags;
    return s_tags;
}

ItemTag& ItemTags::registerTag(const ResourceLocation& id)
{
    auto& allTags = tags();

    auto it = allTags.find(id);
    if (it != allTags.end()) {
        return *it->second;
    }

    auto inserted = allTags.emplace(id, std::make_unique<ItemTag>(id));
    return *inserted.first->second;
}

ItemTag* ItemTags::getTag(const ResourceLocation& id)
{
    auto& allTags = tags();
    auto it = allTags.find(id);
    if (it == allTags.end()) {
        return nullptr;
    }
    return it->second.get();
}

ItemTag* ItemTags::getTag(const std::string& id)
{
    return getTag(ResourceLocation(id));
}

void ItemTags::forEachTag(std::function<void(ItemTag&)> callback)
{
    for (auto& pair : tags()) {
        callback(*pair.second);
    }
}

// ============================================================================
// 内置物品标签
// ============================================================================

ItemTag& ItemTags::FLOWERS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "flowers"));
    }
    return *tag;
}

void ItemTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& allTags = tags();

    // 创建 FLOWERS 标签
    // 参考 MC 1.16.5: ItemTags.FLOWERS
    // 包含所有小型花朵和大型花朵
    auto flowers = std::make_unique<ItemTag>(ResourceLocation("minecraft", "flowers"));

    // 小型花朵（单格）
    // 参考: VanillaBlocks.hpp 中的定义
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "poppy")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_orchid")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "allium")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "azure_bluet")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxeye_daisy")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lily_of_the_valley")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cornflower")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wither_rose")));

    // 大型花朵（双格）
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "sunflower")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lilac")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "rose_bush")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "peony")));

    allTags[flowers->getId()] = std::move(flowers);

    s_initialized = true;
}

} // namespace item::tag
} // namespace mc