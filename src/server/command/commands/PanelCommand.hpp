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

namespace mc::command {

/**
 * @brief `/panel` 命令
 *
 * 为交互式终端（`--interactive`）的右侧信息面板刷新"世界状态摘要"：
 * 各维度的区块状态（已加载 / 可 tick / 已后处理）、方块分布与群系分布。
 *
 * 不使用原版命令名（原版无 `/panel`），因此不参与命令树差分对比。
 *
 * 子命令：
 * - `/panel`              等价于 `/panel refresh`
 * - `/panel refresh`      重新扫描全部维度并刷新面板
 * - `/panel block <x> <y> <z>`        查看指定方块的方块状态与方块实体 NBT
 * - `/panel places [count]`           从不重复的方块坐标中随机抽取若干处，逐个打印方块状态
 * - `/panel biome <x> <y> <z>`        查看指定坐标的运行时群系
 */
class PanelCommand {
public:
    /**
     * @brief 把 `/panel` 及其子命令注册到分发器
     * @param dispatcher 目标分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /// `/panel` / `/panel refresh`
    static i32 _refresh(CommandContext<ServerCommandSource>& context);
    /// `/panel block <pos>`
    static i32 _describeBlock(CommandContext<ServerCommandSource>& context);
    /// `/panel places [<count>]`
    static i32 _describePlaces(CommandContext<ServerCommandSource>& context);
    /// `/panel biome <pos>`
    static i32 _describeBiome(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
