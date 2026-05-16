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

#include "RavagerGoals.hpp"
#include "../../../../entities/monster/illager/RavagerEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockTags.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/gamerule/GameRules.hpp"
#include "../../../../../sound/SoundEvents.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"

namespace mc::entity::ai::goal {

// ============================================================================
// RavagerAttackGoal
// ============================================================================

RavagerAttackGoal::RavagerAttackGoal(RavagerEntity* ravager)
    : MeleeAttackGoal(ravager, 1.0, true)
    , m_ravager(ravager)
{
}

f32 RavagerAttackGoal::getAttackReachSqr(LivingEntity* target) const
{
    // MC 1.16.5: float f = this.mob.getWidth() - 0.1F;
    // return (double)(f * 2.0F * f * 2.0F + target.getWidth());
    if (!m_ravager || !target) return 0.0f;

    f32 f = m_ravager->width() - 0.1f;
    return f * 2.0f * f * 2.0f + target->width();
}

} // namespace mc::entity::ai::goal
