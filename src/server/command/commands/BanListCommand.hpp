#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /banlist 命令
 *
 * 显示服务器封禁列表。
 *
 * 用法：
 * - /banlist - 显示所有封禁（玩家和 IP）
 * - /banlist players - 显示封禁的玩家
 * - /banlist ips - 显示封禁的 IP 地址
 *
 * 权限等级：3
 */
class BanListCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 列出所有封禁
     */
    static i32 listAll(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 列出封禁的玩家
     */
    static i32 listPlayers(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 列出封禁的 IP
     */
    static i32 listIps(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
