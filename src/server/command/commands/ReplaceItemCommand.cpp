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

#include "ReplaceItemCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/command/support/ItemSlotAccess.hpp"
#include "server/world/ServerWorld.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

namespace {

/// 物品堆的显示名（原版 ItemStack.getDisplayName：方括号包裹的名称）。
std::unique_ptr<::mc::text::ITextComponent> _itemDisplayName(const ItemStack& stack)
{
    auto name = stack.isEmpty() ? ::mc::text::translatable("block.minecraft.air")
                                : ::mc::text::translatable(stack.getItem()->getTranslationKey());
    return ::mc::text::ComponentUtils::wrapInSquareBrackets(std::move(name));
}

/// 按命令参数构造物品堆；air 表示空堆（清空槽位）。
ItemStack _createStack(const ItemInput& input, i32 count)
{
    if (input.isAir()) {
        return ItemStack();
    }
    const Item* item = input.getItem();
    MC_ASSERT_RELEASE(item != nullptr);
    return ItemStack(item, std::min(count, item->maxStackSize()));
}

/// 读取数量参数；缺省 1。
i32 _requestedCount(CommandContext<ServerCommandSource>& context)
{
    return context.hasArgument("count") ? context.getArgument<i32>("count") : 1;
}

} // namespace

void ReplaceItemCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto replaceitemNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replaceitem");
    replaceitemNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(replaceitemNode,
        support::makeMetadata("Replaces items in an inventory.",
            "/replaceitem <entity|block> <target> <slot> <item> [count]",
            2,
            {},
            true));

    // /replaceitem entity <targets> <slot> <item> [count]
    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    auto slotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());
    auto itemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    auto countArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityItem(ctx);
    });
    itemArg->addChild(countArg);
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityItem(ctx);
    });
    slotArg->addChild(itemArg);
    targetsArg->addChild(slotArg);
    entityNode->addChild(targetsArg);
    replaceitemNode->addChild(entityNode);

    // /replaceitem block <pos> <slot> <item> [count]
    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto blockSlotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());
    auto blockItemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    auto blockCountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockItem(ctx);
    });
    blockItemArg->addChild(blockCountArg);
    blockItemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockItem(ctx);
    });
    blockSlotArg->addChild(blockItemArg);
    posArg->addChild(blockSlotArg);
    blockNode->addChild(posArg);
    replaceitemNode->addChild(blockNode);

    dispatcher.registerCommand(replaceitemNode);
}

i32 ReplaceItemCommand::_replaceEntityItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemStack stack = _createStack(context.getArgument<ItemInput>("item"), _requestedCount(context));

    std::vector<Entity*> targets = support::EntityResolver::resolve(source, selector);
    if (targets.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    std::vector<Entity*> changed;
    for (Entity* entity : targets) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        bool success = false;
        if (auto* player = dynamic_cast<Player*>(entity); player != nullptr) {
            success = support::setPlayerSlotItem(source, player->playerId(), slot, stack);
            if (success) {
                support::syncPlayerInventoryToClient(source, player->playerId());
            }
        } else {
            success = support::setNonPlayerEntitySlotItem(*entity, slot, stack);
        }

        if (success) {
            changed.push_back(entity);
        }
    }

    if (changed.empty()) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(_itemDisplayName(stack));
        params.push_back(::mc::text::literal(std::to_string(slot.slotIndex())));
        source.sendFailure(::mc::text::translatable("commands.item.target.no_changed.known_item", std::move(params)));
        return 0;
    }

    if (changed.size() == 1) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(changed.front()->getDisplayName());
        params.push_back(_itemDisplayName(stack));
        source.sendSuccess(
            ::mc::text::translatable("commands.item.entity.set.success.single", std::move(params)), true);
    } else {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(std::to_string(changed.size())));
        params.push_back(_itemDisplayName(stack));
        source.sendSuccess(
            ::mc::text::translatable("commands.item.entity.set.success.multiple", std::move(params)), true);
    }

    return static_cast<i32>(changed.size());
}

i32 ReplaceItemCommand::_replaceBlockItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const Vector3i blockPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemStack stack = _createStack(context.getArgument<ItemInput>("item"), _requestedCount(context));

    const BlockPos pos(blockPos.x, blockPos.y, blockPos.z);
    auto* world = source.world();
    MC_ASSERT_RELEASE(world != nullptr);

    auto* container = dynamic_cast<ContainerBlockEntity*>(world->getBlockEntity(pos));
    if (container == nullptr) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(std::to_string(pos.x)));
        params.push_back(::mc::text::literal(std::to_string(pos.y)));
        params.push_back(::mc::text::literal(std::to_string(pos.z)));
        source.sendFailure(::mc::text::translatable("commands.item.target.not_a_container", std::move(params)));
        return 0;
    }

    if (!support::setBlockContainerSlot(*world, pos, slot.slotIndex(), stack)) {
        source.sendFailure(
            ::mc::text::translatableEscape("commands.item.target.no_such_slot", std::to_string(slot.slotIndex())));
        return 0;
    }

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
    params.push_back(::mc::text::literal(std::to_string(pos.x)));
    params.push_back(::mc::text::literal(std::to_string(pos.y)));
    params.push_back(::mc::text::literal(std::to_string(pos.z)));
    params.push_back(_itemDisplayName(stack));
    source.sendSuccess(::mc::text::translatable("commands.item.block.set.success", std::move(params)), true);

    return 1;
}

} // namespace command
} // namespace mc
