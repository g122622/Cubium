#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /tellraw 命令
 *
 * 向玩家发送原始 JSON 消息。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /tellraw <player> <json message>
 */
class TellRawCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 发送原始消息
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 sendRawMessage(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
