#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /whitelist 命令
 *
 * 用法：
 * - /whitelist on - 开启白名单
 * - /whitelist off - 关闭白名单
 * - /whitelist list - 列出白名单玩家
 * - /whitelist add <player> - 添加玩家到白名单
 * - /whitelist remove <player> - 从白名单移除玩家
 * - /whitelist reload - 重新加载白名单
 *
 * 权限等级：3（需要管理员权限）
 *
 * 参考 MC 1.16.5 的 WhitelistCommand
 */
class WhitelistCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 whitelistOn(CommandContext<ServerCommandSource>& context);
    static i32 whitelistOff(CommandContext<ServerCommandSource>& context);
    static i32 whitelistList(CommandContext<ServerCommandSource>& context);
    static i32 whitelistAdd(CommandContext<ServerCommandSource>& context);
    static i32 whitelistRemove(CommandContext<ServerCommandSource>& context);
    static i32 whitelistReload(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
