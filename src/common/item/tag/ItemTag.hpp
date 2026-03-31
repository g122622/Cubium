#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include <vector>
#include <unordered_set>

namespace mc {

class Item;

// Forward declaration for ItemStack (defined in item/core/ItemStack.hpp)
class ItemStack;

namespace item::tag {

/**
 * @brief 物品标签
 *
 * 用于将物品分组以便配方和功能判断。
 * 参考: net.minecraft.tags.ITag
 *
 * 用法示例:
 * @code
 * // 检查物品是否在标签中
 * if (item.isIn(ItemTags::PLANKS)) {
 *     // 物品是木板
 * }
 *
 * // 检查物品堆是否匹配标签
 * if (ItemTags::LOGS.contains(stack)) {
 *     // 物品堆是原木
 * }
 * @endcode
 */
class ItemTag {
public:
    /**
     * @brief 构造物品标签
     * @param id 标签资源位置
     */
    explicit ItemTag(ResourceLocation id);

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加物品到标签
     * @param item 物品指针
     */
    void add(const Item* item);

    /**
     * @brief 检查物品是否在标签中
     * @param item 物品指针
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Item* item) const;

    /**
     * @brief 检查物品堆是否在标签中
     * @param stack 物品堆
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ItemStack& stack) const;

    /**
     * @brief 获取标签中的所有物品
     */
    [[nodiscard]] const std::unordered_set<const Item*>& getItems() const { return m_items; }

    /**
     * @brief 获取标签中的所有物品（有序列表）
     */
    [[nodiscard]] std::vector<const Item*> getItemsList() const;

private:
    ResourceLocation m_id;
    std::unordered_set<const Item*> m_items;
};

} // namespace item::tag
} // namespace mc
