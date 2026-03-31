#pragma once

#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include <vector>
#include <functional>
#include <optional>

namespace mc {
namespace crafting {

/**
 * @brief 原料匹配器，用于配方中检查物品是否匹配
 *
 * Ingredient 是配方系统的核心组件，用于定义配方所需的输入物品。
 * 支持三种匹配方式：
 * 1. 单个物品：fromItem()
 * 2. 多个物品：fromItems()
 * 3. 物品标签：fromTag()（未来实现）
 *
 * 使用示例：
 * @code
 * // 匹配橡木木板
 * Ingredient planks = Ingredient::fromItem(Items::OAK_PLANKS);
 *
 * // 匹配任意木板
 * Ingredient anyPlanks = Ingredient::fromItems({
 *     Items::OAK_PLANKS, Items::SPRUCE_PLANKS,
 *     Items::BIRCH_PLANKS, Items::JUNGLE_PLANKS
 * });
 *
 * // 检查匹配
 * ItemStack stack(Items::OAK_PLANKS, 1);
 * if (planks.test(stack)) {
 *     // 匹配成功
 * }
 * @endcode
 *
 * 注意事项：
 * - 空Ingredient匹配任何空物品
 * - Ingredient是不可变的，创建后不应修改
 * - getMatchingStacks()返回的是所有可能匹配的物品堆
 */
class Ingredient {
public:
    /**
     * @brief 默认构造函数，创建空Ingredient
     *
     * 空Ingredient不匹配任何物品（isEmpty()返回true）
     */
    Ingredient() = default;

    /**
     * @brief 从单个物品创建Ingredient
     * @param item 要匹配的物品
     * @return 匹配该物品的Ingredient
     */
    static Ingredient fromItem(const Item& item);

    /**
     * @brief 从物品指针创建Ingredient
     * @param item 要匹配的物品指针（可为nullptr）
     * @return 匹配该物品的Ingredient
     */
    static Ingredient fromItem(const Item* item);

    /**
     * @brief 从多个物品创建Ingredient
     * @param items 要匹配的物品列表
     * @return 匹配任一物品的Ingredient
     *
     * 注意：如果items为空，返回空Ingredient
     */
    static Ingredient fromItems(std::vector<const Item*> items);

    /**
     * @brief 从物品标签创建Ingredient
     * @param tag 物品标签名（如 "minecraft:planks"）
     * @return 匹配标签内所有物品的Ingredient
     *
     * 注意：当前版本标签系统尚未完全实现，此方法暂时返回空Ingredient
     */
    static Ingredient fromTag(const String& tag);

    /**
     * @brief 从物品堆列表创建Ingredient
     * @param stacks 匹配的物品堆列表
     * @return 匹配任一物品堆的Ingredient
     *
     * 注意：物品堆的数量不影响匹配，只检查物品类型
     */
    static Ingredient fromStacks(std::vector<ItemStack> stacks);

    /**
     * @brief 检查物品堆是否匹配此Ingredient
     * @param stack 要检查的物品堆
     * @return 如果匹配返回true
     *
     * 匹配规则：
     * - 空Ingredient不匹配任何物品（包括空堆）
     * - 空物品堆不匹配任何非空Ingredient
     * - 检查物品类型是否在匹配列表中
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 检查物品是否匹配此Ingredient
     * @param item 要检查的物品
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool test(const Item& item) const;
    [[nodiscard]] bool test(const Item* item) const;

    /**
     * @brief 获取所有匹配的物品堆
     * @return 匹配的物品堆列表
     *
     * 注意：返回的物品堆数量均为1
     */
    [[nodiscard]] const std::vector<ItemStack>& getMatchingStacks() const {
        return m_matchingStacks;
    }

    /**
     * @brief 检查是否为空Ingredient
     * @return 如果没有任何匹配项返回true
     */
    [[nodiscard]] bool isEmpty() const {
        return m_matchingStacks.empty() && !m_hasTag;
    }

    /**
     * @brief 检查是否有物品标签
     * @return 如果使用标签匹配返回true
     */
    [[nodiscard]] bool hasTag() const {
        return m_hasTag;
    }

    /**
     * @brief 获取物品标签（如果有）
     * @return 物品标签名，如果没有则返回空字符串
     */
    [[nodiscard]] const String& getTag() const {
        return m_tag;
    }

    /**
     * @brief 比较两个Ingredient是否相等
     * @param other 要比较的Ingredient
     * @return 如果匹配相同的物品返回true
     */
    bool operator==(const Ingredient& other) const;

    /**
     * @brief 比较两个Ingredient是否不相等
     */
    bool operator!=(const Ingredient& other) const {
        return !(*this == other);
    }

    /**
     * @brief 获取用于哈希的值
     * @return 哈希值
     */
    size_t hash() const;

private:
    std::vector<ItemStack> m_matchingStacks;
    String m_tag;
    bool m_hasTag = false;

    // 用于缓存解析后的标签物品
    mutable bool m_tagResolved = false;
    mutable std::vector<const Item*> m_tagItems;
};

} // namespace crafting
} // namespace mc

// std::hash 特化，允许Ingredient用于unordered容器
namespace std {
template<>
struct hash<mc::crafting::Ingredient> {
    size_t operator()(const mc::crafting::Ingredient& ingredient) const {
        return ingredient.hash();
    }
};
}
