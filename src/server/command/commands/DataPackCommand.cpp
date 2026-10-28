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
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

void DataPackCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto datapackNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("datapack");
    datapackNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(datapackNode,
        support::makeMetadata("Controls data packs.", "/datapack <enable|disable|list> [name]", 2, {}, true));

    // /datapack enable <name>
    auto enableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    enableNameArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    enableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _enableDataPack(ctx);
    });
    // 原版 enable 支持 first / last / before <existing> / after <existing>。
    for (const char* position : {"first", "last"}) {
        auto positionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(position);
        positionNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _enableDataPack(ctx);
        });
        enableNameArg->addChild(positionNode);
    }
    for (const char* position : {"before", "after"}) {
        auto positionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(position);
        auto existingArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "existing", StringArgumentType::string());
        existingArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
        existingArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _enableDataPack(ctx);
        });
        positionNode->addChild(existingArg);
        enableNameArg->addChild(positionNode);
    }
    enableNode->addChild(enableNameArg);
    datapackNode->addChild(enableNode);

    // /datapack disable <name>
    auto disableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("disable");
    auto disableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    disableNameArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    disableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _disableDataPack(ctx);
    });
    disableNode->addChild(disableNameArg);
    datapackNode->addChild(disableNode);

    // /datapack create <id> [description]
    // 原版 create 会新建一个空数据包；这里在数据包目录下建出 pack.mcmeta 与空的 data/，
    // 并把它加入列表（默认启用）。id 只允许字母数字、下划线和连字符，避免路径穿越。
    auto createNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("create");
    auto createIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());
    // 原版 create 可以再跟一个描述（聊天组件）。
    auto createDescriptionArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "description", ComponentArgumentType::component());
    createDescriptionArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _createDataPack(ctx);
    });
    createIdArg->addChild(createDescriptionArg);
    createNode->addChild(createIdArg);
    datapackNode->addChild(createNode);

    // /datapack list [available|enabled]
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listDataPacks(ctx);
    });
    for (const char* filter : {"available", "enabled"}) {
        auto filterNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(filter);
        const std::string filterName = filter;
        filterNode->setCommand([filterName](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("filter", filterName);
            return _listDataPacks(ctx);
        });
        listNode->addChild(filterNode);
    }
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
        server::net::setRegistryDatapackSource(dataPacks);
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
        server::net::setRegistryDatapackSource(dataPacks);
        source.sendMessage("Disabled data pack '" + name + "'");
        return 1;
    }

    source.sendMessage("Failed to disable data pack '" + name + "'");
    return 0;
}

i32 DataPackCommand::_createDataPack(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string id = context.getArgument<std::string>("id");

    for (const char c : id) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!ok) {
            source.sendError("Invalid data pack id: " + id + " (only [a-z0-9_-] are allowed)");
            return 0;
        }
    }

    auto* server = source.server();
    if (!server) {
        source.sendError("Failed to create data pack: server not available");
        return 0;
    }

    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("datapacks") / id;
    if (std::filesystem::exists(dir, ec)) {
        source.sendError("A data pack with id '" + id + "' already exists");
        return 0;
    }

    std::filesystem::create_directories(dir / "data", ec);
    if (ec) {
        source.sendError("Failed to create data pack '" + id + "': " + ec.message());
        return 0;
    }

    // 原版新建的包是一个标准的 pack.mcmeta（pack_format 1，description 为 "id"）。
    std::ofstream meta(dir / "pack.mcmeta", std::ios::trunc);
    meta << "{\"pack\":{\"pack_format\":1,\"description\":\"" << id << "\"}}" << char(10);
    meta.close();

    auto& dataPacks = server->dataPackList();
    const auto added = dataPacks.addPack(dir, true, 0);
    if (!added.success()) {
        source.sendError("Failed to create data pack '" + id + "'");
        return 0;
    }
    server::net::setRegistryDatapackSource(dataPacks);
    source.sendMessage("Created empty data pack '" + id + "'");
    return 1;
}

i32 DataPackCommand::_listDataPacks(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string filter = context.getArgumentOr<std::string>("filter", "all");

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
        } else if (filter == "enabled") {
            continue;
        } else {
            if (disabledCount > 0) {
                disabledList << ", ";
            }
            disabledList << packName;
            ++disabledCount;
        }
    }

    if (filter == "available") {
        // 原版 available = 所有可用的包（含已启用）。
        source.sendMessage("Enabled data packs (" + std::to_string(enabledCount) + "): " + enabledList.str());
        source.sendMessage("Disabled data packs (" + std::to_string(disabledCount) + "): " + disabledList.str());
    } else if (filter == "enabled") {
        source.sendMessage("Enabled data packs (" + std::to_string(enabledCount) + "): " + enabledList.str());
    } else {
        source.sendMessage("Enabled data packs (" + std::to_string(enabledCount) + "): " + enabledList.str());
        source.sendMessage("Disabled data packs (" + std::to_string(disabledCount) + "): " + disabledList.str());
    }
    return 1;
}

} // namespace command
} // namespace mc
