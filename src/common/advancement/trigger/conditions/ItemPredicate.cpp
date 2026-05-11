#include "ItemPredicate.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

ItemPredicate::ItemPredicate(
    std::optional<ResourceLocation> item,
    std::optional<i32> count,
    IntBounds durability,
    std::optional<ResourceLocation> potion,
    const nbt::tags::compound_tag* nbt
)
    : m_item(std::move(item))
    , m_count(count)
    , m_durability(std::move(durability))
    , m_potion(std::move(potion))
    , m_isAny(!m_item.has_value() && !m_count.has_value() && m_durability.isUnbounded() && !m_potion.has_value())
{
    MC_UNUSED(nbt);
}

bool ItemPredicate::test(const ItemStack& stack) const {
    if (m_isAny) {
        return true;
    }

    if (stack.isEmpty()) {
        return false;
    }

    // 检查物品ID
    if (m_item.has_value()) {
        // TODO: 获取物品ID进行比较
        // if (stack.getItem().getId() != m_item.value()) return false;
    }

    // 检查数量
    if (m_count.has_value() && stack.getCount() != m_count.value()) {
        return false;
    }

    // 检查耐久
    if (!m_durability.isUnbounded()) {
        i32 durability = stack.getMaxDamage() - stack.getDamage();
        if (!m_durability.test(durability)) {
            return false;
        }
    }

    // TODO: 检查药水、NBT、附魔等

    return true;
}

bool ItemPredicate::isAny() const noexcept {
    return m_isAny;
}

Result<ItemPredicate> ItemPredicate::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return ItemPredicate{};
    }

    std::optional<ResourceLocation> item;
    std::optional<i32> count;
    IntBounds durability;
    std::optional<ResourceLocation> potion;

    if (json.contains("item")) {
        item = ResourceLocation(json["item"].get<std::string>());
    }

    if (json.contains("count")) {
        if (json["count"].is_number()) {
            count = json["count"].get<i32>();
        } else {
            durability = IntBounds::fromJson(json["count"]);
        }
    }

    if (json.contains("durability")) {
        durability = IntBounds::fromJson(json["durability"]);
    }

    if (json.contains("potion")) {
        potion = ResourceLocation(json["potion"].get<std::string>());
    }

    // TODO: 解析 nbt, enchantments, stored_enchantments 等

    return ItemPredicate(std::move(item), count, std::move(durability), std::move(potion), nullptr);
}

nlohmann::json ItemPredicate::toJson() const {
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_item.has_value()) {
        json["item"] = m_item.value().toString();
    }
    if (m_count.has_value()) {
        json["count"] = m_count.value();
    }
    if (!m_durability.isUnbounded()) {
        json["durability"] = m_durability.toJson();
    }
    if (m_potion.has_value()) {
        json["potion"] = m_potion.value().toString();
    }
    return json;
}

} // namespace mc::advancement
