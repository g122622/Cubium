#include "PlaySoundCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void PlaySoundCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto playsoundNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("playsound");
    playsoundNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        playsoundNode,
        support::makeMetadata(
            "Plays a sound effect.",
            "/playsound <sound> <source> <player> [<pos>] [<volume>] [<pitch>] [<minimumVolume>]",
            2,
            {},
            true));

    // /playsound <sound>
    auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "sound",
        ResourceLocationArgumentType::resourceLocation()
    );

    // 声源子节点
    auto createSourceNode = [](const char* name) {
        return std::make_shared<LiteralCommandNode<ServerCommandSource>>(name);
    };

    auto masterNode = createSourceNode("master");
    auto musicNode = createSourceNode("music");
    auto recordNode = createSourceNode("record");
    auto weatherNode = createSourceNode("weather");
    auto blockNode = createSourceNode("block");
    auto hostileNode = createSourceNode("hostile");
    auto neutralNode = createSourceNode("neutral");
    auto playerSoundNode = createSourceNode("player");
    auto ambientNode = createSourceNode("ambient");
    auto voiceNode = createSourceNode("voice");

    // 为每个声源添加目标节点
    for (auto& sourceNode : {masterNode, musicNode, recordNode, weatherNode, blockNode,
                              hostileNode, neutralNode, playerSoundNode, ambientNode, voiceNode}) {
        auto targetNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "player",
            EntityArgumentType::player()
        );
        targetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return playSoundDefault(ctx);
        });

        auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
            "pos",
            Vec3ArgumentType::vec3()
        );
        posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return playSoundAtPosition(ctx);
        });

        targetNode->addChild(posNode);
        sourceNode->addChild(targetNode);
        soundNode->addChild(sourceNode);
    }

    playsoundNode->addChild(soundNode);
    dispatcher.registerCommand(playsoundNode);
}

i32 PlaySoundCommand::playSoundDefault(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    auto& selector = context.getArgument<EntitySelector>("player");
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

        // TODO: 发送 PlaySoundPacket
        successCount++;
    }

    return successCount;
}

i32 PlaySoundCommand::playSoundAtPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d& pos = context.getArgument<Vector3d>("pos");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // TODO: 发送 PlaySoundPacket 到指定位置

    return static_cast<i32>(playerIds.size());
}

i32 PlaySoundCommand::playSoundWithParams(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d& pos = context.getArgument<Vector3d>("pos");
    f32 volume = context.getArgument<f32>("volume");
    f32 pitch = context.getArgument<f32>("pitch");
    f32 minVolume = context.getArgument<f32>("minimumVolume");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // TODO: 发送 PlaySoundPacket 到指定位置带参数

    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
