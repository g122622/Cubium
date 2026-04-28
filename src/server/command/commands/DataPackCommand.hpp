#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief DataPackCommand - 数据包管理
 *
 * 用法: /datapack <enable|disable|list> [name]
 * 权限: 2 (游戏管理员)
 */
class DataPackCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 enableDataPack(CommandContext<ServerCommandSource>& context);
    static i32 disableDataPack(CommandContext<ServerCommandSource>& context);
    static i32 listDataPacks(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
