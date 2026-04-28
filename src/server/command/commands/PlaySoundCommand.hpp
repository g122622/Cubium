#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /playsound 命令
 *
 * 播放声音效果。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /playsound <sound> <source> <player>
 * - /playsound <sound> <source> <player> <pos>
 * - /playsound <sound> <source> <player> <pos> <volume> <pitch> <minimumVolume>
 */
class PlaySoundCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 playSoundDefault(CommandContext<ServerCommandSource>& context);
    static i32 playSoundAtPosition(CommandContext<ServerCommandSource>& context);
    static i32 playSoundWithParams(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
