#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "Potion.hpp"
#include <unordered_map>
#include <vector>

namespace mc {
namespace potion {

/**
 * @brief 药水注册表
 *
 * 管理所有药水类型的注册和查找。
 * 单例模式，通过 instance() 访问。
 *
 * 参考: net.minecraft.util.registry.Registry.POTION
 */
class PotionRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static PotionRegistry& instance();

    // ========== 注册 ==========

    /**
     * @brief 注册药水
     * @param id 资源位置（如 "minecraft:night_vision"）
     * @param potion 药水实例
     * @return 药水指针
     */
    [[nodiscard]] const Potion* registerPotion(const ResourceLocation& id, Potion potion);

    /**
     * @brief 通过ID获取药水
     * @param id 资源位置
     * @return 药水指针，未找到返回nullptr
     */
    [[nodiscard]] const Potion* getPotion(const ResourceLocation& id) const;

    /**
     * @brief 通过药水ID枚举获取药水
     * @param id 药水ID
     * @return 药水指针，未找到返回nullptr
     */
    [[nodiscard]] const Potion* getPotion(PotionId id) const;

    /**
     * @brief 获取所有注册的药水
     */
    [[nodiscard]] const std::vector<std::pair<ResourceLocation, Potion>>& getAllPotions() const { return m_potions; }

    /**
     * @brief 获取药水数量
     */
    [[nodiscard]] size_t size() const { return m_potions.size(); }

    // ========== 预定义药水指针 ==========

    // 基础药水
    static const Potion* EMPTY;
    static const Potion* WATER;
    static const Potion* MUNDANE;
    static const Potion* THICK;
    static const Potion* AWKWARD;

    // 夜视
    static const Potion* NIGHT_VISION;
    static const Potion* LONG_NIGHT_VISION;

    // 隐身
    static const Potion* INVISIBILITY;
    static const Potion* LONG_INVISIBILITY;

    // 跳跃提升
    static const Potion* LEAPING;
    static const Potion* LONG_LEAPING;
    static const Potion* STRONG_LEAPING;

    // 防火
    static const Potion* FIRE_RESISTANCE;
    static const Potion* LONG_FIRE_RESISTANCE;

    // 速度
    static const Potion* SWIFTNESS;
    static const Potion* LONG_SWIFTNESS;
    static const Potion* STRONG_SWIFTNESS;

    // 缓慢
    static const Potion* SLOWNESS;
    static const Potion* LONG_SLOWNESS;
    static const Potion* STRONG_SLOWNESS;

    // 海龟大师
    static const Potion* TURTLE_MASTER;
    static const Potion* LONG_TURTLE_MASTER;
    static const Potion* STRONG_TURTLE_MASTER;

    // 水下呼吸
    static const Potion* WATER_BREATHING;
    static const Potion* LONG_WATER_BREATHING;

    // 瞬间治疗
    static const Potion* HEALING;
    static const Potion* STRONG_HEALING;

    // 瞬间伤害
    static const Potion* HARMING;
    static const Potion* STRONG_HARMING;

    // 中毒
    static const Potion* POISON;
    static const Potion* LONG_POISON;
    static const Potion* STRONG_POISON;

    // 生命恢复
    static const Potion* REGENERATION;
    static const Potion* LONG_REGENERATION;
    static const Potion* STRONG_REGENERATION;

    // 力量
    static const Potion* STRENGTH;
    static const Potion* LONG_STRENGTH;
    static const Potion* STRONG_STRENGTH;

    // 虚弱
    static const Potion* WEAKNESS;
    static const Potion* LONG_WEAKNESS;

    // 幸运
    static const Potion* LUCK;

    // 缓降
    static const Potion* SLOW_FALLING;
    static const Potion* LONG_SLOW_FALLING;

private:
    PotionRegistry() = default;

    std::vector<std::pair<ResourceLocation, Potion>> m_potions;
    std::unordered_map<ResourceLocation, size_t> m_idToIndex;
    std::unordered_map<PotionId, size_t> m_enumToIndex;
};

} // namespace potion
} // namespace mc
