#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /summon 命令
 *
 * 用法：
 * - /summon <entity> - 在执行者位置生成实体
 * - /summon <entity> <pos> - 在指定位置生成实体
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 的 SummonCommand
 */
class SummonCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 summonEntity(CommandContext<ServerCommandSource>& context);
    static i32 summonEntityAtPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
