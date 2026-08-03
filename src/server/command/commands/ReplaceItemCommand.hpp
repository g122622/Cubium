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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY CLAIM, DAMAGES OR OTHER
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

namespace mc {
namespace command {

/**
 * @brief ReplaceItemCommand - 替换物品命令
 *
 * 用法: /replaceitem entity <targets> <slot> <item> [count]
 *       /replaceitem block <pos> <slot> <item> [count]
 * 权限: 2 (游戏管理员)
 *
 * 支持两种目标类型：
 * - entity: 替换实体（玩家）物品栏中的物品
 * - block: 替换方块容器（如箱子）中的物品
 *
 * 槽位名称参考 ItemSlotArgumentType 支持的格式，
 * 如 weapon.mainhand, armor.head, container.5, hotbar.0 等。
 *
 * 注: MC 1.17+ 已将 /replaceitem 重命名为 /item replace with，
 *     本项目保留 /replaceitem 名称以兼容旧版命令习惯。
 */
class ReplaceItemCommand {
public:
    /**
     * @brief 注册命令到调度器
     * @param dispatcher 命令调度器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 替换实体物品栏中的物品
     * @param context 命令上下文
     * @return 成功替换的实体数量
     */
    static i32 _replaceEntityItem(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 替换方块容器中的物品
     * @param context 命令上下文
     * @return 成功替换的数量（0或1）
     */
    static i32 _replaceBlockItem(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
