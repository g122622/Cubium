#include "AllEnchantments.hpp"
#include "../EnchantmentRegistry.hpp"

namespace mc {
namespace item {
namespace enchant {

// ========== 保护类附魔静态实例 ==========
AllProtectionEnchantment AllEnchantments::PROTECTION;
FireProtectionEnchantment AllEnchantments::FIRE_PROTECTION;
FeatherFallingEnchantment AllEnchantments::FEATHER_FALLING;
BlastProtectionEnchantment AllEnchantments::BLAST_PROTECTION;
ProjectileProtectionEnchantment AllEnchantments::PROJECTILE_PROTECTION;
ThornsEnchantment AllEnchantments::THORNS;
RespirationEnchantment AllEnchantments::RESPIRATION;
AquaAffinityEnchantment AllEnchantments::AQUA_AFFINITY;
DepthStriderEnchantment AllEnchantments::DEPTH_STRIDER;
FrostWalkerEnchantment AllEnchantments::FROST_WALKER;

// ========== 武器类附魔静态实例 ==========
SharpnessEnchantment AllEnchantments::SHARPNESS;
SmiteEnchantment AllEnchantments::SMITE;
BaneOfArthropodsEnchantment AllEnchantments::BANE_OF_ARTHROPODS;
KnockbackEnchantment AllEnchantments::KNOCKBACK;
FireAspectEnchantment AllEnchantments::FIRE_ASPECT;
LootingEnchantment AllEnchantments::LOOTING;
SweepingEnchantment AllEnchantments::SWEEPING;

// ========== 工具类附魔静态实例 ==========
EfficiencyEnchantment AllEnchantments::EFFICIENCY;
UnbreakingEnchantment AllEnchantments::UNBREAKING;
FortuneEnchantment AllEnchantments::FORTUNE;
SilkTouchEnchantment AllEnchantments::SILK_TOUCH;

// ========== 弓类附魔静态实例 ==========
PowerEnchantment AllEnchantments::POWER;
PunchEnchantment AllEnchantments::PUNCH;
FlameEnchantment AllEnchantments::FLAME;
InfinityEnchantment AllEnchantments::INFINITY;

// ========== 钓鱼类附魔静态实例 ==========
LuckOfTheSeaEnchantment AllEnchantments::LUCK_OF_THE_SEA;
LureEnchantment AllEnchantments::LURE;

// ========== 三叉戟附魔静态实例 ==========
LoyaltyEnchantment AllEnchantments::LOYALTY;
ImpalingEnchantment AllEnchantments::IMPALING;
RiptideEnchantment AllEnchantments::RIPTIDE;
ChannelingEnchantment AllEnchantments::CHANNELING;

// ========== 弩类附魔静态实例 ==========
MultishotEnchantment AllEnchantments::MULTISHOT;
QuickChargeEnchantment AllEnchantments::QUICK_CHARGE;
PiercingEnchantment AllEnchantments::PIERCING;

// ========== 特殊附魔静态实例 ==========
MendingEnchantment AllEnchantments::MENDING;
VanishingCurseEnchantment AllEnchantments::VANISHING_CURSE;
BindingCurseEnchantment AllEnchantments::BINDING_CURSE;
SoulSpeedEnchantment AllEnchantments::SOUL_SPEED;

void AllEnchantments::registerAll() {
    auto& registry = EnchantmentRegistry::instance();

    // 保护类
    registry.registerEnchantment(PROTECTION);
    registry.registerEnchantment(FIRE_PROTECTION);
    registry.registerEnchantment(FEATHER_FALLING);
    registry.registerEnchantment(BLAST_PROTECTION);
    registry.registerEnchantment(PROJECTILE_PROTECTION);
    registry.registerEnchantment(THORNS);
    registry.registerEnchantment(RESPIRATION);
    registry.registerEnchantment(AQUA_AFFINITY);
    registry.registerEnchantment(DEPTH_STRIDER);
    registry.registerEnchantment(FROST_WALKER);

    // 武器类
    registry.registerEnchantment(SHARPNESS);
    registry.registerEnchantment(SMITE);
    registry.registerEnchantment(BANE_OF_ARTHROPODS);
    registry.registerEnchantment(KNOCKBACK);
    registry.registerEnchantment(FIRE_ASPECT);
    registry.registerEnchantment(LOOTING);
    registry.registerEnchantment(SWEEPING);

    // 工具类
    registry.registerEnchantment(EFFICIENCY);
    registry.registerEnchantment(UNBREAKING);
    registry.registerEnchantment(FORTUNE);
    registry.registerEnchantment(SILK_TOUCH);

    // 弓类
    registry.registerEnchantment(POWER);
    registry.registerEnchantment(PUNCH);
    registry.registerEnchantment(FLAME);
    registry.registerEnchantment(INFINITY);

    // 钓鱼类
    registry.registerEnchantment(LUCK_OF_THE_SEA);
    registry.registerEnchantment(LURE);

    // 三叉戟
    registry.registerEnchantment(LOYALTY);
    registry.registerEnchantment(IMPALING);
    registry.registerEnchantment(RIPTIDE);
    registry.registerEnchantment(CHANNELING);

    // 弩类
    registry.registerEnchantment(MULTISHOT);
    registry.registerEnchantment(QUICK_CHARGE);
    registry.registerEnchantment(PIERCING);

    // 特殊
    registry.registerEnchantment(MENDING);
    registry.registerEnchantment(VANISHING_CURSE);
    registry.registerEnchantment(BINDING_CURSE);
    registry.registerEnchantment(SOUL_SPEED);
}

} // namespace enchant
} // namespace item
} // namespace mc
