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

#include "CriterionTriggers.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "impl/AvoidVibrationTrigger.hpp"
#include "impl/BlockTriggers.hpp"
#include "impl/ChanneledLightningTrigger.hpp"
#include "impl/EffectTriggers.hpp"
#include "impl/EntityTriggers.hpp"
#include "impl/ImpossibleTrigger.hpp"
#include "impl/InventoryChangedTrigger.hpp"
#include "impl/ItemTriggers.hpp"
#include "impl/LocationTrigger.hpp"
#include "impl/PlayerKilledEntityTrigger.hpp"
#include "impl/TickTrigger.hpp"
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::advancement {

CriterionTriggers& CriterionTriggers::instance() noexcept
{
    static CriterionTriggers instance;
    return instance;
}

void CriterionTriggers::registerTrigger(std::unique_ptr<ICriterionTriggerBase> trigger)
{
    if (!trigger) {
        return;
    }

    ResourceLocation id = trigger->getId();
    if (m_triggers.find(id) != m_triggers.end()) {
        spdlog::warn("Trigger already registered, replacing: {}", id.toString());
    }

    m_triggers[id] = std::move(trigger);
}

ICriterionTriggerBase* CriterionTriggers::getTrigger(const ResourceLocation& id)
{
    auto it = m_triggers.find(id);
    return it != m_triggers.end() ? it->second.get() : nullptr;
}

bool CriterionTriggers::hasTrigger(const ResourceLocation& id) const noexcept
{
    return m_triggers.find(id) != m_triggers.end();
}

std::vector<ResourceLocation> CriterionTriggers::getAllTriggerIds() const noexcept
{
    std::vector<ResourceLocation> ids;
    ids.reserve(m_triggers.size());
    for (const auto& [id, _] : m_triggers) {
        ids.push_back(id);
    }
    return ids;
}

void CriterionTriggers::clear() noexcept
{
    m_triggers.clear();
}

void CriterionTriggers::registerBuiltinTriggers()
{
    // 注册基础触发器
    registerTrigger(std::make_unique<ImpossibleTrigger>());
    registerTrigger(std::make_unique<InventoryChangedTrigger>());
    registerTrigger(std::make_unique<TickTrigger>());
    registerTrigger(std::make_unique<RecipeUnlockedTrigger>());
    registerTrigger(std::make_unique<EffectsChangedTrigger>());
    registerTrigger(std::make_unique<BrewedPotionTrigger>());

    // 注册实体相关触发器
    registerTrigger(std::make_unique<TameAnimalTrigger>());
    registerTrigger(std::make_unique<BredAnimalsTrigger>());
    registerTrigger(std::make_unique<SummonedEntityTrigger>());
    registerTrigger(std::make_unique<PlayerKilledEntityTrigger>());
    registerTrigger(std::make_unique<EntityKilledPlayerTrigger>());
    registerTrigger(std::make_unique<CuredZombieVillagerTrigger>());
    registerTrigger(std::make_unique<PlayerInteractedWithEntityTrigger>());
    registerTrigger(std::make_unique<ChanneledLightningTrigger>());
    registerTrigger(std::make_unique<VillagerTradeTrigger>());

    // 注册方块相关触发器
    registerTrigger(std::make_unique<PlacedBlockTrigger>());
    registerTrigger(std::make_unique<EnterBlockTrigger>());
    registerTrigger(std::make_unique<SlideDownBlockTrigger>());
    registerTrigger(std::make_unique<BeeNestDestroyedTrigger>());

    // 注册位置相关触发器
    registerTrigger(std::make_unique<LocationTrigger>());
    registerTrigger(std::make_unique<SleptInBedTrigger>());
    registerTrigger(std::make_unique<HeroOfTheVillageTrigger>());
    registerTrigger(std::make_unique<VoluntaryExileTrigger>());

    // 注册物品相关触发器
    registerTrigger(std::make_unique<ConsumeItemTrigger>());
    registerTrigger(std::make_unique<ItemDurabilityTrigger>());
    registerTrigger(std::make_unique<EnchantedItemTrigger>());
    registerTrigger(std::make_unique<FilledBucketTrigger>());

    // 注册振动相关触发器
    registerTrigger(std::make_unique<AvoidVibrationTrigger>());

    spdlog::info("Registered {} builtin triggers", m_triggers.size());
}

} // namespace mc::advancement
