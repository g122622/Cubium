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

#include "common/item/loot/conditions/EntityScoresCondition.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {
namespace loot {

namespace {

Entity* getConditionTargetEntity(LootContext& context, EntityPropertiesCondition::EntityTarget target)
{
    switch (target) {
        case EntityPropertiesCondition::EntityTarget::This:
            return context.get<Entity>(LootParams::THIS_ENTITY);
        case EntityPropertiesCondition::EntityTarget::Killer:
            return context.get<Entity>(LootParams::KILLER_ENTITY);
        case EntityPropertiesCondition::EntityTarget::DirectKiller:
            return context.get<Entity>(LootParams::DIRECT_KILLER);
        case EntityPropertiesCondition::EntityTarget::KillerPlayer: {
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            return static_cast<Entity*>(player);
        }
    }
    return nullptr;
}

std::string getScoreboardEntryName(const Entity& entity)
{
    if (const auto* player = dynamic_cast<const Player*>(&entity)) {
        return player->username();
    }
    return entity.uuid();
}

} // namespace

EntityScoresCondition::EntityScoresCondition(
    EntityPropertiesCondition::EntityTarget target, std::unordered_map<std::string, RandomValueRange> scores)
    : m_target(target)
    , m_scores(std::move(scores))
{}

bool EntityScoresCondition::test(LootContext& context) const
{
    Entity* entity = getConditionTargetEntity(context, m_target);
    if (!entity) {
        return false;
    }

    auto* player = dynamic_cast<Player*>(entity);
    if (player == nullptr) {
        return false;
    }

    auto* scoreboard = player->getScoreboard();
    if (scoreboard == nullptr) {
        return false;
    }

    const std::string entryName = getScoreboardEntryName(*entity);

    for (const auto& [objectiveName, range] : m_scores) {
        auto* objective = scoreboard->getObjective(objectiveName);
        if (objective == nullptr) {
            return false;
        }

        const auto* score = scoreboard->getScore(entryName, *objective);
        if (score == nullptr) {
            return false;
        }

        const f32 points = static_cast<f32>(score->getScorePoints());
        if (points < range.getMin() || points > range.getMax()) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<LootCondition> EntityScoresCondition::clone() const noexcept
{
    return std::make_unique<EntityScoresCondition>(m_target, m_scores);
}

} // namespace loot
} // namespace mc
