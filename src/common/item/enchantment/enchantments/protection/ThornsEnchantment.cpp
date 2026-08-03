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

#include "ThornsEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

bool ThornsEnchantment::shouldTrigger(i32 level, math::Random& random)
{
    if (level <= 0) {
        return false;
    }
    // 每级 15% 概率触发
    return random.nextFloat() < static_cast<f32>(level) * 0.15f;
}

i32 ThornsEnchantment::getThornsDamage(i32 level, math::Random& random)
{
    // 等级 > 10 时返回 level - 10，否则返回 1-4
    if (level > 10) {
        return level - 10;
    }
    return 1 + random.nextInt(4);
}

void ThornsEnchantment::onUserHurt(LivingEntity& user, Entity& attacker, i32 level) const
{
    if (level <= 0 || &user == &attacker) {
        return;
    }

    // 使用用户实体的随机数生成器
    math::Random rng(static_cast<u64>(user.id()) ^ static_cast<u64>(user.ticksExisted()));

    // 检查是否触发荆棘效果
    if (!shouldTrigger(level, rng)) {
        return;
    }

    // 对攻击者造成荆棘伤害（仅对生物实体有效）
    LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(&attacker);
    if (livingAttacker != nullptr) {
        // 创建荆棘伤害来源
        auto damageSource = DamageSources::thorns(&user);
        i32 thornsDamage = getThornsDamage(level, rng);
        livingAttacker->hurt(damageSource, static_cast<f32>(thornsDamage));
    }

    // 注意：荆棘会消耗装备耐久度
    // 这部分逻辑需要在调用方处理，因为需要访问装备槽位
}

} // namespace enchant
} // namespace item
} // namespace mc
