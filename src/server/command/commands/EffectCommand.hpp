#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /effect 命令
 *
 * 管理实体的状态效果。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /effect give <player> <effect> [<seconds>] [<amplifier>] [<hideParticles>]
 * - /effect clear <player> [<effect>]
 */
class EffectCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 giveEffect(CommandContext<ServerCommandSource>& context);
    static i32 clearAllEffects(CommandContext<ServerCommandSource>& context);
    static i32 clearSpecificEffect(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
