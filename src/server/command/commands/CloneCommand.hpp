#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief 克隆模式
 *
 * 参考 MC 1.16.5 CloneCommand.Mode
 */
enum class CloneMode {
    Normal, // 正常模式，不允许重叠
    Force,  // 强制模式，允许重叠
    Move    // 移动模式，复制后清空源区域
};

/**
 * @brief 方块过滤模式
 *
 * 参考 MC 1.16.5 CloneCommand 的 filtered/masked/replace
 */
enum class FilterMode {
    Replace, // 复制所有方块
    Masked,  // 只复制非空气方块
    Filtered // 只复制匹配指定方块的方块
};

/**
 * @brief CloneCommand - 复制方块区域
 *
 * 用法: /clone <begin> <end> <destination> [replace|masked|filtered] [normal|force|move]
 * 权限: 2 (游戏管理员)
 *
 * 模式说明:
 * - replace: 复制所有方块（默认）
 * - masked: 只复制非空气方块
 * - filtered: 只复制匹配指定方块的方块
 *
 * 执行模式:
 * - normal: 不允许源区域和目标区域重叠（默认）
 * - force: 允许重叠
 * - move: 复制后清空源区域
 *
 * 参考: MC 1.16.5 CloneCommand
 */
class CloneCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 cloneBlocks(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 默认克隆操作（replace + normal）
     */
    static i32 doCloneDefault(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 执行克隆操作
     * @param context 命令上下文
     * @param filterMode 方块过滤模式
     * @param cloneMode 克隆模式
     */
    static i32 doCloneStatic(CommandContext<ServerCommandSource>& context, FilterMode filterMode, CloneMode cloneMode);

    /**
     * @brief 执行过滤克隆操作
     * @param context 命令上下文
     * @param cloneMode 克隆模式
     */
    static i32 doCloneFilteredStatic(CommandContext<ServerCommandSource>& context, CloneMode cloneMode);
};

} // namespace command
} // namespace mc
