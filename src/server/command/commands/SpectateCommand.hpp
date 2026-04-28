#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief SpectateCommand - 旁观模式跟踪
 *
 * 用法: /spectate <target> [player]
 * 权限: 2 (游戏管理员)
 */
class SpectateCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 startSpectating(CommandContext<ServerCommandSource>& context);
    static i32 stopSpectating(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
