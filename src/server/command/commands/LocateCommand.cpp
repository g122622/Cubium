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
#include "common/world/gen/structure/Structure.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
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
    structureArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _locateStructure(ctx); });
    locateNode->addChild(structureArg);

    dispatcher.registerCommand(locateNode);
}

i32 LocateCommand::_locateStructure(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const std::string structureName = context.getArgument<std::string>("structure");
    const Vector3d& playerPos = source.position();

    // 通过 Structure::nameToStructureType 将结构名称转换为 StructureType 枚举
    auto structureType = world::gen::structure::Structure::nameToStructureType(structureName);
    if (!structureType.has_value()) {
        source.sendError("Unknown structure: " + structureName);
        source.sendError(
            "Valid structures: village, pillager_outpost, mansion, stronghold, fortress, mineshaft, "
            "ocean_monument, buried_treasure, shipwreck, ocean_ruin, ruined_portal, bastion_remnant, "
            "end_city, trial_chambers, temple (desert_pyramid/jungle_temple/igloo/swamp_hut/nether_fossil)");
        return 0;
    }

    // 获取 ServerWorld
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not available");
        return 0;
    }

    BlockPos searchCenter(static_cast<BlockCoord>(playerPos.x),
        static_cast<BlockCoord>(playerPos.y),
        static_cast<BlockCoord>(playerPos.z));

    // 搜索最近的结构，半径 6400 格（与 MC Java 版 /locate 一致）
    constexpr i32 SEARCH_RADIUS = 6400;

    auto result = world->findNearestStructure(searchCenter, structureType.value(), SEARCH_RADIUS, false);

    if (result.has_value()) {
        i32 dx = result->x - searchCenter.x;
        i32 dz = result->z - searchCenter.z;
        i32 distance = static_cast<i32>(std::sqrt(static_cast<f64>(dx * dx + dz * dz)));

        std::ostringstream ss;
        ss << "Found " << structureName << " at (" << result->x << ", " << result->z << ") "
           << "(" << distance << " blocks away)";
        source.sendMessage(ss.str());
        return 1;
    } else {
        std::ostringstream errorSs;
        errorSs << "Could not find structure '" << structureName << "' within " << SEARCH_RADIUS << " blocks";
        source.sendError(errorSs.str());
        return 0;
    }
}

std::string LocateCommand::_normalizeStructureName(const std::string& name)
{
    // 规范化结构名称：移除 minecraft: 前缀并解析别名
    auto structureType = world::gen::structure::Structure::nameToStructureType(name);
    if (structureType.has_value()) {
        auto id = world::gen::structure::Structure::typeToId(structureType.value());
        return id.toString();
    }
    return name;
}

} // namespace command
} // namespace mc
