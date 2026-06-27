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

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp" // For EquipmentSlot enum
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvent.hpp"
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
    Head = 0,  ///< 头盔
    Chest = 1, ///< 胸甲
    Legs = 2,  ///< 护腿
    Feet = 3   ///< 靴子
};

/**
 * @brief 盔甲材质接口
 *
 * 定义盔甲材质的属性，如耐久度、防御值、附魔能力等。
 * 参考: net.minecraft.item.IArmorMaterial
 *
 * 原版材质：LEATHER, COPPER, CHAIN, IRON, GOLD, DIAMOND, TURTLE, NETHERITE
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
     * @brief 获取材质的资源资产ID
     *
     * 用于构造装备纹理路径。与 getName() 不同，资产ID与 MC 1.21+ 的
     * equipment 纹理目录名称一致（如 "chainmail" 而非 "chain"，"turtle_scute" 而非 "turtle"）。
     *
     * 纹理路径格式：
     * - 头盔/胸甲/靴子: textures/entity/equipment/humanoid/<assetId>.png
     * - 护腿: textures/entity/equipment/humanoid_leggings/<assetId>.png
     * - 皮革盔甲覆盖层: textures/entity/equipment/<layerType>/leather_overlay.png
     *
     * @return 资产ID字符串
     */
    [[nodiscard]] virtual std::string getAssetId() const = 0;

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

    // ========== 纹理路径工具 ==========

    /**
     * @brief 根据材质资产ID和槽位构建盔甲纹理路径
     *
     * 使用 MC 1.21+ 的 equipment 纹理路径格式：
     * - 头盔/胸甲/靴子: textures/entity/equipment/humanoid/<assetId>.png
     * - 护腿: textures/entity/equipment/humanoid_leggings/<assetId>.png
     *
     * @param assetId 材质资产ID（来自 ArmorMaterial::getAssetId()）
     * @param slot 盔甲槽位
     * @return 纹理资源路径
     */
    [[nodiscard]] static ResourceLocation getArmorTexturePath(const std::string& assetId, ArmorSlot slot);

    /**
     * @brief 根据槽位构建皮革盔甲覆盖层纹理路径
     *
     * 皮革盔甲有两层纹理：底色层（可染色）和覆盖层（不可染色，显示细节图案）。
     * 覆盖层纹理路径格式：
     * - 头盔/胸甲/靴子: textures/entity/equipment/humanoid/leather_overlay.png
     * - 护腿: textures/entity/equipment/humanoid_leggings/leather_overlay.png
     *
     * @param slot 盔甲槽位
     * @return 覆盖层纹理资源路径
     */
    [[nodiscard]] static ResourceLocation getLeatherOverlayTexturePath(ArmorSlot slot);
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
    [[nodiscard]] std::string getAssetId() const override { return "leather"; }
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
    [[nodiscard]] std::string getAssetId() const override { return "chainmail"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 12; }
    [[nodiscard]] sound::SoundEvent getEquipSound() const override;
    [[nodiscard]] crafting::Ingredient getRepairMaterial() const override;
};

/**
 * @brief 铜材质
 *
 * MC 1.21.11 新增铜护甲材质。
 * - 耐久度介于皮革和锁链之间
 * - 防御值：头盔=2, 胸甲=4, 护腿=3, 靴子=1
 * - 附魔能力 8
 * - 无特殊属性（韧性=0, 击退抗性=0）
 */
class CopperArmorMaterial : public ArmorMaterial {
public:
    [[nodiscard]] std::string getName() const override { return "copper"; }
    [[nodiscard]] std::string getAssetId() const override { return "copper"; }
    [[nodiscard]] i32 getDurability(ArmorSlot slot) const override;
    [[nodiscard]] i32 getDefense(ArmorSlot slot) const override;
    [[nodiscard]] i32 getEnchantability() const override { return 8; }
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
    [[nodiscard]] std::string getAssetId() const override { return "iron"; }
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
    [[nodiscard]] std::string getAssetId() const override { return "gold"; }
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
    [[nodiscard]] std::string getAssetId() const override { return "diamond"; }
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
    [[nodiscard]] std::string getAssetId() const override { return "turtle_scute"; }
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
    [[nodiscard]] std::string getAssetId() const override { return "netherite"; }
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
extern const CopperArmorMaterial COPPER;
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
} // namespace ArmorMaterials

} // namespace item::armor
} // namespace mc
