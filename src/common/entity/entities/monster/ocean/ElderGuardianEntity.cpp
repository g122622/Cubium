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

#include "ElderGuardianEntity.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "../../../entities/player/Player.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {

ElderGuardianEntity::ElderGuardianEntity(EntityInstanceId id)
    : GuardianEntity(id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ElderGuardianEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ElderGuardianEntity>(EntityInstanceId(0));
}

void ElderGuardianEntity::tick()
{
    GuardianEntity::tick();

    // 更新挖掘疲劳
    m_fatigueTimer++;
    if (m_fatigueTimer >= FATIGUE_INTERVAL) {
        m_fatigueTimer = 0;
        // 给附近的玩家挖掘疲劳效果
        if (m_world != nullptr) {
            std::vector<Entity*> nearbyEntities = m_world->getEntitiesInRange(m_position, MINING_FATIGUE_RANGE);
            for (Entity* entity : nearbyEntities) {
                // 只对玩家生效
                if (auto* player = dynamic_cast<Player*>(entity)) {
                    // Mining Fatigue III，持续 6000 tick (5分钟)
                    // amplifier = 2 表示等级 III (0=I, 1=II, 2=III)
                    entity::effect::EffectInstance fatigue(entity::effect::EffectType::MiningFatigue,
                        6000,  // 持续时间: 5分钟
                        2,     // 等级 III
                        false, // 非环境效果（显示粒子）
                        true,  // 显示粒子
                        true   // 显示图标
                    );
                    player->addEffect(std::move(fatigue));
                }
            }
        }
    }
}

void ElderGuardianEntity::registerAttributes()
{
    // 调用父类方法
    GuardianEntity::registerAttributes();

    // 远古守卫者的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 80.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

} // namespace mc
