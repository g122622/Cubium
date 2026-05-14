#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <memory>

namespace mc {
namespace command {

/**
 * @brief `/time` 命令。
 *
 * 当前实现覆盖项目现阶段最常用、也最贴近 Java 版原版体验的时间控制语义：
 * - `/time set <value>`
 * - `/time set <day|noon|night|midnight>`
 * - `/time add <value>`
 * - `/time query <day|daytime|gametime>`
 */
class TimeCommand {
public:
    /**
     * @brief 将命令注册到分发器。
     *
     * @param dispatcher 命令分发器。
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 设置世界昼夜时间。
     *
     * @param context 命令上下文。
     * @return 设置后的昼夜时间。
     */
    static i32 setTime(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 在当前昼夜时间基础上增加 tick。
     *
     * @param context 命令上下文。
     * @return 增加后的昼夜时间。
     */
    static i32 addTime(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 查询当前时间信息。
     *
     * @param context 命令上下文。
     * @return 查询值。
     */
    static i32 queryTime(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
