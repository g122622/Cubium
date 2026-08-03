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

#include "AllEnchantments.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/enchantment/enchantments/FortuneEnchantment.hpp"
#include "common/item/enchantment/enchantments/SilkTouchEnchantment.hpp"
#include "common/item/enchantment/enchantments/bow/FlameEnchantment.hpp"
#include "common/item/enchantment/enchantments/bow/InfinityEnchantment.hpp"
#include "common/item/enchantment/enchantments/bow/PowerEnchantment.hpp"
#include "common/item/enchantment/enchantments/bow/PunchEnchantment.hpp"
#include "common/item/enchantment/enchantments/crossbow/MultishotEnchantment.hpp"
#include "common/item/enchantment/enchantments/crossbow/PiercingEnchantment.hpp"
#include "common/item/enchantment/enchantments/crossbow/QuickChargeEnchantment.hpp"
#include "common/item/enchantment/enchantments/fishing/LuckOfTheSeaEnchantment.hpp"
#include "common/item/enchantment/enchantments/fishing/LureEnchantment.hpp"
#include "common/item/enchantment/enchantments/mace/BreachEnchantment.hpp"
#include "common/item/enchantment/enchantments/mace/DensityEnchantment.hpp"
#include "common/item/enchantment/enchantments/mace/WindBurstEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/AllProtectionEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/AquaAffinityEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/BlastProtectionEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/DepthStriderEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/FeatherFallingEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/FireProtectionEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/FrostWalkerEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/ProjectileProtectionEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/RespirationEnchantment.hpp"
#include "common/item/enchantment/enchantments/protection/ThornsEnchantment.hpp"
#include "common/item/enchantment/enchantments/special/BindingCurseEnchantment.hpp"
#include "common/item/enchantment/enchantments/special/MendingEnchantment.hpp"
#include "common/item/enchantment/enchantments/special/SoulSpeedEnchantment.hpp"
#include "common/item/enchantment/enchantments/special/VanishingCurseEnchantment.hpp"
#include "common/item/enchantment/enchantments/tool/EfficiencyEnchantment.hpp"
#include "common/item/enchantment/enchantments/tool/UnbreakingEnchantment.hpp"
#include "common/item/enchantment/enchantments/trident/ChannelingEnchantment.hpp"
#include "common/item/enchantment/enchantments/trident/ImpalingEnchantment.hpp"
#include "common/item/enchantment/enchantments/trident/LoyaltyEnchantment.hpp"
#include "common/item/enchantment/enchantments/trident/RiptideEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/BaneOfArthropodsEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/FireAspectEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/KnockbackEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/LootingEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/SharpnessEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/SmiteEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/SweepingEnchantment.hpp"
#include <memory>

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

// ========== 重锤附魔静态实例 ==========
DensityEnchantment AllEnchantments::DENSITY;
BreachEnchantment AllEnchantments::BREACH;
WindBurstEnchantment AllEnchantments::WIND_BURST;

void AllEnchantments::registerAll()
{
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

    // 重锤
    EnchantmentRegistry::registerEnchantment(std::make_unique<DensityEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<BreachEnchantment>());
    EnchantmentRegistry::registerEnchantment(std::make_unique<WindBurstEnchantment>());
}

} // namespace enchant
} // namespace item
} // namespace mc
