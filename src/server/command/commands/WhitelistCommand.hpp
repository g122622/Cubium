/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>
#include <string>

namespace mc {
namespace command {

/**
 * @brief /whitelist 命令
 *
 * 用法：
 * - /whitelist on - 开启白名单
 * - /whitelist off - 关闭白名单
 * - /whitelist list - 列出白名单玩家
 * - /whitelist add <player> - 添加玩家到白名单
 * - /whitelist remove <player> - 从白名单移除玩家
 * - /whitelist reload - 重新加载白名单
 *
 * 权限等级：3（需要管理员权限）
 *
 * 参考 MC 1.16.5 的 WhitelistCommand
 */
class WhitelistCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 whitelistOn(CommandContext<ServerCommandSource>& context);
    static i32 whitelistOff(CommandContext<ServerCommandSource>& context);
    static i32 whitelistList(CommandContext<ServerCommandSource>& context);
    static i32 whitelistAdd(CommandContext<ServerCommandSource>& context);
    static i32 whitelistRemove(CommandContext<ServerCommandSource>& context);
    static i32 whitelistReload(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 踢出不在白名单的玩家
     * @param source 命令源
     */
    static void kickNonWhitelistedPlayers(ServerCommandSource& source);

    /**
     * @brief 从玩家名生成临时 UUID
     * @param name 玩家名
     * @return 临时 UUID
     * @note 实际服务器应从 Mojang API 获取真实 UUID
     */
    static std::string generateUuidFromName(const std::string& name);
};

} // namespace command
} // namespace mc
