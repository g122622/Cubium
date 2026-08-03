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

#include "PiglinAi.hpp"

#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gamerule/GameRules.hpp"

namespace mc {
namespace entity {

void PiglinAi::angerNearbyPiglins(IWorld& world, Player& player, bool requireLineOfSight)
{
    // 仅在服务端执行
    if (world.isClientSide()) {
        return;
    }

    // 查找玩家周围16格范围内的所有猪灵
    Vector3 playerPos = player.position();
    auto piglins = EntityUtils::findEntities<PiglinEntity>(
        &world, playerPos, PLAYER_ANGER_RANGE, nullptr, [&player, requireLineOfSight](PiglinEntity* piglin) {
            // 只激怒空闲状态的猪灵（非愤怒状态）
            if (piglin->isAngry()) {
                return false;
            }

            // 如果需要视线检查，则只有能看到玩家的猪灵才会被激怒
            if (requireLineOfSight && !piglin->canSee(player)) {
                return false;
            }

            return true;
        });

    // 对每个符合条件的猪灵设置愤怒目标
    for (auto* piglin : piglins) {
        if (world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::UNIVERSAL_ANGER)) {
            // 全局愤怒模式下，猪灵攻击最近的可见玩家
            Player* nearestPlayer = EntityUtils::findClosestEntity<Player>(
                &world, piglin->position(), PLAYER_ANGER_RANGE, nullptr, [](Player* p) { return p->isAlive(); });
            if (nearestPlayer != nullptr) {
                piglin->setAttackTarget(nearestPlayer);
            } else {
                // 如果找不到最近的可见玩家，则攻击触发者
                piglin->setAttackTarget(&player);
            }
        } else {
            // 非全局愤怒模式，直接攻击触发者
            piglin->setAttackTarget(&player);
        }

        piglin->setAngry(true);
        piglin->setAngerTime(ANGER_DURATION);
    }
}

bool PiglinAi::isWearingGold(const Player& player)
{
    // 检查玩家是否穿戴了金盔甲中的任意一件
    return player.isWearingGoldArmor();
}

} // namespace entity
} // namespace mc
