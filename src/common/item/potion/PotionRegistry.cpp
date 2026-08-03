/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction without limitation the rights
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

#include "PotionRegistry.hpp"
#include "PotionType.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace potion {

// ========== 静态成员初始化 ==========

const Potion* PotionRegistry::EMPTY = nullptr;
const Potion* PotionRegistry::WATER = nullptr;
const Potion* PotionRegistry::MUNDANE = nullptr;
const Potion* PotionRegistry::THICK = nullptr;
const Potion* PotionRegistry::AWKWARD = nullptr;

const Potion* PotionRegistry::NIGHT_VISION = nullptr;
const Potion* PotionRegistry::LONG_NIGHT_VISION = nullptr;

const Potion* PotionRegistry::INVISIBILITY = nullptr;
const Potion* PotionRegistry::LONG_INVISIBILITY = nullptr;

const Potion* PotionRegistry::LEAPING = nullptr;
const Potion* PotionRegistry::LONG_LEAPING = nullptr;
const Potion* PotionRegistry::STRONG_LEAPING = nullptr;

const Potion* PotionRegistry::FIRE_RESISTANCE = nullptr;
const Potion* PotionRegistry::LONG_FIRE_RESISTANCE = nullptr;

const Potion* PotionRegistry::SWIFTNESS = nullptr;
const Potion* PotionRegistry::LONG_SWIFTNESS = nullptr;
const Potion* PotionRegistry::STRONG_SWIFTNESS = nullptr;

const Potion* PotionRegistry::SLOWNESS = nullptr;
const Potion* PotionRegistry::LONG_SLOWNESS = nullptr;
const Potion* PotionRegistry::STRONG_SLOWNESS = nullptr;

const Potion* PotionRegistry::TURTLE_MASTER = nullptr;
const Potion* PotionRegistry::LONG_TURTLE_MASTER = nullptr;
const Potion* PotionRegistry::STRONG_TURTLE_MASTER = nullptr;

const Potion* PotionRegistry::WATER_BREATHING = nullptr;
const Potion* PotionRegistry::LONG_WATER_BREATHING = nullptr;

const Potion* PotionRegistry::HEALING = nullptr;
const Potion* PotionRegistry::STRONG_HEALING = nullptr;

const Potion* PotionRegistry::HARMING = nullptr;
const Potion* PotionRegistry::STRONG_HARMING = nullptr;

const Potion* PotionRegistry::POISON = nullptr;
const Potion* PotionRegistry::LONG_POISON = nullptr;
const Potion* PotionRegistry::STRONG_POISON = nullptr;

const Potion* PotionRegistry::REGENERATION = nullptr;
const Potion* PotionRegistry::LONG_REGENERATION = nullptr;
const Potion* PotionRegistry::STRONG_REGENERATION = nullptr;

const Potion* PotionRegistry::STRENGTH = nullptr;
const Potion* PotionRegistry::LONG_STRENGTH = nullptr;
const Potion* PotionRegistry::STRONG_STRENGTH = nullptr;

const Potion* PotionRegistry::WEAKNESS = nullptr;
const Potion* PotionRegistry::LONG_WEAKNESS = nullptr;

const Potion* PotionRegistry::LUCK = nullptr;

const Potion* PotionRegistry::SLOW_FALLING = nullptr;
const Potion* PotionRegistry::LONG_SLOW_FALLING = nullptr;

// ========== PotionRegistry 实现 ==========

PotionRegistry& PotionRegistry::instance() noexcept
{
    static PotionRegistry registry;
    return registry;
}

const Potion* PotionRegistry::registerPotion(const ResourceLocation& id, Potion potion)
{
    // 设置药水的ID
    potion.setId(id);

    // 使用 make_unique 创建稳定的指针
    auto ptr = std::make_unique<Potion>(std::move(potion));
    const Potion* rawPtr = ptr.get();

    m_potions.push_back(std::move(ptr));
    m_idToPotion[id] = rawPtr;

    return rawPtr;
}

const Potion* PotionRegistry::getPotion(const ResourceLocation& id) const
{
    auto it = m_idToPotion.find(id);
    if (it == m_idToPotion.end()) {
        return nullptr;
    }
    return it->second;
}

const Potion* PotionRegistry::getPotion(PotionId id) const
{
    auto it = m_enumToPotion.find(id);
    if (it == m_enumToPotion.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::pair<ResourceLocation, const Potion*>> PotionRegistry::getAllPotions() const
{
    std::vector<std::pair<ResourceLocation, const Potion*>> result;
    result.reserve(m_potions.size());
    for (const auto& ptr : m_potions) {
        result.emplace_back(ptr->id(), ptr.get());
    }
    return result;
}

} // namespace potion
} // namespace mc
