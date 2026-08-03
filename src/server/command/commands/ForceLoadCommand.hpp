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
#include <string_view>

namespace mc {
namespace command {

/**
 * @brief ForceLoadCommand - 强制加载区块命令
 *
 * 用法:
 *   /forceload add <from> [to]    - 添加强制加载区块
 *   /forceload remove <from> [to] - 移除强制加载区块
 *   /forceload remove all         - 移除当前维度所有强制加载区块
 *   /forceload query [<pos>]      - 查询单个区块或列出所有强制加载区块
 *
 * 权限: 2 (游戏管理员)
 *
 * 限制:
 *   - 单次操作最多 256 个区块
 *   - 坐标必须在世界边界内 [-WORLD_BORDER, WORLD_BORDER)
 *
 * 注意:
 *   - 强制加载区块在服务器重启后会丢失（当前未实现持久化）
 *   - 每个维度独立管理强制加载区块
 */
class ForceLoadCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 获取维度名称
     * @param dimensionId 维度ID
     * @return 维度名称字符串
     */
    static std::string_view _getDimensionName(DimensionId dimensionId);

    /**
     * @brief 添加强制加载区块
     * @return 成功添加的区块数量
     */
    static i32 _addForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 移除强制加载区块
     * @return 成功移除的区块数量
     */
    static i32 _removeForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询单个区块是否被强制加载
     * @return 1 如果被强制加载，0 否则
     */
    static i32 _queryForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 列出当前维度所有强制加载区块
     * @return 强制加载区块的数量
     */
    static i32 _listAllForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 移除当前维度所有强制加载区块
     * @return 成功移除的区块数量
     */
    static i32 _removeAllForceLoad(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
