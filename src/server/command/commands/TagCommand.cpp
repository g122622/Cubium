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
 * The above copyright notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TagCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/EntityResolver.hpp"
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

void TagCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto tagNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tag");
    tagNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        tagNode, support::makeMetadata("Manages entity tags.", "/tag <targets> <add|remove|list> [tag]", 2, {}, true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());

    // /tag <targets> add <tag>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    // 原版 tag add 的 name 是单字串（不以引号开头）。
    auto tagArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::word());
    tagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addTag(ctx);
    });
    addNode->addChild(tagArg);
    targetsArg->addChild(addNode);

    // /tag <targets> remove <tag>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTagArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::word());
    removeTagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeTag(ctx);
    });
    // 原版 tag remove 的 name 带实体标签建议。
    removeTagArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    removeNode->addChild(removeTagArg);
    targetsArg->addChild(removeNode);

    // /tag <targets> list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listTags(ctx);
    });
    targetsArg->addChild(listNode);

    tagNode->addChild(targetsArg);
    dispatcher.registerCommand(tagNode);
}

/**
 * @brief 获取实体的显示名称。
 *
 * 优先使用玩家用户名，其次使用自定义名称，最后使用实体类型ID。
 */
[[nodiscard]] static std::string getEntityDisplayName(const Entity& entity)
{
    auto* player = dynamic_cast<const Player*>(&entity);
    if (player != nullptr) {
        return player->username();
    }
    if (entity.hasCustomName()) {
        return entity.customNameText();
    }
    return entity.getTypeId();
}

i32 TagCommand::_addTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("name");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    i32 successCount = 0;
    for (Entity* entity : entities) {
        if (entity->addTag(tag)) {
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.tag.add.failed"));
        return 0;
    }

    std::ostringstream ss;
    source.sendSuccess(
        ::mc::text::translatable("commands.tag.add.success.multiple", {tag, std::to_string(successCount)}), true);

    return successCount;
}

i32 TagCommand::_removeTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("name");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    i32 successCount = 0;
    for (Entity* entity : entities) {
        if (entity->removeTag(tag)) {
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.tag.remove.failed"));
        return 0;
    }

    std::ostringstream ss;
    source.sendSuccess(
        ::mc::text::translatable("commands.tag.remove.success.multiple", {tag, std::to_string(successCount)}), true);

    return successCount;
}

i32 TagCommand::_listTags(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    // 收集所有唯一的标签
    std::set<std::string> allTags;
    std::vector<std::pair<std::string, std::set<std::string>>> entityTags;

    for (Entity* entity : entities) {
        const auto& tags = entity->getTags();
        std::string entityName = getEntityDisplayName(*entity);

        entityTags.emplace_back(std::move(entityName), tags);
        for (const auto& tag : tags) {
            allTags.insert(tag);
        }
    }

    // 输出结果
    if (entities.size() == 1) {
        // 单个实体：显示该实体的所有标签
        if (entityTags.empty() || entityTags[0].second.empty()) {
            source.sendSuccess(::mc::text::translatable("commands.tag.list.single.empty",
                                   entityTags.empty() ? std::string("entity") : entityTags[0].first),
                false);
        } else {
            const auto& [name, tags] = entityTags[0];
            std::string tagsText;
            for (const auto& tag : tags) {
                if (!tagsText.empty()) {
                    tagsText += ' ';
                }
                tagsText += tag;
            }
            source.sendSuccess(::mc::text::translatable(
                                   "commands.tag.list.single.success", {name, std::to_string(tags.size()), tagsText}),
                false);
        }
    } else {
        // 多个实体：显示所有实体的标签总数
        if (allTags.empty()) {
            source.sendSuccess(
                ::mc::text::translatable("commands.tag.list.multiple.empty", std::to_string(entities.size())), false);
        } else {
            std::string tagsText;
            for (const auto& tag : allTags) {
                if (!tagsText.empty()) {
                    tagsText += ' ';
                }
                tagsText += tag;
            }
            source.sendSuccess(::mc::text::translatable("commands.tag.list.multiple.success",
                                   {std::to_string(entities.size()), std::to_string(allTags.size()), tagsText}),
                false);
        }
    }

    return static_cast<i32>(allTags.size());
}

} // namespace command
} // namespace mc
