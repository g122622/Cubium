#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "../../MinMaxBounds.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// 前向声明
namespace mc {
    class BlockState;
    class ItemStack;
    namespace nbt {
        namespace tags {
            struct compound_tag;
        }
    }
}

namespace mc::advancement {

/**
 * @brief 物品谓词
 *
 * 用于匹配物品的条件谓词，检查物品类型、数量、NBT等。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.ItemPredicate
 */
class ItemPredicate {
public:
    /**
     * @brief 默认构造（匹配任意物品）
     */
    ItemPredicate() = default;

    /**
     * @brief 构造物品谓词
     */
    ItemPredicate(
        std::optional<ResourceLocation> item,
        std::optional<i32> count,
        IntBounds durability,
        std::optional<ResourceLocation> potion,
        const nbt::tags::compound_tag* nbt
    );

    /**
     * @brief 检查物品是否匹配
     * @param stack 物品堆
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 检查是否匹配任意物品
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     */
    static Result<ItemPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getItem() const noexcept { return m_item; }
    [[nodiscard]] const std::optional<i32>& getCount() const noexcept { return m_count; }
    [[nodiscard]] const IntBounds& getDurability() const noexcept { return m_durability; }
    [[nodiscard]] const std::optional<ResourceLocation>& getPotion() const noexcept { return m_potion; }

private:
    std::optional<ResourceLocation> m_item;      ///< 物品ID
    std::optional<i32> m_count;                   ///< 数量
    IntBounds m_durability;                       ///< 耐久范围
    std::optional<ResourceLocation> m_potion;     ///< 药水类型
    // TODO: NBT匹配、附魔匹配等
    bool m_isAny = true;                          ///< 是否匹配任意物品
};

} // namespace mc::advancement
