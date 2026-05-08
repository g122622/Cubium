#include "ItemTags.hpp"

namespace mc {
namespace item::tag {

std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& ItemTags::tags() {
    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>> s_tags;
    return s_tags;
}

ItemTag& ItemTags::registerTag(const ResourceLocation& id) {
    auto& allTags = tags();

    auto it = allTags.find(id);
    if (it != allTags.end()) {
        return *it->second;
    }

    auto inserted = allTags.emplace(id, std::make_unique<ItemTag>(id));
    return *inserted.first->second;
}

ItemTag* ItemTags::getTag(const ResourceLocation& id) {
    auto& allTags = tags();
    auto it = allTags.find(id);
    if (it == allTags.end()) {
        return nullptr;
    }
    return it->second.get();
}

ItemTag* ItemTags::getTag(const std::string& id) {
    return getTag(ResourceLocation(id));
}

void ItemTags::forEachTag(std::function<void(ItemTag&)> callback) {
    for (auto& pair : tags()) {
        callback(*pair.second);
    }
}

} // namespace item::tag
} // namespace mc