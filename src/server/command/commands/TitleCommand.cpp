#include "TitleCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include <sstream>

namespace mc {
namespace command {

void TitleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto titleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    titleNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        titleNode,
        support::makeMetadata(
            "Controls screen title display.",
            "/title <player> (clear|reset|title|subtitle|actionbar|times) ...",
            2,
            {},
            true));

    // /title <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );

    // clear 子命令
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearTitle(ctx);
    });

    // reset 子命令
    auto resetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    resetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return resetTitle(ctx);
    });

    // title <json> 子命令
    auto titleTextNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    auto titleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "json",
        StringArgumentType::greedyString()
    );
    titleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setTitle(ctx);
    });
    titleTextNode->addChild(titleJsonNode);

    // subtitle <json> 子命令
    auto subtitleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("subtitle");
    auto subtitleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "json",
        StringArgumentType::greedyString()
    );
    subtitleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setSubtitle(ctx);
    });
    subtitleNode->addChild(subtitleJsonNode);

    // actionbar <json> 子命令
    auto actionbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("actionbar");
    auto actionbarJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "json",
        StringArgumentType::greedyString()
    );
    actionbarJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setActionbar(ctx);
    });
    actionbarNode->addChild(actionbarJsonNode);

    // times <fadeIn> <stay> <fadeOut> 子命令
    auto timesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("times");
    auto fadeInNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "fadeIn",
        IntegerArgumentType::integer(0)
    );
    auto stayNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "stay",
        IntegerArgumentType::integer(0)
    );
    auto fadeOutNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "fadeOut",
        IntegerArgumentType::integer(0)
    );
    fadeOutNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setTimes(ctx);
    });

    fadeInNode->addChild(stayNode);
    stayNode->addChild(fadeOutNode);
    timesNode->addChild(fadeInNode);

    playerNode->addChild(clearNode);
    playerNode->addChild(resetNode);
    playerNode->addChild(titleTextNode);
    playerNode->addChild(subtitleNode);
    playerNode->addChild(actionbarNode);
    playerNode->addChild(timesNode);
    titleNode->addChild(playerNode);

    dispatcher.registerCommand(titleNode);
}

i32 TitleCommand::clearTitle(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 清除标题
    return static_cast<i32>(playerIds.size());
}

i32 TitleCommand::resetTitle(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 重置标题
    return static_cast<i32>(playerIds.size());
}

i32 TitleCommand::setTitle(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 设置标题

    return static_cast<i32>(playerIds.size());
}

i32 TitleCommand::setSubtitle(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 设置副标题

    return static_cast<i32>(playerIds.size());
}

i32 TitleCommand::setActionbar(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 设置动作栏

    return static_cast<i32>(playerIds.size());
}

i32 TitleCommand::setTimes(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    // TODO: 发送 TitlePacket 设置时间

    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
