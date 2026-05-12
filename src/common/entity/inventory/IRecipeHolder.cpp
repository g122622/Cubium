#include "entity/inventory/IRecipeHolder.hpp"
#include "item/crafting/IRecipe.hpp"
#include "entity/entities/player/Player.hpp"

namespace mc {

void IRecipeHolder::onCrafting(Player& player) {
    // MC 1.16.5: 如果配方不是动态的，解锁配方并清除
    const crafting::IRecipe<IInventory>* recipe = getRecipeUsed();
    if (recipe != nullptr && !recipe->isDynamic()) {
        // 获取配方 ID
        ResourceLocation recipeId = recipe->getId();

        // 触发配方解锁成就（仅对 ServerPlayer 有效）
        // ServerPlayer::unlockRecipe 方法会更新配方书并触发成就
        player.unlockRecipe(recipeId);

        setRecipeUsed(nullptr);
    }
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
