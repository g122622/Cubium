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

#include "ItemCommand.hpp"

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
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/world/IWorld.hpp"
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
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::command {

namespace {

/// 按 `/item` 的 `with <item> [count]` 构造物品堆；air 表示空堆（清空槽位）。
ItemStack _createStack(const ItemInput& input, i32 count)
{
    if (input.isAir()) {
        return ItemStack();
    }
    const Item* item = input.getItem();
    MC_ASSERT_RELEASE(item != nullptr);
    return ItemStack(item, std::min(count, item->maxStackSize()));
}

/// 物品堆的显示名（原版 ItemStack.getDisplayName：方括号包裹的名称）。
std::unique_ptr<::mc::text::ITextComponent> _itemDisplayName(const ItemStack& stack)
{
    auto name = stack.isEmpty() ? ::mc::text::translatable("block.minecraft.air")
                                : ::mc::text::translatable(stack.getItem()->getTranslationKey());
    return ::mc::text::ComponentUtils::wrapInSquareBrackets(std::move(name));
}

/// 命令上下文里的数量参数；缺省 1。
i32 _requestedCount(CommandContext<ServerCommandSource>& context)
{
    return context.hasArgument("count") ? context.getArgument<i32>("count") : 1;
}

/**
 * @brief 写入方块容器槽位。
 *
 * @return 成功返回 1；失败（已下发错误消息）返回空值。
 */
std::optional<i32> _writeBlockTarget(
    ServerCommandSource& source, const Vector3i& rawPos, const ItemSlot& slot, const ItemStack& stack)
{
    const BlockPos pos(rawPos.x, rawPos.y, rawPos.z);
    auto* world = source.world();
    MC_ASSERT_RELEASE(world != nullptr);

    auto* container = dynamic_cast<ContainerBlockEntity*>(world->getBlockEntity(pos));
    if (container == nullptr) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(std::to_string(pos.x)));
        params.push_back(::mc::text::literal(std::to_string(pos.y)));
        params.push_back(::mc::text::literal(std::to_string(pos.z)));
        source.sendFailure(
            ::mc::text::translatable("commands.item.target.not_a_container", std::move(params)));
        return std::nullopt;
    }

    const i32 slotIndex = slot.slotIndex();
    if (!support::setBlockContainerSlot(*world, pos, slotIndex, stack)) {
        source.sendFailure(
            ::mc::text::translatableEscape("commands.item.target.no_such_slot", std::to_string(slotIndex)));
        return std::nullopt;
    }

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
    params.push_back(::mc::text::literal(std::to_string(pos.x)));
    params.push_back(::mc::text::literal(std::to_string(pos.y)));
    params.push_back(::mc::text::literal(std::to_string(pos.z)));
    params.push_back(_itemDisplayName(stack));
    source.sendSuccess(::mc::text::translatable("commands.item.block.set.success", std::move(params)), true);
    return 1;
}

/**
 * @brief 写入实体槽位集合。
 *
 * @return 成功返回被修改的实体数；失败（已下发错误消息）返回空值。
 */
std::optional<i32> _writeEntityTargets(ServerCommandSource& source,
    const EntitySelector& selector,
    const ItemSlot& slot,
    const ItemStack& stack)
{
    std::vector<Entity*> targets = support::EntityResolver::resolve(source, selector);
    if (targets.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return std::nullopt;
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
        return std::nullopt;
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

/// 读源方块容器槽位；失败时已下发错误消息。
std::optional<ItemStack> _readBlockSource(ServerCommandSource& source, const Vector3i& rawPos, const ItemSlot& slot)
{
    const BlockPos pos(rawPos.x, rawPos.y, rawPos.z);
    auto* world = source.world();
    MC_ASSERT_RELEASE(world != nullptr);

    auto* container = dynamic_cast<ContainerBlockEntity*>(world->getBlockEntity(pos));
    if (container == nullptr) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(std::to_string(pos.x)));
        params.push_back(::mc::text::literal(std::to_string(pos.y)));
        params.push_back(::mc::text::literal(std::to_string(pos.z)));
        source.sendFailure(
            ::mc::text::translatable("commands.item.source.not_a_container", std::move(params)));
        return std::nullopt;
    }

    ItemStack stack;
    if (!support::getBlockContainerSlot(*world, pos, slot.slotIndex(), stack)) {
        source.sendFailure(::mc::text::translatableEscape(
            "commands.item.source.no_such_slot", std::to_string(slot.slotIndex())));
        return std::nullopt;
    }
    return stack;
}

/// 读源实体槽位；失败时已下发错误消息。
std::optional<ItemStack> _readEntitySource(
    ServerCommandSource& source, const EntitySelector& selector, const ItemSlot& slot)
{
    Entity* entity = support::EntityResolver::resolveSingle(source, selector);
    if (entity == nullptr) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return std::nullopt;
    }

    ItemStack stack;
    bool success = false;
    if (auto* player = dynamic_cast<Player*>(entity); player != nullptr) {
        success = support::getPlayerSlotItem(source, player->playerId(), slot, stack);
    } else {
        success = support::getNonPlayerEntitySlotItem(*entity, slot, stack);
    }

    if (!success) {
        source.sendFailure(::mc::text::translatableEscape(
            "commands.item.source.no_such_slot", std::to_string(slot.slotIndex())));
        return std::nullopt;
    }
    return stack;
}

} // namespace

void ItemCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto itemNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("item");
    itemNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(itemNode,
        support::makeMetadata("Modifies the items of an inventory.",
            "/item replace <block|entity> ... (with <item> [<count>]|from <block|entity> ...)",
            2,
            {},
            true));

    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");

    // ========== item replace block <pos> <slot> ==========
    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto blockSlotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());

    auto blockWithNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("with");
    auto blockItemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    blockItemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockWith(ctx);
    });
    auto blockCountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockWith(ctx);
    });
    blockItemArg->addChild(blockCountArg);
    blockWithNode->addChild(blockItemArg);

    auto blockFromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
    auto blockFromBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto blockSourcePosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "source", BlockPosArgumentType::blockPos());
    auto blockSourceSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>(
        "sourceSlot", ItemSlotArgumentType::itemSlot());
    blockSourceSlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockFrom(ctx, true);
    });
    blockSourcePosArg->addChild(blockSourceSlotArg);
    blockFromBlockNode->addChild(blockSourcePosArg);

    auto blockFromEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto blockSourceEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "source", EntityArgumentType::entity());
    auto blockSourceEntitySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>(
        "sourceSlot", ItemSlotArgumentType::itemSlot());
    blockSourceEntitySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceBlockFrom(ctx, false);
    });
    blockSourceEntityArg->addChild(blockSourceEntitySlotArg);
    blockFromEntityNode->addChild(blockSourceEntityArg);

    blockFromNode->addChild(blockFromBlockNode);
    blockFromNode->addChild(blockFromEntityNode);

    blockSlotArg->addChild(blockWithNode);
    blockSlotArg->addChild(blockFromNode);
    posArg->addChild(blockSlotArg);
    blockNode->addChild(posArg);

    // ========== item replace entity <targets> <slot> ==========
    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    auto entitySlotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());

    auto entityWithNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("with");
    auto entityItemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    entityItemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityWith(ctx);
    });
    auto entityCountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    entityCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityWith(ctx);
    });
    entityItemArg->addChild(entityCountArg);
    entityWithNode->addChild(entityItemArg);

    auto entityFromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
    auto entityFromBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto entitySourcePosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "source", BlockPosArgumentType::blockPos());
    auto entitySourceSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>(
        "sourceSlot", ItemSlotArgumentType::itemSlot());
    entitySourceSlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityFrom(ctx, true);
    });
    entitySourcePosArg->addChild(entitySourceSlotArg);
    entityFromBlockNode->addChild(entitySourcePosArg);

    auto entityFromEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto entitySourceEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "source", EntityArgumentType::entity());
    auto entitySourceEntitySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>(
        "sourceSlot", ItemSlotArgumentType::itemSlot());
    entitySourceEntitySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _replaceEntityFrom(ctx, false);
    });
    entitySourceEntityArg->addChild(entitySourceEntitySlotArg);
    entityFromEntityNode->addChild(entitySourceEntityArg);

    entityFromNode->addChild(entityFromBlockNode);
    entityFromNode->addChild(entityFromEntityNode);

    entitySlotArg->addChild(entityWithNode);
    entitySlotArg->addChild(entityFromNode);
    targetsArg->addChild(entitySlotArg);
    entityNode->addChild(targetsArg);

    replaceNode->addChild(blockNode);
    replaceNode->addChild(entityNode);
    itemNode->addChild(replaceNode);

    // TODO: `/item modify <block|entity> ... <modifier>` 与 `from ... <modifier>` 依赖战利品函数
    // （LootItemFunction）注册表，该子系统尚未落地，这两条分支暂不注册。

    dispatcher.registerCommand(itemNode);
}

i32 ItemCommand::_replaceBlockWith(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemInput input = context.getArgument<ItemInput>("item");

    const auto written = _writeBlockTarget(source, pos, slot, _createStack(input, _requestedCount(context)));
    return written.value_or(0);
}

i32 ItemCommand::_replaceEntityWith(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemInput input = context.getArgument<ItemInput>("item");

    const auto written = _writeEntityTargets(source, selector, slot, _createStack(input, _requestedCount(context)));
    return written.value_or(0);
}

i32 ItemCommand::_replaceBlockFrom(CommandContext<ServerCommandSource>& context, bool fromBlockSource)
{
    auto& source = context.getSource();
    const Vector3i targetPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    const ItemSlot targetSlot = context.getArgument<ItemSlot>("slot");
    const ItemSlot sourceSlot = context.getArgument<ItemSlot>("sourceSlot");

    std::optional<ItemStack> sourceStack;
    if (fromBlockSource) {
        const Vector3i sourcePos = BlockPosArgumentType::getBlockPos(context, "source", source);
        sourceStack = _readBlockSource(source, sourcePos, sourceSlot);
    } else {
        sourceStack = _readEntitySource(source, context.getArgument<EntitySelector>("source"), sourceSlot);
    }
    if (!sourceStack.has_value()) {
        return 0;
    }

    const auto written = _writeBlockTarget(source, targetPos, targetSlot, *sourceStack);
    return written.value_or(0);
}

i32 ItemCommand::_replaceEntityFrom(CommandContext<ServerCommandSource>& context, bool fromBlockSource)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const ItemSlot targetSlot = context.getArgument<ItemSlot>("slot");
    const ItemSlot sourceSlot = context.getArgument<ItemSlot>("sourceSlot");

    std::optional<ItemStack> sourceStack;
    if (fromBlockSource) {
        const Vector3i sourcePos = BlockPosArgumentType::getBlockPos(context, "source", source);
        sourceStack = _readBlockSource(source, sourcePos, sourceSlot);
    } else {
        sourceStack = _readEntitySource(source, context.getArgument<EntitySelector>("source"), sourceSlot);
    }
    if (!sourceStack.has_value()) {
        return 0;
    }

    const auto written = _writeEntityTargets(source, selector, targetSlot, *sourceStack);
    return written.value_or(0);
}

} // namespace mc::command
