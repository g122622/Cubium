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

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item::items {

KnowledgeBookItem::KnowledgeBookItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ItemActionResult KnowledgeBookItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 仅在服务端执行配方解锁逻辑
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

    // 解析并验证配方ID列表
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

    // 通过 Player::unlockRecipe 虚方法解锁配方
    // ServerPlayer 重写了此方法以实际解锁配方并触发成就
    // 客户端 Player 的默认实现为空操作
    for (const auto& recipeId : recipes) {
        player.unlockRecipe(recipeId);
    }

    return ItemActionResult::success(heldStack);
}

} // namespace item::items
} // namespace mc
