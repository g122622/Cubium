#include "EnchantmentContainer.hpp"
#include "EnchantmentRegistry.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// EnchantmentInstance 实现
// ============================================================================

const Enchantment* EnchantmentInstance::getEnchantment() const
{
    return EnchantmentRegistry::get(enchantmentId);
}

// ============================================================================
// EnchantmentContainer 实现
// ============================================================================

i32 EnchantmentContainer::getLevel(const std::string& enchantmentId) const
{
    for (const auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            return instance.level;
        }
    }
    return 0;
}

bool EnchantmentContainer::has(const std::string& enchantmentId) const
{
    for (const auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            return true;
        }
    }
    return false;
}

bool EnchantmentContainer::hasType(EnchantmentType type) const
{
    for (const auto& instance : m_enchantments) {
        const Enchantment* enchantment = instance.getEnchantment();
        if (enchantment && enchantment->type() == type) {
            return true;
        }
    }
    return false;
}

void EnchantmentContainer::set(const std::string& enchantmentId, i32 level)
{
    // 查找现有附魔
    for (auto& instance : m_enchantments) {
        if (instance.enchantmentId == enchantmentId) {
            instance.level = level;
            return;
        }
    }

    // 添加新附魔
    m_enchantments.emplace_back(enchantmentId, level);
}

bool EnchantmentContainer::remove(const std::string& enchantmentId)
{
    for (auto it = m_enchantments.begin(); it != m_enchantments.end(); ++it) {
        if (it->enchantmentId == enchantmentId) {
            m_enchantments.erase(it);
            return true;
        }
    }
    return false;
}

bool EnchantmentContainer::canAdd(const std::string& enchantmentId) const
{
    const Enchantment* newEnchantment = EnchantmentRegistry::get(enchantmentId);
    if (!newEnchantment) {
        return false;
    }

    // 检查与现有附魔的兼容性
    for (const auto& instance : m_enchantments) {
        const Enchantment* existing = instance.getEnchantment();
        if (existing && !newEnchantment->isCompatibleWith(*existing)) {
            return false;
        }
    }

    return true;
}

void EnchantmentContainer::serialize(network::PacketSerializer& ser) const
{
    // 格式: VarInt count, 然后每个附魔: std::string id, VarInt level
    ser.writeVarInt(static_cast<i32>(m_enchantments.size()));
    for (const auto& instance : m_enchantments) {
        ser.writeString(instance.enchantmentId);
        ser.writeVarInt(instance.level);
    }
}

Result<EnchantmentContainer> EnchantmentContainer::deserialize(network::PacketDeserializer& deser)
{
    EnchantmentContainer container;

    auto countResult = deser.readVarInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 count = countResult.value();

    container.m_enchantments.reserve(static_cast<size_t>(count));
    for (i32 i = 0; i < count; ++i) {
        auto idResult = deser.readString();
        if (idResult.failed()) {
            return idResult.error();
        }

        auto levelResult = deser.readVarInt();
        if (levelResult.failed()) {
            return levelResult.error();
        }

        container.m_enchantments.emplace_back(idResult.value(), levelResult.value());
    }

    return container;
}

nlohmann::json EnchantmentContainer::toJson() const
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto& instance : m_enchantments) {
        nlohmann::json enchJson;
        enchJson["id"] = instance.enchantmentId;
        enchJson["lvl"] = instance.level;
        json.push_back(enchJson);
    }
    return json;
}

Result<EnchantmentContainer> EnchantmentContainer::fromJson(const nlohmann::json& json)
{
    EnchantmentContainer container;

    if (!json.is_array()) {
        return container;
    }

    for (const auto& enchJson : json) {
        if (!enchJson.is_object()) {
            continue;
        }
        if (!enchJson.contains("id") || !enchJson["id"].is_string()) {
            continue;
        }

        std::string id = enchJson["id"].get<std::string>();
        i32 level = 1;
        if (enchJson.contains("lvl") && enchJson["lvl"].is_number()) {
            level = enchJson["lvl"].get<i32>();
        }

        container.m_enchantments.emplace_back(id, level);
    }

    return container;
}

std::unique_ptr<nbt::tags::list_tag> EnchantmentContainer::toNbt() const
{
    auto list = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& instance : m_enchantments) {
        nbt::tags::compound_tag enchTag;
        enchTag.put("id", instance.enchantmentId);
        enchTag.put("lvl", static_cast<i16>(instance.level));
        list->value.push_back(std::move(enchTag));
    }
    return list;
}

EnchantmentContainer EnchantmentContainer::fromNbt(const nbt::tags::list_tag& list)
{
    EnchantmentContainer container;

    // 检查列表类型
    if (list.element_id() != nbt::TagId::Compound) {
        return container;
    }

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(list);
    for (const auto& enchTag : compoundList.value) {
        // 获取附魔ID
        auto it = enchTag.value.find("id");
        if (it == enchTag.value.end() || it->second->id() != nbt::TagId::String) {
            continue;
        }
        std::string id = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;

        // 获取附魔等级
        i32 level = 1;
        it = enchTag.value.find("lvl");
        if (it != enchTag.value.end()) {
            if (it->second->id() == nbt::TagId::Short) {
                level = dynamic_cast<const nbt::tags::short_tag&>(*it->second).value;
            } else if (it->second->id() == nbt::TagId::Int) {
                level = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
            } else if (it->second->id() == nbt::TagId::Byte) {
                level = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
            }
        }

        container.m_enchantments.emplace_back(id, level);
    }

    return container;
}

} // namespace enchant
} // namespace item
} // namespace mc
