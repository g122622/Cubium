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

#include "LocateBiomeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace mc {
namespace command {

void LocateBiomeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto locateBiomeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("locatebiome");
    locateBiomeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(
        locateBiomeNode, support::makeMetadata("Locates the closest biome.", "/locatebiome <biome>", 0, {}, true));

    auto biomeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("biome", StringArgumentType::string());
    biomeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _locateBiome(ctx); });
    locateBiomeNode->addChild(biomeArg);

    dispatcher.registerCommand(locateBiomeNode);
}

i32 LocateBiomeCommand::_locateBiome(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const std::string biomeName = context.getArgument<std::string>("biome");
    const Vector3d& playerPos = source.position();

    // 解析生物群系
    auto biomeId = _parseBiomeId(biomeName);
    if (!biomeId.has_value()) {
        source.sendError("Unknown biome: " + biomeName);
        source.sendError("Use /locatebiome with a valid biome ID (e.g., plains, desert, forest)");
        return 0;
    }

    // 获取 ServerWorld
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not available");
        return 0;
    }

    // 获取区块管理器和生成器
    auto* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("Chunk manager not available");
        return 0;
    }

    auto* generator = chunkManager->generator();
    if (generator == nullptr) {
        source.sendError("Chunk generator not available");
        return 0;
    }

    BlockPos searchCenter(static_cast<BlockCoord>(playerPos.x),
        static_cast<BlockCoord>(playerPos.y),
        static_cast<BlockCoord>(playerPos.z));

    std::ostringstream ss;
    ss << "Searching for biome '" << biomeName << "' near (" << searchCenter.x << ", " << searchCenter.z << ")...";
    source.sendMessage(ss.str());

    // 创建生物群系匹配谓词
    auto predicate = [targetBiome = biomeId.value()](BiomeId biome) { return biome == targetBiome; };

    // 创建随机数生成器
    math::Random random(static_cast<u64>(std::chrono::system_clock::now().time_since_epoch().count()));

    // 搜索生物群系
    // 搜索半径 6400 格，步长 8（对应 2 个噪声单元）
    constexpr i32 SEARCH_RADIUS = 6400;
    constexpr i32 SEARCH_STEP = 8;

    world::biome::IBiomeSource* biomeSource = generator->getBiomeSource();
    if (biomeSource == nullptr) {
        source.sendError("Biome source not available");
        return 0;
    }

    auto result = biomeSource->findBiome(searchCenter.x,
        searchCenter.y,
        searchCenter.z,
        SEARCH_RADIUS,
        SEARCH_STEP,
        predicate,
        random,
        true // stopOnFirst - 找到第一个即返回
    );

    if (result.has_value()) {
        i32 dx = result->x - searchCenter.x;
        i32 dz = result->z - searchCenter.z;
        i32 distance = static_cast<i32>(std::sqrt(static_cast<f64>(dx * dx + dz * dz)));

        std::ostringstream resultSs;
        resultSs << "Found " << biomeName << " at (" << result->x << ", " << result->z << ") "
                 << "(" << distance << " blocks away)";
        source.sendMessage(resultSs.str());
        return 1;
    } else {
        std::ostringstream errorSs;
        errorSs << "Could not find biome '" << biomeName << "' within " << SEARCH_RADIUS << " blocks";
        source.sendError(errorSs.str());
        return 0;
    }
}

std::optional<BiomeId> LocateBiomeCommand::_parseBiomeId(const std::string& name) noexcept
{
    // 规范化名称
    std::string normalized = name;
    if (normalized.find("minecraft:") == 0) {
        normalized = normalized.substr(10);
    }

    // 常见生物群系别名映射
    static const std::unordered_map<std::string, BiomeId> biomeAliases = {
        {"plains", Biomes::Plains},
        {"sunflower_plains", Biomes::SunflowerPlains},
        {"forest", Biomes::Forest},
        {"flower_forest", Biomes::FlowerForest},
        {"birch_forest", Biomes::BirchForest},
        {"dark_forest", Biomes::DarkForest},
        {"taiga", Biomes::Taiga},
        {"snowy_taiga", Biomes::SnowyTaiga},
        {"desert", Biomes::Desert},
        {"badlands", Biomes::Badlands},
        {"savanna", Biomes::Savanna},
        {"jungle", Biomes::Jungle},
        {"swamp", Biomes::Swamp},
        {"beach", Biomes::Beach},
        {"snowy_beach", Biomes::SnowyBeach},
        {"stony_shore", Biomes::StoneShore},
        {"ocean", Biomes::Ocean},
        {"deep_ocean", Biomes::DeepOcean},
        {"warm_ocean", Biomes::WarmOcean},
        {"lukewarm_ocean", Biomes::LukewarmOcean},
        {"cold_ocean", Biomes::ColdOcean},
        {"frozen_ocean", Biomes::FrozenOcean},
        {"river", Biomes::River},
        {"frozen_river", Biomes::FrozenRiver},
        {"mountains", Biomes::Mountains},
        {"snowy_mountains", Biomes::SnowyMountains},
        {"wooded_hills", Biomes::WoodedHills},
        {"taiga_hills", Biomes::TaigaHills},
        {"desert_hills", Biomes::DesertHills},
        {"badlands_plateau", Biomes::BadlandsPlateau},
        {"savanna_plateau", Biomes::SavannaPlateau},
        {"nether_wastes", Biomes::NetherWastes},
        {"crimson_forest", Biomes::CrimsonForest},
        {"warped_forest", Biomes::WarpedForest},
        {"soul_sand_valley", Biomes::SoulSandValley},
        {"basalt_deltas", Biomes::BasaltDeltas},
        {"the_end", Biomes::TheEnd},
        {"mushroom_fields", Biomes::MushroomFields},
        {"ice_spikes", Biomes::IceSpikes},
    };

    auto it = biomeAliases.find(normalized);
    if (it != biomeAliases.end()) {
        return it->second;
    }

    return std::nullopt;
}

} // namespace command
} // namespace mc
