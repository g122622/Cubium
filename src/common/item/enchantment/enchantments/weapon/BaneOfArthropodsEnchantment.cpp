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
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace item {
namespace enchant {

void BaneOfArthropodsEnchantment::onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const
{
    // 仅节肢杀手施加缓慢副作用（锋利/亡灵杀手无副作用）。
    if (level <= 0) {
        return;
    }

    // 检查目标是否为生物实体
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
    if (livingTarget == nullptr) {
        return;
    }

    // 目标判定用 EntityTypeTags::SENSITIVE_TO_BANE_OF_ARTHROPODS 标签，与
    // DamageEnchantment::getDamageBonus 的判定保持一致。标签派生自 ARTHROPOD，覆盖全部节肢成员。
    // 此前此处用 getCreatureAttribute()==Arthropod 枚举判定，与 getDamageBonus 的标签判定不一致
    // （getDamageBonus 已迁移标签，本函数未跟上）。
    if (!EntityTypeTags::SENSITIVE_TO_BANE_OF_ARTHROPODS().contains(livingTarget->getTypeId())) {
        return;
    }

    // 缓慢持续时间：round( randomBetween(1.5, 1.5 + 0.5*(level-1)) * 20 ) tick。
    // minDur=1.5 固定，maxDur 随等级线性增长（每级 +0.5 秒）。此前用 20 + nextInt(10*level)
    // 的整数 tick 公式，范围与浮点公式不符。
    math::Random rng(static_cast<u64>(user.id()) ^ static_cast<u64>(user.ticksExisted()));
    i32 duration = getSlownessDuration(level, rng);

    // 添加缓慢 IV 效果（amplifier = 3 固定）
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
