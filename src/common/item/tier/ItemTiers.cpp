#include "ItemTiers.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../Items.hpp"
#include "../core/ItemRegistry.hpp"
#include "../crafting/Ingredient.hpp"

namespace mc {
namespace item {
namespace tier {

// ============================================================================
// 内部层级实现类
// ============================================================================

/**
 * @brief 具体层级实现
 *
 * 使用模板可以避免为每个层级写重复的类定义。
 */
class ItemTierImpl : public IItemTier {
public:
    ItemTierImpl(i32 maxUses,
        f32 efficiency,
        f32 attackDamage,
        i32 harvestLevel,
        i32 enchantability,
        const crafting::Ingredient& repairMaterial)
        : m_maxUses(maxUses)
        , m_efficiency(efficiency)
        , m_attackDamage(attackDamage)
        , m_harvestLevel(harvestLevel)
        , m_enchantability(enchantability)
        , m_repairMaterial(repairMaterial)
    {}

    [[nodiscard]] i32 getMaxUses() const override { return m_maxUses; }
    [[nodiscard]] f32 getEfficiency() const override { return m_efficiency; }
    [[nodiscard]] f32 getAttackDamage() const override { return m_attackDamage; }
    [[nodiscard]] i32 getHarvestLevel() const override { return m_harvestLevel; }
    [[nodiscard]] i32 getEnchantability() const override { return m_enchantability; }
    [[nodiscard]] const crafting::Ingredient& getRepairMaterial() const override { return m_repairMaterial; }

private:
    i32 m_maxUses;
    f32 m_efficiency;
    f32 m_attackDamage;
    i32 m_harvestLevel;
    i32 m_enchantability;
    const crafting::Ingredient& m_repairMaterial;
};

// ============================================================================
// 静态成员初始化
// ============================================================================

bool ItemTiers::s_initialized = false;
std::unique_ptr<IItemTier> ItemTiers::s_wood;
std::unique_ptr<IItemTier> ItemTiers::s_stone;
std::unique_ptr<IItemTier> ItemTiers::s_iron;
std::unique_ptr<IItemTier> ItemTiers::s_diamond;
std::unique_ptr<IItemTier> ItemTiers::s_gold;
std::unique_ptr<IItemTier> ItemTiers::s_netherite;

// ============================================================================
// 初始化
// ============================================================================

void ItemTiers::initialize()
{
    if (s_initialized) {
        return;
    }

    // 修复材料需要在Items初始化后可用
    // 木制工具 - 使用木板修复
    static crafting::Ingredient woodRepair = crafting::Ingredient::fromItems({Items::OAK_PLANKS,
        Items::SPRUCE_PLANKS,
        Items::BIRCH_PLANKS,
        Items::JUNGLE_PLANKS,
        Items::ACACIA_PLANKS,
        Items::DARK_OAK_PLANKS});
    s_wood = std::make_unique<ItemTierImpl>(59, // maxUses - 耐久度
        2.0f,                                   // efficiency - 挖掘效率
        0.0f,                                   // attackDamage - 攻击伤害加成
        0,                                      // harvestLevel - 挖掘等级
        15,                                     // enchantability - 附魔能力
        woodRepair);

    // 石制工具 - 使用圆石或黑石修复
    // MC 1.16.5 使用 stone_tool_materials 标签 (COBBLESTONE + BLACKSTONE)
    // 黑石在 BlockItemRegistry 中注册，通过 ItemRegistry 查找
    Item* blackstoneItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft:blackstone"));
    if (blackstoneItem) {
        static crafting::Ingredient stoneRepair = crafting::Ingredient::fromItems({Items::COBBLESTONE, blackstoneItem});
        s_stone = std::make_unique<ItemTierImpl>(131, // maxUses
            4.0f,                                     // efficiency
            1.0f,                                     // attackDamage
            1,                                        // harvestLevel
            5,                                        // enchantability
            stoneRepair);
    } else {
        // 回退：仅使用圆石
        static crafting::Ingredient stoneRepair = crafting::Ingredient::fromItems({Items::COBBLESTONE});
        s_stone = std::make_unique<ItemTierImpl>(131, // maxUses
            4.0f,                                     // efficiency
            1.0f,                                     // attackDamage
            1,                                        // harvestLevel
            5,                                        // enchantability
            stoneRepair);
    }

    // 铁制工具 - 使用铁锭修复
    static crafting::Ingredient ironRepair = crafting::Ingredient::fromItems({Items::IRON_INGOT});
    s_iron = std::make_unique<ItemTierImpl>(250, // maxUses
        6.0f,                                    // efficiency
        2.0f,                                    // attackDamage
        2,                                       // harvestLevel
        14,                                      // enchantability
        ironRepair);

    // 钻石工具 - 使用钻石修复
    static crafting::Ingredient diamondRepair = crafting::Ingredient::fromItems({Items::DIAMOND});
    s_diamond = std::make_unique<ItemTierImpl>(1561, // maxUses
        8.0f,                                        // efficiency
        3.0f,                                        // attackDamage
        3,                                           // harvestLevel
        10,                                          // enchantability
        diamondRepair);

    // 金制工具 - 使用金锭修复
    static crafting::Ingredient goldRepair = crafting::Ingredient::fromItems({Items::GOLD_INGOT});
    s_gold = std::make_unique<ItemTierImpl>(32, // maxUses - 极低耐久
        12.0f,                                  // efficiency - 最高效率
        0.0f,                                   // attackDamage
        0,                                      // harvestLevel - 与木相同
        22,                                     // enchantability - 最高附魔能力
        goldRepair);

    // 下界合金工具 - 使用下界合金锭修复
    static crafting::Ingredient netheriteRepair = crafting::Ingredient::fromItems({Items::NETHERITE_INGOT});
    s_netherite = std::make_unique<ItemTierImpl>(2031, // maxUses - 最高耐久
        9.0f,                                          // efficiency
        4.0f,                                          // attackDamage
        4,                                             // harvestLevel - 最高挖掘等级
        15,                                            // enchantability
        netheriteRepair);

    s_initialized = true;
}

} // namespace tier
} // namespace item
} // namespace mc
