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
#include <memory>
#include <string>

namespace mc {
namespace command {

/**
 * @brief /execute 命令
 *
 * 用法：
 * - /execute as <entity> run <command> - 以指定实体身份执行命令
 * - /execute at <entity> run <command> - 在指定实体位置执行命令
 * - /execute in <dimension> run <command> - 在指定维度执行命令
 * - /execute positioned <pos> run <command> - 在指定位置执行命令
 * - /execute run <command> - 直接执行命令
 * - /execute if block <pos> <block> run <command> - 如果指定位置是指定方块则执行
 * - /execute unless block <pos> <block> run <command> - 如果指定位置不是指定方块则执行
 * - /execute align <axes> run <command> - 对齐坐标后执行命令（待实现）
 * - /execute facing <pos/entity> run <command> - 朝向指定方向执行命令（待实现）
 * - /execute rotated <rot/as> run <command> - 旋转后执行命令（待实现）
 * - /execute anchored <anchor> run <command> - 锚定后执行命令（待实现）
 *
 * 权限等级：2
 */
class ExecuteCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // ========== 嵌套命令执行 ==========

    /**
     * @brief 执行嵌套命令
     * @param source 修改后的命令源
     * @param command 要执行的命令字符串
     * @return 命令执行结果（成功返回结果值，失败返回 0）
     */
    static i32 _executeNestedCommand(ServerCommandSource& source, const std::string& command);

    // ========== 子命令执行 ==========

    static i32 _executeAs(CommandContext<ServerCommandSource>& context);
    static i32 _executeAt(CommandContext<ServerCommandSource>& context);
    static i32 _executeIn(CommandContext<ServerCommandSource>& context);
    static i32 _executePositioned(CommandContext<ServerCommandSource>& context);
    static i32 _executeRun(CommandContext<ServerCommandSource>& context);
    static i32 _executeIfBlock(CommandContext<ServerCommandSource>& context);
    static i32 _executeUnlessBlock(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
