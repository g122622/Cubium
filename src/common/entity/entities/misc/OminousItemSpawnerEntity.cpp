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
 * copies of substantial portions of the Software.
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

#include "OminousItemSpawnerEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {

OminousItemSpawnerEntity::OminousItemSpawnerEntity(EntityId id)
    : Entity(id)
{
    // 设置随机延迟（构造时使用简单随机，后续tick中会重新随机化）
    m_spawnDelay = MIN_SPAWN_DELAY;
}

std::unique_ptr<Entity> OminousItemSpawnerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<OminousItemSpawnerEntity>(EntityId(0));
}

void OminousItemSpawnerEntity::tick()
{
    Entity::tick();

    if (m_hasSpawned) {
        // 投掷完成后移除自身
        remove();
        return;
    }

    if (m_spawnDelay > 0) {
        --m_spawnDelay;
        return;
    }

    // 延迟结束，投掷物品
    spawnItem();
    m_hasSpawned = true;
}

void OminousItemSpawnerEntity::setTargetPlayer(const std::string& playerUuid)
{
    m_targetPlayerUuid = playerUuid;
}

void OminousItemSpawnerEntity::setSpawnDelay(i32 delay)
{
    m_spawnDelay = std::max(MIN_SPAWN_DELAY, std::min(delay, MAX_SPAWN_DELAY));
}

void OminousItemSpawnerEntity::spawnItem()
{
    // TODO(trial_chambers): 实现从战利品表选取物品并投掷
    // 1. 从 "minecraft:spawners/trial_chamber/items_to_drop_when_ominous" 战利品表生成物品
    // 2. 创建 ItemEntity 并设置到目标位置
    // 3. 设置物品的投掷速度（向下，约0.2格/tick）
    // 4. 将物品实体添加到世界
    // 5. 播放音效和粒子效果
}

} // namespace entity
} // namespace mc
