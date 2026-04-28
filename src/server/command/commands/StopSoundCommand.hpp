#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /stopsound 命令
 *
 * 停止播放声音效果。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /stopsound <player>
 * - /stopsound <player> <source>
 * - /stopsound <player> <source> <sound>
 */
class StopSoundCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 stopAllSounds(CommandContext<ServerCommandSource>& context);
    static i32 stopSourceSounds(CommandContext<ServerCommandSource>& context);
    static i32 stopSpecificSound(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
