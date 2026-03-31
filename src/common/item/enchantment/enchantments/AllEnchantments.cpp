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
InfinityEnchantment AllEnchantments::INFINITY_ARROW;

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
    // 保护类
    EnchantmentRegistry::registerEnchantment(PROTECTION);
    EnchantmentRegistry::registerEnchantment(FIRE_PROTECTION);
    EnchantmentRegistry::registerEnchantment(FEATHER_FALLING);
    EnchantmentRegistry::registerEnchantment(BLAST_PROTECTION);
    EnchantmentRegistry::registerEnchantment(PROJECTILE_PROTECTION);
    EnchantmentRegistry::registerEnchantment(THORNS);
    EnchantmentRegistry::registerEnchantment(RESPIRATION);
    EnchantmentRegistry::registerEnchantment(AQUA_AFFINITY);
    EnchantmentRegistry::registerEnchantment(DEPTH_STRIDER);
    EnchantmentRegistry::registerEnchantment(FROST_WALKER);

    // 武器类
    EnchantmentRegistry::registerEnchantment(SHARPNESS);
    EnchantmentRegistry::registerEnchantment(SMITE);
    EnchantmentRegistry::registerEnchantment(BANE_OF_ARTHROPODS);
    EnchantmentRegistry::registerEnchantment(KNOCKBACK);
    EnchantmentRegistry::registerEnchantment(FIRE_ASPECT);
    EnchantmentRegistry::registerEnchantment(LOOTING);
    EnchantmentRegistry::registerEnchantment(SWEEPING);

    // 工具类
    EnchantmentRegistry::registerEnchantment(EFFICIENCY);
    EnchantmentRegistry::registerEnchantment(UNBREAKING);
    EnchantmentRegistry::registerEnchantment(FORTUNE);
    EnchantmentRegistry::registerEnchantment(SILK_TOUCH);

    // 弓类
    EnchantmentRegistry::registerEnchantment(POWER);
    EnchantmentRegistry::registerEnchantment(PUNCH);
    EnchantmentRegistry::registerEnchantment(FLAME);
    EnchantmentRegistry::registerEnchantment(INFINITY_ARROW);

    // 钓鱼类
    EnchantmentRegistry::registerEnchantment(LUCK_OF_THE_SEA);
    EnchantmentRegistry::registerEnchantment(LURE);

    // 三叉戟
    EnchantmentRegistry::registerEnchantment(LOYALTY);
    EnchantmentRegistry::registerEnchantment(IMPALING);
    EnchantmentRegistry::registerEnchantment(RIPTIDE);
    EnchantmentRegistry::registerEnchantment(CHANNELING);

    // 弩类
    EnchantmentRegistry::registerEnchantment(MULTISHOT);
    EnchantmentRegistry::registerEnchantment(QUICK_CHARGE);
    EnchantmentRegistry::registerEnchantment(PIERCING);

    // 特殊
    EnchantmentRegistry::registerEnchantment(MENDING);
    EnchantmentRegistry::registerEnchantment(VANISHING_CURSE);
    EnchantmentRegistry::registerEnchantment(BINDING_CURSE);
    EnchantmentRegistry::registerEnchantment(SOUL_SPEED);
}

} // namespace enchant
} // namespace item
} // namespace mc
