#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief DataCommand - 获取或修改方块/实体/存储的 NBT 数据
 *
 * 用法: /data <get|set|merge|remove> <target> [<path>] [<value>]
 * 权限: 2 (游戏管理员)
 */
class DataCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 getData(CommandContext<ServerCommandSource>& context);
    static i32 setData(CommandContext<ServerCommandSource>& context);
    static i32 mergeData(CommandContext<ServerCommandSource>& context);
    static i32 removeData(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
