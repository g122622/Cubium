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
 * The above notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LocateCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

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

    // 将用户输入规范化为 ResourceLocation
    ResourceLocation structureId = _normalizeToResourceLocation(structureName);

    // 验证结构是否存在于 StructureSetRegistry 中
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();
    const world::gen::structure::StructureSet* structureSet = structureSetRegistry.findByStructure(structureId);
    if (structureSet == nullptr) {
        source.sendError("Unknown structure: " + structureName);
        source.sendError(
            "Use structure IDs like minecraft:village, minecraft:desert_pyramid, minecraft:stronghold, etc.");
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

    auto result = world->findNearestStructure(searchCenter, structureId, SEARCH_RADIUS, false);

    if (result.has_value()) {
        i32 dx = result->x - searchCenter.x;
        i32 dz = result->z - searchCenter.z;
        i32 distance = static_cast<i32>(std::sqrt(static_cast<f64>(dx * dx + dz * dz)));

        std::ostringstream ss;
        ss << "Found " << structureId.toString() << " at (" << result->x << ", " << result->z << ") "
           << "(" << distance << " blocks away)";
        source.sendMessage(ss.str());
        return 1;
    } else {
        std::ostringstream errorSs;
        errorSs << "Could not find structure '" << structureId.toString() << "' within " << SEARCH_RADIUS << " blocks";
        source.sendError(errorSs.str());
        return 0;
    }
}

ResourceLocation LocateCommand::_normalizeToResourceLocation(const std::string& name)
{
    // 如果用户输入已经包含命名空间（如 minecraft:village），直接解析
    if (name.find(':') != std::string::npos) {
        return ResourceLocation::parse(name);
    }

    // 常见别名映射：用户友好的名称 → ResourceLocation
    // 对齐 MC Java 的 /locate 命令结构名称
    static const std::unordered_map<std::string, std::string> aliases = {
        // 村庄变体
        {"village", "minecraft:village_plains"},
        {"village_plains", "minecraft:village_plains"},
        {"village_desert", "minecraft:village_desert"},
        {"village_savanna", "minecraft:village_savanna"},
        {"village_snowy", "minecraft:village_snowy"},
        {"village_taiga", "minecraft:village_taiga"},

        // 神殿/神庙类
        {"temple", "minecraft:desert_pyramid"},
        {"desert_pyramid", "minecraft:desert_pyramid"},
        {"desert_temple", "minecraft:desert_pyramid"},
        {"jungle_temple", "minecraft:jungle_pyramid"},
        {"jungle_pyramid", "minecraft:jungle_pyramid"},
        {"igloo", "minecraft:igloo"},
        {"swamp_hut", "minecraft:swamp_hut"},
        {"witch_hut", "minecraft:swamp_hut"},

        // 主要结构
        {"mansion", "minecraft:mansion"},
        {"woodland_mansion", "minecraft:mansion"},
        {"monument", "minecraft:monument"},
        {"ocean_monument", "minecraft:monument"},
        {"stronghold", "minecraft:stronghold"},
        {"mineshaft", "minecraft:mineshaft"},
        {"buried_treasure", "minecraft:buried_treasure"},
        {"shipwreck", "minecraft:shipwreck"},
        {"ocean_ruin", "minecraft:ocean_ruin_cold"},
        {"ocean_ruins", "minecraft:ocean_ruin_cold"},
        {"ruined_portal", "minecraft:ruined_portal"},
        {"bastion", "minecraft:bastion_remnant"},
        {"bastion_remnant", "minecraft:bastion_remnant"},
        {"fortress", "minecraft:fortress"},
        {"nether_fortress", "minecraft:fortress"},
        {"end_city", "minecraft:end_city"},
        {"endcity", "minecraft:end_city"},
        {"pillager_outpost", "minecraft:pillager_outpost"},
        {"outpost", "minecraft:pillager_outpost"},

        // 1.21+ 结构
        {"trial_chambers", "minecraft:trial_chambers"},
        {"ancient_city", "minecraft:ancient_city"},
        {"nether_fossil", "minecraft:nether_fossil"},
        {"trail_ruins", "minecraft:trail_ruins"},
    };

    auto it = aliases.find(name);
    if (it != aliases.end()) {
        return ResourceLocation::parse(it->second);
    }

    // 未知名称，尝试添加 minecraft: 命名空间
    return ResourceLocation("minecraft", name);
}

} // namespace command
} // namespace mc
