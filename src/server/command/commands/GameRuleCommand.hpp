/**
 * @file GameRuleCommand.hpp
 * @brief /gamerule 命令
 *
 * 参考 MC 1.16.5: net.minecraft.server.command.GameRuleCommand
 *
 * 用法：
 *   /gamerule <rule>           - 查询规则值
 *   /gamerule <rule> <value>   - 设置规则值
 */

#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief GameRule 命令
 *
 * 用于查询和设置游戏规则。
 */
class GameRuleCommand {
public:
    /**
     * @brief 注册命令到调度器
     * @param dispatcher 命令调度器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 执行查询规则
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 executeQuery(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 执行设置规则
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 executeSet(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 获取所有规则名称
     * @return 规则名称列表
     */
    static std::vector<std::string> getAllRuleNames();
};

} // namespace mc::command
