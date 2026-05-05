#include "entity/inventory/IRecipeHolder.hpp"
#include "item/crafting/IRecipe.hpp"

// Player 和 ServerPlayer 的实际使用在 TODO 中，待配方系统完善后实现

namespace mc {

void IRecipeHolder::onCrafting(Player& player) {
    // MC 1.16.5: 如果配方不是动态的，解锁配方并清除
    const crafting::IRecipe<IInventory>* recipe = getRecipeUsed();
    if (recipe != nullptr && !recipe->isDynamic()) {
        // 解锁配方（需要 ServerPlayer 实现）
        // player.unlockRecipes({recipe});
        setRecipeUsed(nullptr);
    }
    (void)player; // 避免未使用警告
}

bool IRecipeHolder::canUseRecipe(IWorld& world, ServerPlayer& player, const crafting::IRecipe<IInventory>* recipe) {
    // MC 1.16.5: 如果开启了有限合成且配方未解锁，返回false
    // 否则设置当前配方并返回true
    if (recipe != nullptr && !recipe->isDynamic()) {
        // TODO: 检查有限合成规则和配方解锁状态
        // if (world.getGameRules().getBoolean(GameRules::DO_LIMITED_CRAFTING)
        //     && !player.getRecipeBook().isUnlocked(recipe)) {
        //     return false;
        // }
    }
    setRecipeUsed(recipe);
    (void)world;
    (void)player;
    return true;
}

} // namespace mc
