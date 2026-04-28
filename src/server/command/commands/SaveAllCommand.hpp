#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /save-all 命令
 *
 * 用法：
 * - /save-all - 保存所有数据
 * - /save-all flush - 强制刷新并保存
 *
 * 权限等级：4
 *
 * 参考 MC 1.16.5 的 SaveAllCommand
 */
class SaveAllCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 saveAll(CommandContext<ServerCommandSource>& context);
    static i32 saveAllFlush(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
