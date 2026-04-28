#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /title 命令
 *
 * 控制玩家屏幕标题显示。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /title <player> clear - 清除标题
 * - /title <player> reset - 重置标题
 * - /title <player> title <json> - 设置标题
 * - /title <player> subtitle <json> - 设置副标题
 * - /title <player> actionbar <json> - 设置动作栏
 * - /title <player> times <fadeIn> <stay> <fadeOut> - 设置时间
 */
class TitleCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 clearTitle(CommandContext<ServerCommandSource>& context);
    static i32 resetTitle(CommandContext<ServerCommandSource>& context);
    static i32 setTitle(CommandContext<ServerCommandSource>& context);
    static i32 setSubtitle(CommandContext<ServerCommandSource>& context);
    static i32 setActionbar(CommandContext<ServerCommandSource>& context);
    static i32 setTimes(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
