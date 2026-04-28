#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief FunctionCommand - 执行函数文件
 *
 * 用法: /function <name>
 * 权限: 2 (游戏管理员)
 */
class FunctionCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 runFunction(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
