#pragma once

/**
 * @file AllEnchantments.hpp
 * @brief 所有附魔的统一包含文件
 *
 * 此文件包含所有已实现的附魔类。
 * 参考 MC 1.16.5 共 34 种附魔。
 */

// 保护类附魔
#include "protection/ProtectionEnchantment.hpp"
#include "protection/AllProtectionEnchantment.hpp"
#include "protection/FireProtectionEnchantment.hpp"
#include "protection/FeatherFallingEnchantment.hpp"
#include "protection/BlastProtectionEnchantment.hpp"
#include "protection/ProjectileProtectionEnchantment.hpp"
#include "protection/ThornsEnchantment.hpp"
#include "protection/RespirationEnchantment.hpp"
#include "protection/AquaAffinityEnchantment.hpp"
#include "protection/DepthStriderEnchantment.hpp"
#include "protection/FrostWalkerEnchantment.hpp"

// 武器类附魔
#include "weapon/DamageEnchantment.hpp"
#include "weapon/SharpnessEnchantment.hpp"
#include "weapon/SmiteEnchantment.hpp"
#include "weapon/BaneOfArthropodsEnchantment.hpp"
#include "weapon/KnockbackEnchantment.hpp"
#include "weapon/FireAspectEnchantment.hpp"
#include "weapon/LootingEnchantment.hpp"
#include "weapon/SweepingEnchantment.hpp"

// 工具类附魔
#include "tool/EfficiencyEnchantment.hpp"
#include "tool/UnbreakingEnchantment.hpp"
#include "../FortuneEnchantment.hpp"
#include "../SilkTouchEnchantment.hpp"

// 弓类附魔
#include "bow/PowerEnchantment.hpp"
#include "bow/PunchEnchantment.hpp"
#include "bow/FlameEnchantment.hpp"
#include "bow/InfinityEnchantment.hpp"

// 钓鱼类附魔
#include "fishing/LuckOfTheSeaEnchantment.hpp"
#include "fishing/LureEnchantment.hpp"

// 三叉戟附魔
#include "trident/LoyaltyEnchantment.hpp"
#include "trident/ImpalingEnchantment.hpp"
#include "trident/RiptideEnchantment.hpp"
#include "trident/ChannelingEnchantment.hpp"

// 弩类附魔
#include "crossbow/MultishotEnchantment.hpp"
#include "crossbow/QuickChargeEnchantment.hpp"
#include "crossbow/PiercingEnchantment.hpp"

// 特殊附魔
#include "special/MendingEnchantment.hpp"
#include "special/VanishingCurseEnchantment.hpp"
#include "special/BindingCurseEnchantment.hpp"
#include "special/SoulSpeedEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔注册辅助类
 *
 * 提供便捷的附魔注册方法。
 */
class AllEnchantments {
public:
    /**
     * @brief 注册所有附魔到注册表
     *
     * 此方法应在游戏启动时调用。
     */
    static void registerAll();

    // ========== 保护类附魔 ==========
    static AllProtectionEnchantment PROTECTION;
    static FireProtectionEnchantment FIRE_PROTECTION;
    static FeatherFallingEnchantment FEATHER_FALLING;
    static BlastProtectionEnchantment BLAST_PROTECTION;
    static ProjectileProtectionEnchantment PROJECTILE_PROTECTION;
    static ThornsEnchantment THORNS;
    static RespirationEnchantment RESPIRATION;
    static AquaAffinityEnchantment AQUA_AFFINITY;
    static DepthStriderEnchantment DEPTH_STRIDER;
    static FrostWalkerEnchantment FROST_WALKER;

    // ========== 武器类附魔 ==========
    static SharpnessEnchantment SHARPNESS;
    static SmiteEnchantment SMITE;
    static BaneOfArthropodsEnchantment BANE_OF_ARTHROPODS;
    static KnockbackEnchantment KNOCKBACK;
    static FireAspectEnchantment FIRE_ASPECT;
    static LootingEnchantment LOOTING;
    static SweepingEnchantment SWEEPING;

    // ========== 工具类附魔 ==========
    static EfficiencyEnchantment EFFICIENCY;
    static UnbreakingEnchantment UNBREAKING;
    static FortuneEnchantment FORTUNE;
    static SilkTouchEnchantment SILK_TOUCH;

    // ========== 弓类附魔 ==========
    static PowerEnchantment POWER;
    static PunchEnchantment PUNCH;
    static FlameEnchantment FLAME;
    static InfinityEnchantment INFINITY;

    // ========== 钓鱼类附魔 ==========
    static LuckOfTheSeaEnchantment LUCK_OF_THE_SEA;
    static LureEnchantment LURE;

    // ========== 三叉戟附魔 ==========
    static LoyaltyEnchantment LOYALTY;
    static ImpalingEnchantment IMPALING;
    static RiptideEnchantment RIPTIDE;
    static ChannelingEnchantment CHANNELING;

    // ========== 弩类附魔 ==========
    static MultishotEnchantment MULTISHOT;
    static QuickChargeEnchantment QUICK_CHARGE;
    static PiercingEnchantment PIERCING;

    // ========== 特殊附魔 ==========
    static MendingEnchantment MENDING;
    static VanishingCurseEnchantment VANISHING_CURSE;
    static BindingCurseEnchantment BINDING_CURSE;
    static SoulSpeedEnchantment SOUL_SPEED;
};

} // namespace enchant
} // namespace item
} // namespace mc
