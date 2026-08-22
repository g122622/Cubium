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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "AxolotlGoals.hpp"

#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/water/AxolotlEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {

// ============================================================================
// AxolotlPlayDeadGoal
// ============================================================================

AxolotlPlayDeadGoal::AxolotlPlayDeadGoal(AxolotlEntity* axolotl)
    : Goal()
    , m_axolotl(axolotl)
{
    // 装死时不能移动和看向
    setMutexFlags({GoalFlag::Move, GoalFlag::Look});
}

bool AxolotlPlayDeadGoal::shouldExecute()
{
    // 仅在装死状态激活时执行
    return m_axolotl != nullptr && m_axolotl->isPlayingDead() && m_axolotl->isInWater();
}

bool AxolotlPlayDeadGoal::shouldContinueExecuting()
{
    return m_axolotl != nullptr && m_axolotl->isPlayingDead() && m_axolotl->isInWater();
}

void AxolotlPlayDeadGoal::startExecuting()
{
    // 装死开始 - 清除导航路径
    auto* nav = m_axolotl->navigator();
    if (nav != nullptr) {
        nav->clearPath();
    }

    // 给予自身 Regeneration I 效果（200 tick = 10 秒）
    m_axolotl->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Regeneration, 200));
}

void AxolotlPlayDeadGoal::resetTask()
{
    // 装死结束 - 状态已在 AxolotlEntity::tick() 中更新
}

void AxolotlPlayDeadGoal::tick()
{
    // 装死期间不移动、不看向目标
    // 实体保持静止状态
}

// ============================================================================
// AxolotlTargetGoal
// ============================================================================

AxolotlTargetGoal::AxolotlTargetGoal(AxolotlEntity* axolotl)
    : NearestAttackableTargetGoal<LivingEntity>(axolotl,
          true, // checkSight
          10,   // chance - 每10tick检查一次
          // 目标筛选谓词
          [axolotl](const LivingEntity* target) -> bool {
              if (target == nullptr) {
                  return false;
              }

              auto typeId = target->entityType();

              // 始终攻击的敌对目标：溺尸、守卫者、远古守卫者
              if (typeId == entity::VanillaEntityTypeKeys::DROWNED ||
                  typeId == entity::VanillaEntityTypeKeys::GUARDIAN ||
                  typeId == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN) {
                  return true;
              }

              // 狩猎目标（无冷却时）：对齐 Java AxolotlAttackablesSensor + EntityTypeTags.AXOLOTL_HUNT_TARGETS
              //   标签 = {cod, glow_squid, pufferfish, salmon, squid, tadpole, tropical_fish}（7 个）。
              //   vanilla instanceof/标签涵盖 GlowSquid；Cubium 扁平枚举须显式列举。
              //   TODO: 蝌蚪(tadpole)实体未实现（VanillaEntityTypeKeys 未注册 TADPOLE），待蝌蚪实现后补 TADPOLE。
              if (axolotl != nullptr && !axolotl->hasHuntingCooldown()) {
                  if (typeId == entity::VanillaEntityTypeKeys::TROPICAL_FISH ||
                      typeId == entity::VanillaEntityTypeKeys::PUFFERFISH ||
                      typeId == entity::VanillaEntityTypeKeys::SALMON || typeId == entity::VanillaEntityTypeKeys::COD ||
                      typeId == entity::VanillaEntityTypeKeys::SQUID ||
                      typeId == entity::VanillaEntityTypeKeys::GLOW_SQUID) {
                      return true;
                  }
              }

              return false;
          })
    , m_axolotl(axolotl)
{}

bool AxolotlTargetGoal::shouldExecute()
{
    // 装死时不选择攻击目标
    if (m_axolotl != nullptr && m_axolotl->isPlayingDead()) {
        return false;
    }
    return NearestAttackableTargetGoal<LivingEntity>::shouldExecute();
}

} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
