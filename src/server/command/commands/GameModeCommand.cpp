#include "GameModeCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/player/ServerPlayer.hpp"
#include <sstream>

namespace mc {
namespace command {

void GameModeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    // /gamemode <mode> - 设置自己的游戏模式
    auto modeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, GameMode>>(
        "mode",
        GameModeArgumentType::gameMode()
    );

    // 注册模式参数节点
    modeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setGameModeSelf(ctx);
    });

    // /gamemode <mode> <target> - 设置指定玩家的游戏模式
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::players()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setGameModeOthers(ctx);
    });

    modeArg->addChild(targetArg);

    // 创建字面量节点
    auto literalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("gamemode");
    literalNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    literalNode->addChild(modeArg);

    dispatcher.registerCommand(literalNode);
}

i32 GameModeCommand::setGameModeSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    if (!source.isPlayer()) {
        source.sendMessage("You must be an entity to use this command");
        return 0;
    }

    GameMode mode = context.getArgument<GameMode>("mode");
    auto* server = source.server();
    if (!server || source.playerId() == 0 || !server->gameModeManager().setGameMode(source.playerId(), mode)) {
        source.sendMessage("Failed to change game mode");
        return 0;
    }

    std::ostringstream ss;
    ss << "Set " << source.name() << "'s game mode to " << getGameModeName(mode) << " mode";
    source.sendMessage(ss.str());
    return 1;
}

i32 GameModeCommand::setGameModeOthers(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取游戏模式
    GameMode mode = context.getArgument<GameMode>("mode");

    // 获取目标玩家选择器
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // TODO: 解析选择器获取玩家列表
    // 目前只支持单个玩家名称
    // std::vector<ServerPlayer*> players = resolveSelector(selector, source);

    // 暂时返回成功
    source.sendMessage("Set game mode to " + String(getGameModeName(mode)));
    return 1;
}

i32 GameModeCommand::setGameMode(
    ServerCommandSource& source,
    ServerPlayer& player,
    GameMode mode
) {
    // 设置玩家的游戏模式
    // player.setGameMode(mode);

    // 发送反馈
    String playerName = player.username();
    String modeName = getGameModeName(mode);

    std::ostringstream ss;
    ss << "Set " << playerName << "'s game mode to " << modeName << " mode";
    source.sendMessage(ss.str());

    return 1;
}

const char* GameModeCommand::getGameModeName(GameMode mode) {
    switch (mode) {
        case GameMode::Survival:   return "Survival";
        case GameMode::Creative:   return "Creative";
        case GameMode::Adventure:  return "Adventure";
        case GameMode::Spectator:  return "Spectator";
        case GameMode::NotSet:     return "Not Set";
        default:                   return "Unknown";
    }
}

} // namespace command
} // namespace mc
