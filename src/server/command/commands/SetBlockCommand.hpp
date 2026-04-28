#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /setblock 命令
 *
 * 用法：
 * - /setblock <pos> <block> - 在指定位置放置方块
 * - /setblock <pos> <block> destroy - 破坏原有方块并放置新方块
 * - /setblock <pos> <block> keep - 仅在目标位置为空气时放置
 * - /setblock <pos> <block> replace - 替换目标位置的方块（默认）
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 的 SetBlockCommand
 */
class SetBlockCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setBlock(CommandContext<ServerCommandSource>& context);
    static i32 setBlockDestroy(CommandContext<ServerCommandSource>& context);
    static i32 setBlockKeep(CommandContext<ServerCommandSource>& context);
    static i32 setBlockReplace(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
