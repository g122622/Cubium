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

#include "entity/inventory/IRecipeHolder.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/crafting/IRecipe.hpp"

namespace mc {

void IRecipeHolder::onCrafting(Player& player)
{
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

bool IRecipeHolder::canUseRecipe(IWorld& world, ServerPlayer& player, const crafting::IRecipe<IInventory>* recipe)
{
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
