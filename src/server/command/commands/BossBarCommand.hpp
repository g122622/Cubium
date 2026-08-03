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

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>

namespace mc {
namespace command {

/**
 * @brief BossBarCommand - Boss 栏管理
 *
 * 用法: /bossbar <add|remove|list|set|get> ...
 * 权限: 2 (游戏管理员)
 *
 * 子命令:
 * - /bossbar add <id> <name> - 创建新的 Boss 栏
 * - /bossbar remove <id> - 移除 Boss 栏
 * - /bossbar list - 列出所有 Boss 栏
 * - /bossbar set <id> name <name> - 设置名称
 * - /bossbar set <id> color <color> - 设置颜色
 * - /bossbar set <id> style <style> - 设置样式
 * - /bossbar set <id> value <value> - 设置当前值
 * - /bossbar set <id> max <max> - 设置最大值
 * - /bossbar set <id> visible <visible> - 设置可见性
 * - /bossbar set <id> players [<targets>] - 设置可见玩家
 * - /bossbar get <id> value - 获取当前值
 * - /bossbar get <id> max - 获取最大值
 * - /bossbar get <id> visible - 获取可见性
 * - /bossbar get <id> players - 获取可见玩家列表
 */
class BossBarCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _addBossBar(CommandContext<ServerCommandSource>& context);
    static i32 _removeBossBar(CommandContext<ServerCommandSource>& context);
    static i32 _listBossBars(CommandContext<ServerCommandSource>& context);
    static i32 _setName(CommandContext<ServerCommandSource>& context);
    static i32 _setColor(CommandContext<ServerCommandSource>& context, const std::string& colorStr);
    static i32 _setStyle(CommandContext<ServerCommandSource>& context, const std::string& styleStr);
    static i32 _setValue(CommandContext<ServerCommandSource>& context);
    static i32 _setMax(CommandContext<ServerCommandSource>& context);
    static i32 _setVisible(CommandContext<ServerCommandSource>& context);
    static i32 _setPlayers(CommandContext<ServerCommandSource>& context);
    static i32 _getValue(CommandContext<ServerCommandSource>& context);
    static i32 _getMax(CommandContext<ServerCommandSource>& context);
    static i32 _getVisible(CommandContext<ServerCommandSource>& context);
    static i32 _getPlayers(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
