#pragma once

#include "../../core/Item.hpp"
#include "../../enchantment/Enchantment.hpp"
#include <vector>

namespace mc {

// Forward declarations
class ItemStack;

namespace item::items {

/**
 * @brief 附魔书物品
 *
 * 存储附魔的书籍，可用于铁砧将附魔应用到物品上。
 * 附魔书始终显示附魔光效。
 *
 * 参考: net.minecraft.item.EnchantedBookItem
 *
 * NBT结构:
 * - StoredEnchantments: JSON数组 附魔列表
 *   - id: std::string 附魔ID
 *   - lvl: Int 附魔等级
 */
class EnchantedBookItem : public Item {
public:
    /**
     * @brief 附魔数据结构
     */
    struct EnchantmentData {
        const enchant::Enchantment* enchantment;
        i32 level;
    };

    /**
     * @brief 构造附魔书物品
     * @param properties 物品属性
     */
    explicit EnchantedBookItem(ItemProperties properties);

    // ========== 物品重写方法 ==========

    /**
     * @brief 附魔书始终有附魔光效
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const override {
        (void)stack;
        return true;
    }

    /**
     * @brief 获取附魔能力
     * @return 附魔书的附魔能力（MC 1.16.5: 1）
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return 1; }

    // ========== 附魔书特有方法 ==========

    /**
     * @brief 获取附魔书中存储的附魔列表
     * @param stack 物品堆
     * @return 附魔数据列表
     */
    [[nodiscard]] static std::vector<EnchantmentData> getEnchantments(const ItemStack& stack);

    /**
     * @brief 向附魔书添加附魔
     * @param stack 物品堆
     * @param enchantment 附魔
     * @param level 等级
     *
     * 如果已有相同附魔，则升级到更高等级
     */
    static void addEnchantment(ItemStack& stack, const enchant::Enchantment& enchantment, i32 level);

    /**
     * @brief 检查附魔书是否有附魔
     * @param stack 物品堆
     * @return 是否有附魔
     */
    [[nodiscard]] static bool hasEnchantments(const ItemStack& stack);

    /**
     * @brief 获取附魔书的总附魔数量
     * @param stack 物品堆
     * @return 附魔数量
     */
    [[nodiscard]] static size_t getEnchantmentCount(const ItemStack& stack);
};

} // namespace item::items
} // namespace mc
