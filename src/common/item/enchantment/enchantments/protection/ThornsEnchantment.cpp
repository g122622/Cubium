#include "ThornsEnchantment.hpp"
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
    // MC 1.16.5: 每级 15% 概率触发
    // shouldHit(int level, Random rnd) { return level > 0 && rnd.nextFloat() < 0.15F * level; }
    return random.nextFloat() < static_cast<f32>(level) * 0.15f;
}

i32 ThornsEnchantment::getThornsDamage(i32 level, math::Random& random)
{
    // MC 1.16.5: getDamage(int level, Random rnd)
    // return level > 10 ? level - 10 : 1 + rnd.nextInt(4);
    // 等级 > 10 时返回 level - 10，否则返回 1-4
    if (level > 10) {
        return level - 10;
    }
    return 1 + random.nextInt(4);
}

void ThornsEnchantment::onUserHurt(LivingEntity& user, Entity& attacker, i32 level) const
{
    // MC 1.16.5: ThornsEnchantment.onUserHurt()
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
        // MC 1.16.5: DamageSource.causeThornsDamage(user)
        auto damageSource = DamageSources::thorns(&user);
        i32 thornsDamage = getThornsDamage(level, rng);
        livingAttacker->hurt(damageSource, static_cast<f32>(thornsDamage));
    }

    // 注意：MC 1.16.5 中荆棘会消耗装备耐久度
    // 这部分逻辑需要在调用方处理，因为需要访问装备槽位
}

} // namespace enchant
} // namespace item
} // namespace mc
