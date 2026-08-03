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

namespace mc {
namespace command {

/**
 * @brief `/time` 命令。
 *
 * 当前实现覆盖项目现阶段最常用、也最贴近 Java 版原版体验的时间控制语义：
 * - `/time set <value>`
 * - `/time set <day|noon|night|midnight>`
 * - `/time add <value>`
 * - `/time query <day|daytime|gametime>`
 */
class TimeCommand {
public:
    /**
     * @brief 将命令注册到分发器。
     *
     * @param dispatcher 命令分发器。
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 设置世界昼夜时间。
     *
     * @param context 命令上下文。
     * @return 设置后的昼夜时间。
     */
    static i32 _setTime(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 在当前昼夜时间基础上增加 tick。
     *
     * @param context 命令上下文。
     * @return 增加后的昼夜时间。
     */
    static i32 _addTime(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询当前时间信息。
     *
     * @param context 命令上下文。
     * @return 查询值。
     */
    static i32 _queryTime(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
