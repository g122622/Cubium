#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /msg (message) 命令
 *
 * 向指定玩家发送私聊消息。
 * 别名: /tell, /w (whisper)
 * 权限等级: 0 (所有玩家)
 *
 * 用法:
 * - /msg <player> <message>
 * - /tell <player> <message>
 * - /w <player> <message>
 */
class MessageCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 发送私聊消息
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 sendMessage(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
