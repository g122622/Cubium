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

#include "ItemParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

ItemParticleData::ItemParticleData(ParticleTypeId type, const ItemStack& itemStack)
    : m_type(type)
    , m_itemStack(itemStack)
{
    MC_ASSERT_RELEASE_MSG(requiresItemData(type), "ItemParticleData requires an item-type particle");
}

std::string ItemParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::string ItemParticleData::getParameters() const
{
    // 物品粒子参数格式: item_id
    if (m_itemStack.isEmpty()) {
        return "minecraft:air";
    }

    return m_itemStack.getItem()->itemLocation().toString();
}

std::unique_ptr<ParticleData> ItemParticleData::clone() const
{
    return std::make_unique<ItemParticleData>(m_type, m_itemStack);
}

} // namespace mc::client::renderer::trident::particle::data
