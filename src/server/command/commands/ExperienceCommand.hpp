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
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /experience 命令 (别名: /xp)
 *
 * 用法：
 * - /experience add <player> <amount> [points|levels] - 添加经验
 * - /experience set <player> <amount> [points|levels] - 设置经验
 * - /experience query <player> [points|levels] - 查询经验
 *
 * 权限等级：2
 */
class ExperienceCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 添加经验点
     */
    static i32 addPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 添加经验等级
     */
    static i32 addLevels(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置经验点
     */
    static i32 setPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置经验等级
     */
    static i32 setLevels(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询经验点
     */
    static i32 queryPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询经验等级
     */
    static i32 queryLevels(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
