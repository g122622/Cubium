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
 * @brief /weather 命令
 *
 * 用法：
 * - /weather clear [duration] - 设置晴天
 * - /weather rain [duration] - 设置降雨
 * - /weather thunder [duration] - 设置雷暴
 * - /weather query - 查询当前天气
 *
 * 权限等级：2
 *
 * duration 单位为 ticks (20 ticks = 1秒)
 * 如果不指定 duration，默认为 6000 ticks (5分钟)
 */
class WeatherCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /// 设置晴天
    static i32 _setClear(CommandContext<ServerCommandSource>& context);
    /// 设置降雨
    static i32 _setRain(CommandContext<ServerCommandSource>& context);
    /// 设置雷暴
    static i32 _setThunder(CommandContext<ServerCommandSource>& context);
    /// 查询当前天气
    static i32 _query(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
