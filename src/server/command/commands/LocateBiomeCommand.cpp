#include "LocateBiomeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "common/world/biome/Biomes.hpp"
#include <sstream>

namespace mc {
namespace command {

void LocateBiomeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto locateBiomeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("locatebiome");
    locateBiomeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);
    });
    support::applyMetadata(
        locateBiomeNode,
        support::makeMetadata(
            "Locates the closest biome.",
            "/locatebiome <biome>",
            0,
            {},
            true));

    auto biomeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "biome",
        StringArgumentType::string());
    biomeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return locateBiome(ctx);
    });
    locateBiomeNode->addChild(biomeArg);

    dispatcher.registerCommand(locateBiomeNode);
}

i32 LocateBiomeCommand::locateBiome(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const String biomeName = context.getArgument<String>("biome");
    const Vector3d& playerPos = source.position();

    // 解析生物群系
    auto biomeId = parseBiomeId(biomeName);
    if (!biomeId.has_value()) {
        source.sendMessage("Unknown biome: " + biomeName);
        source.sendMessage("Use /locatebiome with a valid biome ID (e.g., plains, desert, forest)");
        return 0;
    }

    BlockPos searchCenter(
        static_cast<BlockCoord>(playerPos.x),
        static_cast<BlockCoord>(playerPos.y),
        static_cast<BlockCoord>(playerPos.z)
    );

    std::ostringstream ss;
    ss << "Searching for biome near ("
       << searchCenter.x << ", " << searchCenter.z << ")...";
    source.sendMessage(ss.str());

    // TODO: 实现真正的生物群系搜索
    // 需要访问世界的 BiomeProvider，向外螺旋搜索直到找到目标生物群系

    source.sendMessage("Biome location search is not yet fully implemented.");
    source.sendMessage("Biome ID: " + std::to_string(static_cast<i32>(biomeId.value())));

    return 1;
}

std::optional<BiomeId> LocateBiomeCommand::parseBiomeId(const String& name) noexcept
{
    // 规范化名称
    String normalized = name;
    if (normalized.find("minecraft:") == 0) {
        normalized = normalized.substr(10);
    }

    // 常见生物群系别名映射
    static const std::unordered_map<String, BiomeId> biomeAliases = {
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
