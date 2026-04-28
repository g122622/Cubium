#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /save-off 命令
 *
 * 禁用服务器自动保存功能。
 * 权限等级: 4 (OP)
 */
class SaveOffCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 执行禁用自动保存
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 disableAutoSave(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
