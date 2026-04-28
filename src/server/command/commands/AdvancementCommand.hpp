#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief AdvancementCommand - 进度管理
 *
 * 用法: /advancement <grant|revoke|test> <targets> <everything|only|from|through|until>
 * 权限: 2 (游戏管理员)
 */
class AdvancementCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 grantAdvancement(CommandContext<ServerCommandSource>& context);
    static i32 revokeAdvancement(CommandContext<ServerCommandSource>& context);
    static i32 testAdvancement(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
