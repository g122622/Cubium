#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /execute 命令
 *
 * 用法：
 * - /execute as <entity> <command> - 以指定实体身份执行命令
 * - /execute at <entity> <command> - 在指定实体位置执行命令
 * - /execute positioned <pos> <command> - 在指定位置执行命令
 * - /execute run <command> - 执行命令
 * - /execute if block <pos> <block> <command> - 如果指定位置是指定方块则执行
 * - /execute unless block <pos> <block> <command> - 如果指定位置不是指定方块则执行
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 的 ExecuteCommand
 */
class ExecuteCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // 子命令执行
    static i32 executeAs(CommandContext<ServerCommandSource>& context);
    static i32 executeAt(CommandContext<ServerCommandSource>& context);
    static i32 executePositioned(CommandContext<ServerCommandSource>& context);
    static i32 executeRun(CommandContext<ServerCommandSource>& context);
    static i32 executeIfBlock(CommandContext<ServerCommandSource>& context);
    static i32 executeUnlessBlock(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
