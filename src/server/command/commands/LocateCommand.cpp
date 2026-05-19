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

#include "LocateCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void LocateCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto locateNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("locate");
    locateNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(
        locateNode, support::makeMetadata("Locates the closest structure.", "/locate <structure>", 0, {}, true));

    auto structureArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "structure", StringArgumentType::string());
    structureArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return locateStructure(ctx); });
    locateNode->addChild(structureArg);

    dispatcher.registerCommand(locateNode);
}

i32 LocateCommand::locateStructure(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const std::string structureName = context.getArgument<std::string>("structure");
    const Vector3d& playerPos = source.position();

    // 规范化结构名称
    std::string normalizedName = normalizeStructureName(structureName);

    BlockPos searchCenter(static_cast<BlockCoord>(playerPos.x),
        static_cast<BlockCoord>(playerPos.y),
        static_cast<BlockCoord>(playerPos.z));

    // TODO: 实现真正的结构搜索，遍历区块查找结构起始点
    // 当前使用占位实现，返回基于玩家位置的估算
    // 实际实现需要访问世界的 StructureManager 并调用其搜索方法

    std::ostringstream ss;
    ss << "Searching for structure '" << structureName << "' near (" << searchCenter.x << ", " << searchCenter.y << ", "
       << searchCenter.z << ")...";
    source.sendMessage(ss.str());

    // 占位实现：提示功能尚未完全实现
    source.sendMessage("Structure location search is not yet fully implemented.");
    source.sendMessage("The structure '" + normalizedName + "' exists in the world generation system.");

    return 1;
}

std::string LocateCommand::normalizeStructureName(const std::string& name)
{
    // 移除 minecraft: 前缀
    std::string normalized = name;
    if (normalized.find("minecraft:") == 0) {
        normalized = normalized.substr(10);
    }

    // 将常见别名转换为内部名称
    static const std::unordered_map<std::string, std::string> aliases = {
        {"village", "village"},
        {"pillager_outpost", "pillager_outpost"},
        {"mansion", "mansion"},
        {"stronghold", "stronghold"},
        {"fortress", "fortress"},
        {"nether_fortress", "fortress"},
        {"mineshaft", "mineshaft"},
        {"ocean_monument", "ocean_monument"},
        {"monument", "ocean_monument"},
        {"buried_treasure", "buried_treasure"},
        {"shipwreck", "shipwreck"},
        {"ocean_ruin", "ocean_ruin"},
        {"ocean_ruins", "ocean_ruin"},
        {"ruined_portal", "ruined_portal"},
        {"bastion", "bastion_remnant"},
        {"bastion_remnant", "bastion_remnant"},
        {"endcity", "end_city"},
        {"end_city", "end_city"},
        {"desert_pyramid", "desert_pyramid"},
        {"desert_temple", "desert_pyramid"},
        {"jungle_temple", "jungle_temple"},
        {"jungle_pyramid", "jungle_temple"},
        {"witch_hut", "swamp_hut"},
        {"swamp_hut", "swamp_hut"},
    };

    auto it = aliases.find(normalized);
    if (it != aliases.end()) {
        return it->second;
    }

    return normalized;
}

} // namespace command
} // namespace mc
