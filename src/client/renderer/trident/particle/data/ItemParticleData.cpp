#include "ItemParticleData.hpp"
#include "../ParticleRegistry.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::data {

ItemParticleData::ItemParticleData(ParticleTypeId type, const ItemStack& itemStack)
    : m_type(type)
    , m_itemStack(itemStack)
{
    MC_ASSERT_MSG(requiresItemData(type), "ItemParticleData requires an item-type particle");
}

std::string ItemParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::string ItemParticleData::getParameters() const
{
    // 物品粒子参数格式: item_id
    // 例如: minecraft:diamond
    if (m_itemStack.isEmpty()) {
        return "minecraft:air";
    }

    const Item* item = m_itemStack.getItem();
    if (item == nullptr) {
        return "minecraft:air";
    }

    return item->itemLocation().toString();
}

std::unique_ptr<ParticleData> ItemParticleData::clone() const
{
    return std::make_unique<ItemParticleData>(m_type, m_itemStack);
}

} // namespace mc::client::renderer::trident::particle::data
