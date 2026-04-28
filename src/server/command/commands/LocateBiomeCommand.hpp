#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief LocateBiomeCommand - 定位生物群系
 *
 * 用法: /locatebiome <biome>
 * 权限: 0 (所有玩家)
 */
class LocateBiomeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 locateBiome(CommandContext<ServerCommandSource>& context);
    static std::optional<BiomeId> parseBiomeId(const String& name) noexcept;
};

} // namespace command
} // namespace mc
