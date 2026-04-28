#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /kill 命令
 *
 * 用法：
 * - /kill - 杀死自己
 * - /kill <target> - 杀死指定实体
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 的 KillCommand
 */
class KillCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 killSelf(CommandContext<ServerCommandSource>& context);
    static i32 killEntities(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
