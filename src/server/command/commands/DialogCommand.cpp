#include "DialogCommand.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
#include "server/network/outbound/PacketBuilders.hpp"
#include <nlohmann/json.hpp>

namespace mc::command {
namespace {
i32 _show(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& input = context.getArgument<std::string>("dialog");
    auto definition = nlohmann::json::parse(input, nullptr, false);
    auto packet = !input.empty() && input.front() == '{'
        ? (definition.is_discarded()
                  ? Result<mc::network::ir::IrPacket>(Error(ErrorCode::InvalidData, "Invalid dialog JSON"))
                  : server::net::buildShowDialogIr(definition))
        : server::net::resolveDialogIr(ResourceLocation(input));
    if (packet.failed()) {
        source.sendError(packet.error().message());
        return 0;
    }
    const auto targets = support::resolvePlayerIds(source, context.getArgument<EntitySelector>("targets"));
    i32 sent = 0;
    for (const auto playerId : targets) {
        if (source.server()->connectionManager().sendToPlayer(playerId, packet.value())) {
            ++sent;
        }
    }
    return sent;
}

i32 _clear(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto targets = support::resolvePlayerIds(source, context.getArgument<EntitySelector>("targets"));
    const mc::network::ir::IrPacket packet{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{mc::network::ir::play::ClearDialog{}}};
    i32 sent = 0;
    for (const auto playerId : targets) {
        if (source.server()->connectionManager().sendToPlayer(playerId, packet)) {
            ++sent;
        }
    }
    return sent;
}
} // namespace

void DialogCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto root = std::make_shared<LiteralCommandNode<ServerCommandSource>>("dialog");
    root->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(root,
        support::makeMetadata(
            "Shows or clears data driven dialogs.", "/dialog (show <targets> <id|JSON>|clear <targets>)", 2, {}, true));
    auto show = std::make_shared<LiteralCommandNode<ServerCommandSource>>("show");
    auto targets = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    auto dialog = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "dialog", StringArgumentType::greedyString());
    dialog->setCommand(_show);
    targets->addChild(dialog);
    show->addChild(targets);
    root->addChild(show);
    auto clear = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    auto clearTargets = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    clearTargets->setCommand(_clear);
    clear->addChild(clearTargets);
    root->addChild(clear);
    dispatcher.registerCommand(root);
}
} // namespace mc::command
