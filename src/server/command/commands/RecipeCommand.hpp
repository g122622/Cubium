#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief RecipeCommand - 配方管理
 *
 * 用法: /recipe <give|take> <targets> <recipe|*>
 * 权限: 2 (游戏管理员)
 */
class RecipeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 giveRecipe(CommandContext<ServerCommandSource>& context);
    static i32 takeRecipe(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
