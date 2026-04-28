#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /fill 命令
 *
 * 用法：
 * - /fill <from> <to> <block> - 填充区域
 * - /fill <from> <to> <block> destroy - 破坏并填充
 * - /fill <from> <to> <block> hollow - 空心填充（仅外壳）
 * - /fill <from> <to> <block> keep - 仅替换空气
 * - /fill <from> <to> <block> outline - 轮廓填充（仅外壳）
 * - /fill <from> <to> <block> replace [filter] - 替换指定方块
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 的 FillCommand
 */
class FillCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 fill(CommandContext<ServerCommandSource>& context);
    static i32 fillDestroy(CommandContext<ServerCommandSource>& context);
    static i32 fillHollow(CommandContext<ServerCommandSource>& context);
    static i32 fillKeep(CommandContext<ServerCommandSource>& context);
    static i32 fillOutline(CommandContext<ServerCommandSource>& context);
    static i32 fillReplace(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
