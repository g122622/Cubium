#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief BossBarCommand - Boss 栏管理
 *
 * 用法: /bossbar <add|remove|list|set|get> ...
 * 权限: 2 (游戏管理员)
 */
class BossBarCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 addBossBar(CommandContext<ServerCommandSource>& context);
    static i32 removeBossBar(CommandContext<ServerCommandSource>& context);
    static i32 listBossBars(CommandContext<ServerCommandSource>& context);
    static i32 setBossBar(CommandContext<ServerCommandSource>& context);
    static i32 getBossBar(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
