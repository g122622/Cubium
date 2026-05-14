#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief TagCommand - 实体标签管理
 *
 * 用法: /tag <targets> <add|remove|list> [tag]
 * 权限: 2 (游戏管理员)
 */
class TagCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 addTag(CommandContext<ServerCommandSource>& context);
    static i32 removeTag(CommandContext<ServerCommandSource>& context);
    static i32 listTags(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
