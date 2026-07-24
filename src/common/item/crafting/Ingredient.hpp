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

#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 原料匹配器，用于配方中检查物品是否匹配
 *
 * Ingredient 是配方系统的核心组件，用于定义配方所需的输入物品。
 * 支持三种匹配方式：
 * 1. 单个物品：fromItem()
 * 2. 多个物品：fromItems()
 * 3. 物品标签：fromTag()
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
 * - 空 Ingredient（isEmpty() 返回 true）只匹配空物品堆
 * - Ingredient 是不可变的，创建后不应修改
 * - getMatchingStacks() 返回的是所有可能匹配的物品堆
 */
class Ingredient {
public:
    /**
     * @brief 空 Ingredient 常量
     *
     * 空 Ingredient 的行为：test() 只对空物品堆返回 true。
     */
    static const Ingredient EMPTY;

    /**
     * @brief 默认构造函数，创建空 Ingredient
     *
     * 空 Ingredient 只匹配空物品堆（isEmpty() 返回 true）。
     */
    Ingredient() = default;

    /**
     * @brief 拷贝构造函数
     */
    Ingredient(const Ingredient&) = default;

    /**
     * @brief 移动构造函数
     */
    Ingredient(Ingredient&&) noexcept = default;

    /**
     * @brief 拷贝赋值运算符
     */
    Ingredient& operator=(const Ingredient&) = default;

    /**
     * @brief 移动赋值运算符
     */
    Ingredient& operator=(Ingredient&&) noexcept = default;

    /**
     * @brief 析构函数
     */
    ~Ingredient() = default;

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
     * 标签Ingredient会尝试立即解析标签内容。如果标签尚未注册，
     * 则延迟到首次test()调用时解析。标签Ingredient的isSimple属性
     * 取决于标签中是否包含可损坏物品。
     */
    static Ingredient fromTag(const std::string& tag);

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
     * - 空 Ingredient（isEmpty() == true）只匹配空物品堆
     * - 非空 Ingredient 不匹配空物品堆
     * - 检查物品类型是否在匹配列表中（不检查 NBT）
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 检查物品是否匹配此Ingredient
     * @param item 要检查的物品
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool test(const Item& item) const;

    /**
     * @brief 检查物品指针是否匹配此Ingredient
     * @param item 要检查的物品指针（可为nullptr）
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool test(const Item* item) const;

    /**
     * @brief 获取所有匹配的物品堆
     * @return 匹配的物品堆列表
     *
     * 注意：返回的物品堆数量均为1。对于标签类型Ingredient，
     * 仅返回显式指定的物品堆，不包含标签解析后的物品。
     * 如需获取包含标签物品的完整列表，请使用getAllMatchingItems()。
     */
    [[nodiscard]] const std::vector<ItemStack>& getMatchingStacks() const { return m_matchingStacks; }

    /**
     * @brief 获取所有匹配的物品（包括标签解析后的物品）
     * @return 匹配的物品指针列表
     *
     * 对于标签类型Ingredient，会包含标签解析后的所有物品。
     * 对于非标签Ingredient，返回m_matchingStacks中对应的物品指针。
     * 标签解析在首次调用时延迟执行。
     */
    [[nodiscard]] std::vector<const Item*> getAllMatchingItems() const;

    /**
     * @brief 检查是否为简单原料
     * @return 如果不包含可损坏物品返回true
     *
     * 简单原料可以用于 RecipeItemHelper 优化匹配。
     * 如果原料包含可损坏物品（如工具），则需要更复杂的匹配逻辑。
     */
    [[nodiscard]] bool isSimple() const;

    /**
     * @brief 检查是否为空Ingredient
     * @return 如果没有任何匹配项返回true
     */
    [[nodiscard]] bool isEmpty() const { return m_matchingStacks.empty() && !m_hasTag; }

    /**
     * @brief 检查是否有物品标签
     * @return 如果使用标签匹配返回true
     */
    [[nodiscard]] bool hasTag() const { return m_hasTag; }

    /**
     * @brief 获取物品标签（如果有）
     * @return 物品标签名，如果没有则返回空字符串
     */
    [[nodiscard]] const std::string& getTag() const { return m_tag; }

    /**
     * @brief 合并多个原料为一个
     * @param parts 要合并的原料列表
     * @return 合并后的原料，匹配任一原料的物品
     */
    static Ingredient merge(const std::vector<Ingredient>& parts);

    /**
     * @brief 比较两个Ingredient是否相等
     * @param other 要比较的Ingredient
     * @return 如果匹配相同的物品返回true
     */
    bool operator==(const Ingredient& other) const;

    /**
     * @brief 比较两个Ingredient是否不相等
     */
    bool operator!=(const Ingredient& other) const { return !(*this == other); }

    /**
     * @brief 获取用于哈希的值
     * @return 哈希值
     */
    size_t hash() const;

    /**
     * @brief 判断原料是否没有匹配物品
     * @return 如果没有匹配物品返回true
     *
     * 注意：空标签也视为没有匹配物品。
     */
    [[nodiscard]] bool hasNoMatchingItems() const;

private:
    std::vector<ItemStack> m_matchingStacks;
    std::string m_tag;
    bool m_hasTag = false;
    mutable bool m_isSimple = true; ///< 是否为简单原料（不包含可损坏物品）

    // 用于缓存解析后的标签物品
    mutable bool m_tagResolved = false;
    mutable std::vector<const Item*> m_tagItems;

    /**
     * @brief 更新 isSimple 标志
     * 在构造后或标签延迟解析后调用，检查是否包含可损坏物品
     */
    void _updateSimple() const;

    /**
     * @brief 延迟解析标签
     * 在首次需要时解析标签中的物品列表
     */
    void _resolveTagIfNeeded() const;
};

} // namespace crafting
} // namespace mc

// std::hash 特化，允许Ingredient用于unordered容器
namespace std {
template <>
struct hash<mc::crafting::Ingredient> {
    size_t operator()(const mc::crafting::Ingredient& ingredient) const { return ingredient.hash(); }
};
} // namespace std
