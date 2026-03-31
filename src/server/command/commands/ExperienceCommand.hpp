#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /experience 命令 (别名: /xp)
 *
 * 用法：
 * - /experience add <player> <amount> [points|levels] - 添加经验
 * - /experience set <player> <amount> [points|levels] - 设置经验
 * - /experience query <player> [points|levels] - 查询经验
 *
 * 权限等级：2
 *
 * 参考 MC 1.16.5 ExperienceCommand
 */
class ExperienceCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 添加经验点
     */
    static i32 addPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 添加经验等级
     */
    static i32 addLevels(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置经验点
     */
    static i32 setPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置经验等级
     */
    static i32 setLevels(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询经验点
     */
    static i32 queryPoints(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询经验等级
     */
    static i32 queryLevels(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
