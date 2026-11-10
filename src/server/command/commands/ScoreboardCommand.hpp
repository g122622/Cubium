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
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/util/text/numbers/NumberFormat.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <optional>

namespace mc {
namespace command {

/**
 * @brief ScoreboardCommand - 记分板管理
 *
 * 用法: /scoreboard <objectives|players|teams> ...
 * 权限: 2 (游戏管理员)
 */
class ScoreboardCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _addObjective(CommandContext<ServerCommandSource>& context);
    static i32 _removeObjective(CommandContext<ServerCommandSource>& context);
    static i32 _listObjectives(CommandContext<ServerCommandSource>& context);
    static i32 _setScore(CommandContext<ServerCommandSource>& context);
    static i32 _addScore(CommandContext<ServerCommandSource>& context);
    static i32 _removeScore(CommandContext<ServerCommandSource>& context);
    static i32 _resetScore(CommandContext<ServerCommandSource>& context);
    static i32 _getScore(CommandContext<ServerCommandSource>& context);
    static i32 _enableTrigger(CommandContext<ServerCommandSource>& context);
    static i32 _listPlayers(CommandContext<ServerCommandSource>& context);
    static i32 _listAllPlayers(CommandContext<ServerCommandSource>& context);
    static i32 _modifyObjectiveDisplayName(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置目标的显示名是否随分数自动更新
     *
     * 对应 /scoreboard objectives modify <objective> displayautoupdate <bool>。
     */
    static i32 _modifyObjectiveDisplayAutoUpdate(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置目标的分数渲染类型
     *
     * 对应原版 `commands.scoreboard.objectives.modify.rendertype`：仅在类型发生变化时写入并
     * 回报成功，类型不变则不产生任何输出。
     *
     * @param context 命令上下文
     * @param renderType 目标渲染类型（hearts / integer）
     */
    static i32 _setObjectiveRenderType(CommandContext<ServerCommandSource>& context, scoreboard::RenderType renderType);

    static i32 _setDisplaySlot(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置目标的数字格式
     *
     * 对应 `/scoreboard objectives modify <objective> numberformat [blank|styled <style>|fixed <contents>]`。
     * 不给子节点时传 `std::nullopt`（清除自定义格式，回退到客户端默认格式），其余情况传具体格式。
     *
     * @param context 命令上下文
     * @param numberFormat 数字格式；nullopt 表示清除
     */
    static i32 _modifyObjectiveNumberFormat(
        CommandContext<ServerCommandSource>& context, std::optional<text::numbers::NumberFormat> numberFormat);

    /**
     * @brief 设置分数持有者的数字格式覆盖
     *
     * 对应 `/scoreboard players display numberformat <targets> <objective> [blank|styled|fixed]`。
     * `targets` 支持逗号分隔的多个持有者与 `*`（所有已记录分数的持有者）。
     *
     * @param context 命令上下文
     * @param numberFormat 覆盖格式；nullopt 表示清除覆盖、回退到目标格式
     */
    static i32 _modifyPlayerNumberFormat(
        CommandContext<ServerCommandSource>& context, std::optional<text::numbers::NumberFormat> numberFormat);

    /**
     * @brief 设置或清除分数持有者的自定义显示名
     *
     * 对应 `/scoreboard players display name <targets> <objective> [<name>]`。
     * `targets` 支持逗号分隔的多个持有者与 `*`（所有已记录分数的持有者）。
     *
     * @param context 命令上下文
     * @param displayJson 显示名的组件 JSON 文本；nullopt 表示清除覆盖、回退到持有者名字
     */
    static i32 _modifyPlayerDisplayName(
        CommandContext<ServerCommandSource>& context, const std::optional<std::string>& displayJson);

    static i32 _operateScore(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
