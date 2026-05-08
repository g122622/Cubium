#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../crafting/Ingredient.hpp"
#include "../../entity/core/LivingEntity.hpp"  // For EquipmentSlot enum
#include "../../sound/SoundEvent.hpp"
#include <string>

namespace mc {

// Forward declarations
namespace sound {
    class SoundEvent;
}

namespace item::armor {

/**
 * @brief 盔甲槽位枚举
 *
 * 定义盔甲的四个装备槽位。
 * 参考: net.minecraft.inventory.EquipmentSlotType
 */
enum class ArmorSlot : u8 {
    Head = 0,   ///< 头盔
    Chest = 1,  ///< 胸甲
    Legs = 2,   ///< 护腿
    Feet = 3    ///< 靴子
};

/**
 * @brief 盔甲材质接口
 *
 * 定义盔甲材质的属性，如耐久度、防御值、附魔能力等。
 * 参考: net.minecraft.item.IArmorMaterial
 *
 * 原版材质：LEATHER, CHAIN, IRON, GOLD, DIAMOND, TURTLE, NETHERITE
 */
class ArmorMaterial {
public:
    virtual ~ArmorMaterial() = default;

    // ========== 核心属性 ==========

    /**
     * @brief 获取材质名称
     * @return 材质ID（如"leather", "diamond"）
     */
    [[nodiscard]] virtual std::string getName() const = 0;

    /**
     * @brief 获取指定槽位的耐久度
     * @param slot 盔甲槽位
     * @return 耐久度
     *
     * 耐久度计算：baseDurability * slotMultiplier
     * slotMultiplier: 头盔=11, 胸甲=16, 护腿=15, 靴子=13
     */
    [[nodiscard]] virtual i32 getDurability(ArmorSlot slot) const = 0;

    /**
     * @brief 获取指定槽位的防御值
     * @param slot 盔甲槽位
     * @return 防御值（护甲值）
     */
    [[nodiscard]] virtual i32 getDefense(ArmorSlot slot) const = 0;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值
     *
     * 值越高，附魔时获得更好附魔的概率越高。
     * 金=25, 皮革/下界合金=15, 铁/海龟=9, 钻石=10, 锁链=12
     */
    [[nodiscard]] virtual i32 getEnchantability() const = 0;

    // ========== 音效 ==========

    /**
     * @brief 获取装备音效
     * @return 音效事件
     */
    [[nodiscard]] virtual sound::SoundEvent getEquipSound() const = 0;

    // ========== 修复 ==========

    /**
     * @brief 获取修复材料
     * @return 修复材料配方成分
     *
     * 皮革=皮革, 铁=铁锭, 金=金锭, 钻石=钻石, 下界合金=下界合金锭
     */
    [[nodiscard]] virtual crafting::Ingredient getRepairMaterial() const = 0;

    // ========== 高级属性 ==========

    /**
     * @brief 获取韧性
     * @return 韧性值
     *
     * 韧性减少高伤害攻击的护甲穿透。
     * 钻石=2.0, 下界合金=3.0, 其他=0.0
     */
    [[nodiscard]] virtual f32 getToughness() const { return 0.0f; }

    /**
     * @brief 获取击退抗性
     * @return 击退抗性 (0.0 - 1.0)
     *
     * 下界合金=0.1, 其他=0.0
     */
    [[nodiscard]] virtual f32 getKnockbackResistance() const { return 0.0f; }

    // ========== 工具方法 ==========

    /**
     * @brief 获取槽位对应的耐久度乘数
     * @param slot 盔甲槽位
     * @return 耐久度乘数
     */
    [[nodiscard]] static i32 getDurabilityMultiplier(ArmorSlot slot);

    /**
     * @brief 将ArmorSlot转换为EquipmentSlot
     * @param slot 盔甲槽位
     * @return 装备槽位索引 (0-3对应头、胸、腿、脚)
     */
    [[nodiscard]] static i32 toEquipmentSlotIndex(ArmorSlot slot);
};

// ============================================================================
// 原版盔甲材质
// ============================================================================

/**
 * @brief 皮革材质
 *
 * - 可染色
 * - 耐久度低
 * - 无特殊属性
 */
class LeatherArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "leather"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 15; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 锁链材质
 *
 * - 无法通过合成获得
 * - 中等属性
 */
class ChainArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "chain"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 12; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 铁材质
 *
 * - 平衡的属性
 * - 常见材质
 */
class IronArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "iron"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 9; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 金材质
 *
 * - 低耐久
 * - 高附魔能力
 * - 快速挖掘（盔甲不适用）
 */
class GoldArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "gold"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 25; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 钻石材质
 *
 * - 高耐久
 * - 高防御
 * - 韧性2.0
 */
class DiamondArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "diamond"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 10; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
    [[nodiscard]] f32 getToughness() const override { return 2.0f; }
};

/**
 * @brief 海龟材质
 *
 * - 仅头盔
 * - 水下呼吸效果
 */
class TurtleArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "turtle"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 9; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 下界合金材质
 *
 * - 最高耐久
 * - 最高防御
 * - 韧性3.0
 * - 击退抗性0.1
 * - 不被火烧毁
 */
class NetheriteArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "netherite"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 15; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
    [[nodiscard]] f32 getToughness() const override { return 3.0f; }
    [[nodiscard]] f32 getKnockbackResistance() const override { return 0.1f; }
};

// ============================================================================
// 材质访问器
// ============================================================================

namespace ArmorMaterials {
    extern const LeatherArmorMaterial LEATHER;
    extern const ChainArmorMaterial CHAIN;
    extern const IronArmorMaterial IRON;
    extern const GoldArmorMaterial GOLD;
    extern const DiamondArmorMaterial DIAMOND;
    extern const TurtleArmorMaterial TURTLE;
    extern const NetheriteArmorMaterial NETHERITE;

    /**
     * @brief 初始化所有材质
     */
    void initialize();
}

} // namespace item::armor
} // namespace mc
