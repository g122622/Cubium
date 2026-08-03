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

#include "DataPackCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

void DataPackCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto datapackNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("datapack");
    datapackNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(datapackNode,
        support::makeMetadata("Controls data packs.", "/datapack <enable|disable|list> [name]", 2, {}, true));

    // /datapack enable <name>
    auto enableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    enableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _enableDataPack(ctx); });
    enableNode->addChild(enableNameArg);
    datapackNode->addChild(enableNode);

    // /datapack disable <name>
    auto disableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("disable");
    auto disableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    disableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _disableDataPack(ctx); });
    disableNode->addChild(disableNameArg);
    datapackNode->addChild(disableNode);

    // /datapack list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listDataPacks(ctx); });
    datapackNode->addChild(listNode);

    dispatcher.registerCommand(datapackNode);
}

i32 DataPackCommand::_enableDataPack(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    auto* server = source.server();
    if (!server) {
        source.sendMessage("Failed to enable data pack: server not available");
        return 0;
    }

    auto& dataPacks = server->dataPackList();

    // Search for a pack matching the given name (by path or pack name)
    auto allPacks = dataPacks.getAllPacks();
    std::string matchedPath;
    for (const auto& packInfo : allPacks) {
        // Match by the last component of the path (directory/zip name)
        std::filesystem::path packPath(packInfo.path);
        if (packPath.filename().string() == name || packPath.stem().string() == name) {
            matchedPath = packInfo.path;
            break;
        }
    }

    if (matchedPath.empty()) {
        source.sendMessage("Unknown data pack '" + name + "'");
        return 0;
    }

    if (dataPacks.setEnabled(matchedPath, true)) {
        source.sendMessage("Enabled data pack '" + name + "'");
        return 1;
    }

    source.sendMessage("Failed to enable data pack '" + name + "'");
    return 0;
}

i32 DataPackCommand::_disableDataPack(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    auto* server = source.server();
    if (!server) {
        source.sendMessage("Failed to disable data pack: server not available");
        return 0;
    }

    auto& dataPacks = server->dataPackList();

    auto allPacks = dataPacks.getAllPacks();
    std::string matchedPath;
    for (const auto& packInfo : allPacks) {
        std::filesystem::path packPath(packInfo.path);
        if (packPath.filename().string() == name || packPath.stem().string() == name) {
            matchedPath = packInfo.path;
            break;
        }
    }

    if (matchedPath.empty()) {
        source.sendMessage("Unknown data pack '" + name + "'");
        return 0;
    }

    if (dataPacks.setEnabled(matchedPath, false)) {
        source.sendMessage("Disabled data pack '" + name + "'");
        return 1;
    }

    source.sendMessage("Failed to disable data pack '" + name + "'");
    return 0;
}

i32 DataPackCommand::_listDataPacks(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    auto* server = source.server();
    if (!server) {
        source.sendMessage("No data packs available");
        return 0;
    }

    auto& dataPacks = server->dataPackList();
    auto allPacks = dataPacks.getAllPacks();

    if (allPacks.empty()) {
        source.sendMessage("There are no data packs available");
        return 0;
    }

    std::ostringstream enabledList;
    std::ostringstream disabledList;
    i32 enabledCount = 0;
    i32 disabledCount = 0;

    for (const auto& packInfo : allPacks) {
        std::filesystem::path packPath(packInfo.path);
        std::string packName = packPath.stem().string();

        if (packInfo.enabled) {
            if (enabledCount > 0) {
                enabledList << ", ";
            }
            enabledList << packName;
            ++enabledCount;
        } else {
            if (disabledCount > 0) {
                disabledList << ", ";
            }
            disabledList << packName;
            ++disabledCount;
        }
    }

    source.sendMessage("Enabled data packs (" + std::to_string(enabledCount) + "): " + enabledList.str());
    source.sendMessage("Disabled data packs (" + std::to_string(disabledCount) + "): " + disabledList.str());
    return 1;
}

} // namespace command
} // namespace mc
