#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief PublishCommand - 发布单人游戏到局域网
 *
 * 用法: /publish [port] [allowCheats]
 * 权限: 4 (服务器管理员)
 */
class PublishCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 publishToWorld(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
