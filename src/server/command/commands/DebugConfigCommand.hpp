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

namespace mc::command {

/**
 * @brief /debugconfig 命令。
 *
 * 对齐 vanilla DebugConfigCommand：
 *   /debugconfig config <player>          —— 让指定玩家切回 Configuration 阶段（重配置）
 *   /debugconfig unconfig <uuid>          —— 让处于配置阶段的玩家重新加入世界
 *   /debugconfig dialog <uuid> <dialog>   —— 向配置阶段玩家展示对话框
 *
 * 本项目实现 config 分支（服务端发起 Play→Configuration 重配置），与 vanilla
 * `ServerGamePacketListenerImpl#switchToConfig` 对齐。unconfig/dialog 依赖「按 UUID 查找
 * 处于配置阶段的连接」这一独立连接查询能力，见 _unconfig/_dialog 的 TODO。
 */
class DebugConfigCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _config(CommandContext<ServerCommandSource>& context);
    static i32 _unconfig(CommandContext<ServerCommandSource>& context);
    static i32 _dialog(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
