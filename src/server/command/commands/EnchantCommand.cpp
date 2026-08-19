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

#include "EnchantCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

void EnchantCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto enchantNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enchant");
    enchantNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(enchantNode,
        support::makeMetadata(
            "Adds an enchantment to a player's held item.", "/enchant <player> <enchantment> [<level>]", 2, {}, true));

    // /enchant <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());

    // /enchant <player> <enchantment>
    auto enchantmentNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "enchantment", StringArgumentType::word());
    enchantmentNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _enchantItem(ctx); });

    // /enchant <player> <enchantment> <level>
    auto levelNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "level", IntegerArgumentType::integer(0, 32767));
    levelNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _enchantItem(ctx); });

    enchantmentNode->addChild(levelNode);
    playerNode->addChild(enchantmentNode);
    enchantNode->addChild(playerNode);
    dispatcher.registerCommand(enchantNode);
}

i32 EnchantCommand::_enchantItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string& enchantmentName = context.getArgument<std::string>("enchantment");
    i32 level = 1;

    if (context.hasArgument("level")) {
        level = context.getArgument<i32>("level");
    }

    // 解析附魔名称（支持简写，自动添加 minecraft: 前缀）
    std::string enchantmentId = enchantmentName;
    if (enchantmentId.find(':') == std::string::npos) {
        enchantmentId = "minecraft:" + enchantmentName;
    }

    // 获取附魔实例
    const item::enchant::Enchantment* enchantment = item::enchant::EnchantmentRegistry::get(enchantmentId);
    if (enchantment == nullptr) {
        source.sendError("Unknown enchantment: " + enchantmentName);
        return 0;
    }

    // 检查等级是否有效
    if (level > enchantment->maxLevel()) {
        std::ostringstream ss;
        ss << "Level " << level << " is too high for " << enchantmentName << " (maximum is " << enchantment->maxLevel()
           << ")";
        source.sendError(ss.str());
        return 0;
    }

    auto* server = source.server();
    server::ServerWorld* world = source.world();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        // 经实体管理器解析实体（含 SimulatedPlayer 旁路）。此前用 playerManager.getPlayer 做前置守卫，
        // 对不进 PlayerManager 的 SimulatedPlayer 返 nullptr 跳过，致 /enchant 对其失效；playerData 后续
        // 未被使用，该守卫纯属冗余前置跳过，删除。
        Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            continue;
        }

        // 取主手物品的可变引用（getSelectedStackRef 返回 m_items[selectedSlot] 引用），
        // 后续 addEnchantment 原地修改权威槽。此前用 getSelectedStack()（按值返回副本）+ addEnchantment
        // 改副本，从未回写 player->inventory()，致附魔对全玩家（含真实玩家）静默失效——命令报
        // "Applied ..." 但主手物品无附魔。改用引用原地改修复此写回缺陷。
        ItemStack& heldItem = player->inventory().getSelectedStackRef();

        // 检查物品是否为空
        if (heldItem.isEmpty()) {
            if (playerIds.size() == 1) {
                source.sendError(player->username() + " has no item in their hand");
            }
            continue;
        }

        // 检查附魔是否可应用于该物品
        if (!enchantment->canApply(heldItem)) {
            if (playerIds.size() == 1) {
                source.sendError("The enchantment " + enchantmentName + " cannot be applied to this item");
            }
            continue;
        }

        // 检查与现有附魔的兼容性
        auto existingEnchantments = item::enchant::EnchantmentHelper::getEnchantments(heldItem);
        bool compatible = true;
        for (const auto& [existingEnch, existingLevel] : existingEnchantments) {
            if (existingEnch && !enchantment->isCompatibleWith(*existingEnch)) {
                compatible = false;
                break;
            }
        }

        if (!compatible) {
            if (playerIds.size() == 1) {
                source.sendError("The enchantment " + enchantmentName + " is incompatible with existing enchantments");
            }
            continue;
        }

        // 应用附魔：原地修改权威槽位（heldItem 是引用），立即生效。
        heldItem.addEnchantment(enchantmentId, level);
        successCount++;
    }

    // 发送成功消息
    if (successCount == 0) {
        source.sendError("Could not enchant any items");
        return 0;
    }

    // 获取附魔显示名称（带等级）
    std::string enchantDisplayName = enchantment->getNameKey(level);

    if (successCount == 1) {
        std::ostringstream ss;
        ss << "Applied " << enchantDisplayName << " to 1 player";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Applied " << enchantDisplayName << " to " << successCount << " players";
        source.sendMessage(ss.str());
    }

    return successCount;
}

} // namespace command
} // namespace mc
