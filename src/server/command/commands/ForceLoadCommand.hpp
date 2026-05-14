#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief ForceLoadCommand - 强制加载区块命令
 *
 * 参考 MC 1.16.5 的 /forceload 命令实现。
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
 *   - 坐标必须在世界边界内 [-30000000, 30000000)
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
     * @brief 添加强制加载区块
     * @return 成功添加的区块数量
     */
    static i32 addForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 移除强制加载区块
     * @return 成功移除的区块数量
     */
    static i32 removeForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询单个区块是否被强制加载
     * @return 1 如果被强制加载，0 否则
     */
    static i32 queryForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 列出当前维度所有强制加载区块
     * @return 强制加载区块的数量
     */
    static i32 listAllForceLoad(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 移除当前维度所有强制加载区块
     * @return 成功移除的区块数量
     */
    static i32 removeAllForceLoad(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
