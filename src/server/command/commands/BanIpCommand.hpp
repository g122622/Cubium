#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /ban-ip 命令
 *
 * 用法：
 * - /ban-ip <target> [reason] - 封禁 IP 地址
 *
 * 权限等级：3（需要管理员权限）
 *
 * 参考 MC 1.16.5 的 BanIpCommand
 */
class BanIpCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 banIp(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
