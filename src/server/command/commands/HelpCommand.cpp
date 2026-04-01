#include "HelpCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include <sstream>

namespace mc {
namespace command {

// 命令帮助信息
static const struct {
    const char* name;
    const char* description;
    const char* usage;
} s_commandHelp[] = {
    {"gamemode", "Sets a player's game mode", "/gamemode <mode> [player]"},
    {"time", "Changes or queries the world's game time", "/time set <value>\n/time add <value>\n/time query <day|daytime|gametime>"},
    {"kill", "Kills entities (players, mobs, etc.)", "/kill [target]"},
    {"list", "Lists players on the server", "/list"},
    {"help", "Provides help for commands", "/help [command]"},
    {"seed", "Displays the world seed", "/seed"},
    {"tp", "Teleports entities", "/tp <target>\n/tp <x> <y> <z>"},
    {"give", "Gives items to a player", "/give <player> <item> [count]"},
    {"clear", "Clears items from player inventory", "/clear [player] [item] [maxCount]"},
    {"weather", "Sets or queries weather", "/weather clear [duration]\n/weather rain [duration]\n/weather thunder [duration]\n/weather query"},
    {"experience", "Modifies player experience", "/experience add <player> <amount> [points|levels]\n/experience set <player> <amount> [points|levels]\n/experience query <player> [points|levels]"},
    {"xp", "Alias of /experience", "/xp add <player> <amount> [points|levels]\n/xp set <player> <amount> [points|levels]\n/xp query <player> [points|levels]"},
};

void HelpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto helpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("help");
    helpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);  // 所有人可用
    });
    helpNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return showHelp(ctx);
    });

    // /help <command>
    auto commandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "command",
        StringArgumentType::word()
    );
    commandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return showCommandHelp(ctx);
    });
    helpNode->addChild(commandArg);

    dispatcher.registerCommand(helpNode);
}

i32 HelpCommand::showHelp(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    std::ostringstream ss;
    ss << "Available commands:\n";

    for (const auto& cmd : s_commandHelp) {
        ss << "  /" << cmd.name << " - " << cmd.description << "\n";
    }

    ss << "\nUse /help <command> for more information";
    source.sendMessage(ss.str());

    return 1;
}

i32 HelpCommand::showCommandHelp(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    String commandName = context.getArgument<String>("command");

    // 查找命令帮助
    for (const auto& cmd : s_commandHelp) {
        if (cmd.name == commandName) {
            std::ostringstream ss;
            ss << "/" << cmd.name << " - " << cmd.description << "\n";
            ss << "Usage:\n" << cmd.usage;
            source.sendMessage(ss.str());
            return 1;
        }
    }

    source.sendMessage("Unknown command: " + commandName);
    return 0;
}

} // namespace command
} // namespace mc
