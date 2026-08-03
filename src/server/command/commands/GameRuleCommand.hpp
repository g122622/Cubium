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

/**
 * @file GameRuleCommand.hpp
 * @brief /gamerule 命令
 *
 * 用法：
 *   /gamerule <rule>           - 查询规则值
 *   /gamerule <rule> <value>   - 设置规则值
 */

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>
#include <vector>

namespace mc::command {

/**
 * @brief GameRule 命令
 *
 * 用于查询和设置游戏规则。
 */
class GameRuleCommand {
public:
    /**
     * @brief 注册命令到调度器
     * @param dispatcher 命令调度器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 执行查询规则
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _executeQuery(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 执行设置规则
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _executeSet(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 获取所有规则名称
     * @return 规则名称列表
     */
    static std::vector<std::string> _getAllRuleNames();
};

} // namespace mc::command
