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

#include "common/entity/serialization/components/PlayerComponentSerialization.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/PlayerScoreComponent.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc::entity::serialization::components {

// ============================================================================
// PlayerScoreComponent — Score（玩家分数，i32）
// ============================================================================

static void savePlayerScore(const Entity& entity, nbt::tags::compound_tag& tag)
{
    auto* player = dynamic_cast<const Player*>(&entity);
    if (player == nullptr) {
        return;
    }
    tag.put(nbt_keys::SCORE, player->getScore());
}

static Result<void> loadPlayerScore(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return {};
    }
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::SCORE)) {
        // setScore 同时写 PlayerScoreComponent 真相源 + DATA_PLAYER_SCORE_PARAM 镜像下发客户端。
        player->setScore(*val);
    }
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerPlayerComponentSerializers(ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<ecs::PlayerScoreComponent>(savePlayerScore, loadPlayerScore);
}

} // namespace mc::entity::serialization::components
