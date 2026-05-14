#pragma once

#include "DamageEnchantment.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 节肢杀手附魔
 *
 * 增加对节肢生物的伤害。
 * 参考 MC 1.16.5 BaneOfArthropodsEnchantment
 *
 * 效果:
 * - 对节肢生物每级增加 2.5 点伤害
 * - 节肢生物包括：蜘蛛、洞穴蜘蛛、蠹虫、末影螨、蜜蜂等
 * - 被击中的节肢生物会获得缓慢 IV 效果 1-1.5 秒
 * - 最大 V 级
 * - 与锋利、亡灵杀手互斥
 */
class BaneOfArthropodsEnchantment : public DamageEnchantment {
public:
    BaneOfArthropodsEnchantment()
        : DamageEnchantment(Type::Arthropods)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:bane_of_arthropods"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.bane_of_arthropods";
    }

    [[nodiscard]] EnchantmentRarity rarity() const override { return EnchantmentRarity::Uncommon; }

    /**
     * @brief 获取缓慢效果的持续时间（tick）
     *
     * MC 1.16.5 公式: duration = 20 + random.nextInt(10 * level)
     * - 基础时间: 20 tick (1秒)
     * - 额外随机: 0 到 (10*level - 1) tick
     *
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return 持续时间（tick）
     */
    [[nodiscard]] static i32 getSlownessDuration(i32 level, math::Random& random)
    {
        // 20 + random.nextInt(10 * level)
        // Level I: 20-29 tick, Level V: 20-69 tick
        return 20 + random.nextInt(10 * level);
    }

    /**
     * @brief 获取缓慢效果等级（固定为 IV）
     * @return 缓慢效果等级 (3 = Slowness IV)
     */
    [[nodiscard]] static constexpr i32 getSlownessAmplifier()
    {
        return 3; // Slowness IV
    }

    /**
     * @brief 当攻击目标实体时调用
     *
     * 对节肢生物施加缓慢 IV 效果。
     * 参考 MC 1.16.5 DamageEnchantment.onEntityDamaged()
     *
     * @param user 攻击者
     * @param target 目标实体
     * @param level 附魔等级
     */
    void onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
