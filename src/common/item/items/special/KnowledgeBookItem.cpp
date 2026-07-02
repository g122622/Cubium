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

#include "KnowledgeBookItem.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/world/IWorld.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace mc {
namespace item::items {

KnowledgeBookItem::KnowledgeBookItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ItemActionResult KnowledgeBookItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 仅在服务端执行
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    ItemStack& heldStack = player.getHeldItem(hand);

    // 从物品NBT中读取配方列表
    // NBT格式: {"recipes": ["minecraft:recovery_compass", "minecraft:stone_sword", ...]}
    const nlohmann::json* tag = heldStack.getTag();
    if (tag == nullptr) {
        return ItemActionResult::fail(heldStack);
    }

    auto it = tag->find("recipes");
    if (it == tag->end() || !it->is_array()) {
        return ItemActionResult::fail(heldStack);
    }

    const auto& recipesArray = *it;
    if (recipesArray.empty()) {
        return ItemActionResult::fail(heldStack);
    }

    // 解析配方ID列表
    std::vector<ResourceLocation> recipes;
    recipes.reserve(recipesArray.size());

    for (const auto& elem : recipesArray) {
        if (!elem.is_string()) {
            continue;
        }
        std::string recipeStr = elem.get<std::string>();
        ResourceLocation recipeId(recipeStr);

        // 验证配方是否存在
        if (!crafting::RecipeManager::instance().hasRecipe(recipeId)) {
            spdlog::warn("KnowledgeBookItem: invalid recipe '{}' in knowledge book, skipping", recipeStr);
            continue;
        }

        recipes.push_back(std::move(recipeId));
    }

    if (recipes.empty()) {
        return ItemActionResult::fail(heldStack);
    }

    // 消耗一个物品（创造模式不消耗）
    if (!player.isCreative()) {
        heldStack.shrink(1);
    }

    // TODO: 需要通过 ServerPlayer::unlockRecipes() 解锁配方给玩家。
    // 当前 IWorld 和 Player 接口不直接暴露配方解锁功能，
    // 需要添加 Player::unlockRecipes() 虚方法并在 ServerPlayer 中重写，
    // 或通过其他服务端机制触发。
    // 配方列表已解析并验证，此处仅消耗物品。

    return ItemActionResult::success(heldStack);
}

} // namespace item::items
} // namespace mc
