#include "StopSoundCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"

namespace mc {
namespace command {

void StopSoundCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto stopsoundNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stopsound");
    stopsoundNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        stopsoundNode,
        support::makeMetadata(
            "Stops playing a sound effect.",
            "/stopsound <player> [<source>] [<sound>]",
            2,
            {},
            true));

    // /stopsound <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    playerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return stopAllSounds(ctx);
    });

    auto masterNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("master");
    auto musicNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("music");
    auto recordNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("record");
    auto weatherNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("weather");
    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto hostileNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hostile");
    auto neutralNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("neutral");
    auto playerSoundNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("player");
    auto ambientNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ambient");
    auto voiceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("voice");

    // /stopsound <player> <source> <sound>
    auto createSoundNode = []() {
        auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "sound",
            ResourceLocationArgumentType::resourceLocation()
        );
        soundNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return stopSpecificSound(ctx);
        });
        return soundNode;
    };

    for (auto& sourceNode : {masterNode, musicNode, recordNode, weatherNode, blockNode,
                              hostileNode, neutralNode, playerSoundNode, ambientNode, voiceNode}) {
        sourceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return stopSourceSounds(ctx);
        });
        sourceNode->addChild(createSoundNode());
        playerNode->addChild(sourceNode);
    }

    stopsoundNode->addChild(playerNode);
    dispatcher.registerCommand(stopsoundNode);
}

i32 StopSoundCommand::stopAllSounds(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // TODO: 发送 StopSoundPacket 停止所有声音
    return static_cast<i32>(playerIds.size());
}

i32 StopSoundCommand::stopSourceSounds(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // TODO: 发送 StopSoundPacket 停止指定声源
    return static_cast<i32>(playerIds.size());
}

i32 StopSoundCommand::stopSpecificSound(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto& selector = context.getArgument<EntitySelector>("player");
    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // TODO: 发送 StopSoundPacket 停止指定声音

    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
