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
    EnchantmentRegistry::registerEnchantment(std::make_unique<AllProtectionEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FireProtectionEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FeatherFallingEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<BlastProtectionEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<ProjectileProtectionEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<ThornsEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<RespirationEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<AquaAffinityEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<DepthStriderEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FrostWalkerEnchantment>());

    // 武器类
    EnchantmentRegistry::registerEnchantment(std::make_unique<SharpnessEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<SmiteEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<BaneOfArthropodsEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<KnockbackEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FireAspectEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<LootingEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<SweepingEnchantment>());

    // 工具类
    EnchantmentRegistry::registerEnchantment(std::make_unique<EfficiencyEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<UnbreakingEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FortuneEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<SilkTouchEnchantment>());

    // 弓类
    EnchantmentRegistry::registerEnchantment(std::make_unique<PowerEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<PunchEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<FlameEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<InfinityEnchantment>());

    // 钓鱼类
    EnchantmentRegistry::registerEnchantment(std::make_unique<LuckOfTheSeaEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<LureEnchantment>());

    // 三叉戟
    EnchantmentRegistry::registerEnchantment(std::make_unique<LoyaltyEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<ImpalingEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<RiptideEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<ChannelingEnchantment>());

    // 弩类
    EnchantmentRegistry::registerEnchantment(std::make_unique<MultishotEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<QuickChargeEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<PiercingEnchantment>());

    // 特殊
    EnchantmentRegistry::registerEnchantment(std::make_unique<MendingEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<VanishingCurseEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<BindingCurseEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<SoulSpeedEnchantment>());
}

} // namespace enchant
} // namespace item
} // namespace mc
