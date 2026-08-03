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
 * @brief /clear 命令
 *
 * 用法：
 * - /clear - 清空自己的背包
 * - /clear <player> - 清空指定玩家的背包
 * - /clear <player> <item> - 清空指定玩家匹配的物品
 * - /clear <player> <item> <maxCount> - 清空指定玩家匹配的物品，最多清除数量
 *
 * <item> 参数支持物品谓词语法（ItemPredicateArgumentType）：
 * - 物品ID：minecraft:stone、stone — 精确匹配指定物品
 * - 物品标签：#minecraft:logs — 匹配标签中的所有物品
 * - 通配符：* — 匹配任意物品
 *
 * 权限等级：2
 */
class ClearCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _clearSelf(CommandContext<ServerCommandSource>& context);
    static i32 _clearPlayer(CommandContext<ServerCommandSource>& context);
    static i32 _clearPlayerItem(CommandContext<ServerCommandSource>& context);
    static i32 _clearPlayerItemCount(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
