#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief ReloadCommand - 重新加载资源包和数据包
 *
 * 用法: /reload
 * 权限: 2 (游戏管理员)
 */
class ReloadCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 reload(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
