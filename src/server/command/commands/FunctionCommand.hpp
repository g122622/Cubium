/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace command {

/**
 * @brief FunctionCommand - 执行函数文件或函数标签
 *
 * 用法:
 *   /function <name>
 *   /function <name> <arguments>
 *   /function <name> with <entity|block|storage> [path]
 *   /function #<tag>
 *   /function #<tag> <arguments>
 *   /function #<tag> with <entity|block|storage> [path]
 *
 * 权限: 2 (游戏管理员)
 *
 * 引用方式：
 * - 直接函数引用: /function minecraft:foo/bar
 * - 标签引用: /function #minecraft:tick（执行标签中的所有函数）
 *
 * 参数形式：
 * - 无参数：普通函数直接执行；宏函数若缺参数将失败
 * - <arguments>：内联 SNBT 复合标签（如 {key:value}），传给宏函数实例化
 * - with <entity <target>>：从实体 NBT 取参数
 * - with <block <pos>>：从方块实体 NBT 取参数
 * - with <storage <id>>：从命令存储取参数
 * - 上述 with 形式可附加 [path] 限定 NBT 路径（路径必须指向 CompoundTag）
 *
 * 对应 MC 1.21.11 的 net.minecraft.server.commands.FunctionCommand。
 */
class FunctionCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // ========== 子命令处理器 ==========

    /** @brief /function <name> [无参数] */
    static i32 _runFunction(CommandContext<ServerCommandSource>& context);

    /** @brief /function <name> <arguments> */
    static i32 _runFunctionWithArguments(CommandContext<ServerCommandSource>& context);

    /** @brief /function <name> with entity <target> [path] */
    static i32 _runWithEntity(CommandContext<ServerCommandSource>& context);

    /** @brief /function <name> with block <pos> [path] */
    static i32 _runWithBlock(CommandContext<ServerCommandSource>& context);

    /** @brief /function <name> with storage <id> [path] */
    static i32 _runWithStorage(CommandContext<ServerCommandSource>& context);

    // ========== 辅助 ==========

    /**
     * @brief 执行函数集合（单个函数或标签下所有函数）
     *
     * @param context 命令上下文
     * @param arguments 实参 CompoundTag（可为 nullptr）
     * @return 命令返回值（成功执行的命令数）
     */
    static i32 _executeFunctions(
        CommandContext<ServerCommandSource>& context, const nbt::tags::compound_tag* arguments);
};

} // namespace command
} // namespace mc
