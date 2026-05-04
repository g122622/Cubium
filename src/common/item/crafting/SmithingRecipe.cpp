#include "SmithingRecipe.hpp"

namespace mc {
namespace crafting {

const String SmithingRecipe::EMPTY_GROUP = "";

SmithingRecipe::SmithingRecipe(
    const ResourceLocation& id,
    const Ingredient& base,
    const Ingredient& addition,
    const ItemStack& result
)
    : m_id(id)
    , m_base(base)
    , m_addition(addition)
    , m_result(result) {
    // 缓存原料列表
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingRecipe::matches(const IInventory& inventory) const {
    // MC 原版：检查基础槽位和添加物槽位
    if (inventory.getContainerSize() < 2) {
        return false;
    }

    ItemStack baseStack = inventory.getItem(SLOT_BASE);
    ItemStack additionStack = inventory.getItem(SLOT_ADDITION);

    return m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingRecipe::assemble(const IInventory& inventory) const {
    // MC 1.16.5: 锻造结果复制基础物品的 NBT 数据（附魔、耐久、自定义名称等）
    // 参考: net.minecraft.item.crafting.SmithingRecipe.getCraftingResult()
    ItemStack resultStack = m_result.copy();

    // 获取基础物品（槽位0）
    if (inventory.getContainerSize() > SLOT_BASE) {
        const ItemStack& baseStack = inventory.getItem(SLOT_BASE);

        // 复制基础物品的NBT数据
        const nlohmann::json* baseTag = baseStack.getTag();
        if (baseTag != nullptr && !baseTag->is_null()) {
            // 使用 getOrCreateTag() 设置标签，然后复制数据
            nlohmann::json& resultTag = resultStack.getOrCreateTag();
            resultTag = *baseTag;  // JSON拷贝赋值会执行深拷贝
        }

        // 复制附魔数据（如果NBT中没有附魔数据，这里作为备份）
        if (baseStack.hasEnchantments() && !resultStack.hasEnchantments()) {
            resultStack.getEnchantmentsMutable() = baseStack.getEnchantments();
        }

        // 复制耐久度
        if (baseStack.isDamageable() && baseStack.getDamage() > 0) {
            resultStack.setDamage(baseStack.getDamage());
        }

        // 复制修复成本
        if (baseStack.getRepairCost() > 0) {
            resultStack.setRepairCost(baseStack.getRepairCost());
        }

        // 复制自定义名称
        if (baseStack.hasCustomName()) {
            const text::ITextComponent* customName = baseStack.getCustomNameComponent();
            if (customName != nullptr) {
                resultStack.setCustomNameComponent(customName->deepCopy());
            }
        }
    }

    return resultStack;
}

const std::vector<Ingredient>& SmithingRecipe::getIngredients() const {
    return m_ingredients;
}

std::vector<ItemStack> SmithingRecipe::getRemainingItems(const IInventory& inventory) const {
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
