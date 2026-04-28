#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief CloneCommand - 复制方块区域
 *
 * 用法: /clone <begin> <end> <destination> [replace|masked|filtered] [normal|force|move]
 * 权限: 2 (游戏管理员)
 */
class CloneCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 cloneBlocks(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
