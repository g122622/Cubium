#include "EffectCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/command/support/EffectResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void EffectCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto effectNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("effect");
    effectNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        effectNode,
        support::makeMetadata(
            "Gives or removes status effects from players.",
            "/effect (give|clear) <player> [<effect>] [<seconds>] [<amplifier>] [<hideParticles>]",
            2,
            {},
            true));

    // /effect give <player> <effect> [<seconds>] [<amplifier>] [<hideParticles>]
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");

    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );

    // 效果类型节点 - 使用字符串参数
    auto effectNameNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "effect",
        StringArgumentType::word()
    );

    // 可选参数
    auto secondsNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "seconds",
        IntegerArgumentType::integer(0, 1000000)
    );
    secondsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveEffect(ctx);
    });

    auto amplifierNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amplifier",
        IntegerArgumentType::integer(0, 255)
    );
    amplifierNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveEffect(ctx);
    });

    auto hideParticlesNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>(
        "hideParticles",
        BoolArgumentType::boolArg()
    );
    hideParticlesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveEffect(ctx);
    });

    // 默认使用效果名称
    effectNameNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveEffect(ctx);
    });

    amplifierNode->addChild(hideParticlesNode);
    secondsNode->addChild(amplifierNode);
    effectNameNode->addChild(secondsNode);
    playerNode->addChild(effectNameNode);
    giveNode->addChild(playerNode);

    // /effect clear <player> [<effect>]
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");

    auto clearPlayerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    clearPlayerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearAllEffects(ctx);
    });

    auto clearEffectNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "effect",
        StringArgumentType::word()
    );
    clearEffectNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearSpecificEffect(ctx);
    });

    clearPlayerNode->addChild(clearEffectNode);
    clearNode->addChild(clearPlayerNode);

    effectNode->addChild(giveNode);
    effectNode->addChild(clearNode);
    dispatcher.registerCommand(effectNode);
}

i32 EffectCommand::giveEffect(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const String& effectName = context.getArgument<String>("effect");

    auto effectTypeOpt = support::tryParseEffectType(effectName);
    if (!effectTypeOpt.has_value()) {
        source.sendError("Unknown effect: " + effectName);
        return 0;
    }
    entity::effect::EffectType effectType = effectTypeOpt.value();

    // 获取可选参数
    i32 seconds = 30;  // 默认30秒
    i32 amplifier = 0; // 默认等级0
    bool hideParticles = false;

    if (context.hasArgument("seconds")) {
        seconds = context.getArgument<i32>("seconds");
    }
    if (context.hasArgument("amplifier")) {
        amplifier = context.getArgument<i32>("amplifier");
    }
    if (context.hasArgument("hideParticles")) {
        hideParticles = context.getArgument<bool>("hideParticles");
    }

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // 添加效果
        entity::effect::EffectInstance effect(effectType, seconds * 20, amplifier, false, !hideParticles);
        playerData->addEffect(effect);
        successCount++;
    }

    std::ostringstream ss;
    ss << "Gave " << support::getEffectCommandName(effectType);
    if (amplifier > 0) {
        ss << " " << (amplifier + 1);
    }
    ss << " to " << successCount << " player(s)";
    source.sendMessage(ss.str());

    return successCount;
}

i32 EffectCommand::clearAllEffects(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        size_t effectCount = playerData->getAllEffects().size();
        playerData->removeAllEffects();
        successCount += static_cast<i32>(effectCount);
    }

    source.sendMessage("Cleared all effects");
    return successCount;
}

i32 EffectCommand::clearSpecificEffect(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const String& effectName = context.getArgument<String>("effect");

    auto effectTypeOpt = support::tryParseEffectType(effectName);
    if (!effectTypeOpt.has_value()) {
        source.sendError("Unknown effect: " + effectName);
        return 0;
    }
    entity::effect::EffectType effectType = effectTypeOpt.value();

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        if (playerData->hasEffect(effectType)) {
            playerData->removeEffect(effectType);
            successCount++;
        }
    }

    std::ostringstream ss;
    ss << "Cleared " << support::getEffectCommandName(effectType) << " from " << successCount << " player(s)";
    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
