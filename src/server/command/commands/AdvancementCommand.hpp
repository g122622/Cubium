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
#include "common/resource/ResourceLocation.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief 授予/撤销成就的遍历模式
 */
enum class GrantMode : u8 {
    Only,       ///< 仅操作指定成就
    Everything, ///< 操作所有成就
    From,       ///< 操作指定成就及其所有子成就
    Through,    ///< 操作从根到指定成就的路径
    Until       ///< 操作指定成就及其所有父成就
};

/**
 * @brief AdvancementCommand - 进度管理
 *
 * 用法:
 *   /advancement grant <targets> everything
 *   /advancement grant <targets> only <advancement>
 *   /advancement grant <targets> from <advancement>
 *   /advancement grant <targets> through <advancement>
 *   /advancement grant <targets> until <advancement>
 *   /advancement revoke <targets> everything
 *   /advancement revoke <targets> only <advancement>
 *   /advancement revoke <targets> from <advancement>
 *   /advancement revoke <targets> through <advancement>
 *   /advancement revoke <targets> until <advancement>
 *   /advancement test <targets> <advancement>
 *
 * 权限: 2 (游戏管理员)
 */
class AdvancementCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 授予成就
     * @param context 命令上下文
     * @param mode 授予模式
     * @return 受影响的玩家数量
     */
    static i32 _grantAdvancement(CommandContext<ServerCommandSource>& context, GrantMode mode);

    /**
     * @brief 撤销成就
     * @param context 命令上下文
     * @param mode 撤销模式
     * @return 受影响的玩家数量
     */
    static i32 _revokeAdvancement(CommandContext<ServerCommandSource>& context, GrantMode mode);

    /**
     * @brief 测试成就
     * @param context 命令上下文
     * @return 完成成就的玩家数量
     */
    static i32 _testAdvancement(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
