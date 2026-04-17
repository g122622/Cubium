#pragma once

#include "../../resource/ResourceLocation.hpp"
#include "ItemTag.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace item::tag {

/**
 * @brief 物品标签注册表
 *
 * 负责集中管理 ItemTag 的创建、查询和遍历。
 */
class ItemTags {
public:
    /**
     * @brief 注册或获取指定ID的标签。
     * @param id 标签ID。
     * @return 标签引用。
     */
    static ItemTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 根据ID获取标签。
     * @param id 标签ID。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const ResourceLocation& id);

    /**
     * @brief 根据ID字符串获取标签。
     * @param id 标签ID字符串（namespace:path）。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const String& id);

    /**
     * @brief 遍历全部标签。
     * @param callback 回调函数。
     */
    static void forEachTag(std::function<void(ItemTag&)> callback);

private:
    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& tags();
};

} // namespace item::tag
} // namespace mc