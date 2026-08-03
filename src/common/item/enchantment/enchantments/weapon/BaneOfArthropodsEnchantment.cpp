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

#include "BaneOfArthropodsEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

void BaneOfArthropodsEnchantment::onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const
{
    // 只有节肢杀手会应用缓慢效果
    if (level <= 0) {
        return;
    }

    // 检查目标是否为生物实体
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
    if (livingTarget == nullptr) {
        return;
    }

    // 检查目标是否为节肢生物
    if (livingTarget->getCreatureAttribute() != CreatureAttribute::Arthropod) {
        return;
    }

    // 计算缓慢效果持续时间
    math::Random rng(static_cast<u64>(user.id()) ^ static_cast<u64>(user.ticksExisted()));
    i32 duration = getSlownessDuration(level, rng);

    // 添加缓慢 IV 效果（amplifier = 3 = Slowness IV）
    livingTarget->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Slowness,
        duration,
        getSlownessAmplifier(), // amplifier = 3 = Slowness IV
        false,                  // 不作为环境效果
        true,                   // 显示粒子
        true                    // 显示图标
        ));
}

} // namespace enchant
} // namespace item
} // namespace mc
