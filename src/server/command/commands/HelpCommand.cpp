#include "HelpCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <algorithm>
#include <sstream>

namespace mc {
namespace command {

namespace {

[[nodiscard]] std::vector<std::shared_ptr<LiteralCommandNode<ServerCommandSource>>> getVisibleRootCommands(
    CommandDispatcher<ServerCommandSource>& dispatcher,
    const ServerCommandSource& source
) {
    std::vector<std::shared_ptr<LiteralCommandNode<ServerCommandSource>>> commands;
    for (const auto& [name, child] : dispatcher.getRoot()->getChildren()) {
        (void)name;
        if (!child || child->getType() != NodeType::Literal || !child->canUse(source)) {
            continue;
        }

        commands.push_back(std::static_pointer_cast<LiteralCommandNode<ServerCommandSource>>(child));
    }

    std::sort(commands.begin(), commands.end(), [](const auto& left, const auto& right) {
        return left->getName() < right->getName();
    });
    return commands;
}

} // namespace

void HelpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto helpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("help");
    helpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);
    });
    support::applyMetadata(
        helpNode,
        support::makeMetadata(
            "Show command help.",
            "/help [command]",
            0));
    helpNode->setCommand([&dispatcher](CommandContext<ServerCommandSource>& ctx) {
        return showHelp(ctx, dispatcher);
    });

    auto commandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command",
        StringArgumentType::word());
    commandArg->setCommand([&dispatcher](CommandContext<ServerCommandSource>& ctx) {
        return showCommandHelp(ctx, dispatcher);
    });
    helpNode->addChild(commandArg);

    dispatcher.registerCommand(helpNode);
}

i32 HelpCommand::showHelp(CommandContext<ServerCommandSource>& context, CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto& source = context.getSource();
    const auto commands = getVisibleRootCommands(dispatcher, source);

    std::ostringstream ss;
    ss << "Available commands:\n";
    for (const auto& node : commands) {
        const auto& metadata = node->getMetadataInfo();
        ss << "  /" << node->getName();
        if (!metadata.description.empty()) {
            ss << " - " << metadata.description;
        }
        if (!metadata.implemented) {
            ss << " [partial]";
        }
        ss << "\n";
    }
    ss << "\nUse /help <command> for more information";
    source.sendMessage(ss.str());
    return 1;
}

i32 HelpCommand::showCommandHelp(CommandContext<ServerCommandSource>& context, CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto& source = context.getSource();
    const std::string commandName = context.getArgument<std::string>("command");
    auto node = dispatcher.getRoot()->getChild(commandName);
    if (!node || node->getType() != NodeType::Literal || !node->canUse(source)) {
        source.sendError("Unknown command: " + commandName);
        return 0;
    }

    const auto literalNode = std::static_pointer_cast<LiteralCommandNode<ServerCommandSource>>(node);
    const auto& metadata = literalNode->getMetadataInfo();

    std::ostringstream ss;
    ss << "/" << literalNode->getName();
    if (!metadata.description.empty()) {
        ss << " - " << metadata.description;
    }
    if (!metadata.usage.empty()) {
        ss << "\nUsage:\n" << metadata.usage;
    }
    if (!metadata.aliases.empty()) {
        ss << "\nAliases: ";
        for (size_t index = 0; index < metadata.aliases.size(); ++index) {
            if (index > 0) {
                ss << ", ";
            }
            ss << metadata.aliases[index];
        }
    }
    if (!metadata.implemented) {
        ss << "\nStatus: partially implemented";
    }

    source.sendMessage(ss.str());
    return 1;
}

} // namespace command
} // namespace mc
