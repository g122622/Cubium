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
 * The copyright notice and this permission notice shall be included in all
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
#include "common/command/arguments/NbtPath.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <functional>
#include <memory>
#include <string>

namespace mc {

// 前向声明
class Entity;
class IWorld;
class BlockPos;

namespace command {

// 前向声明
class IDataAccessor;

/**
 * @brief DataCommand - 获取或修改方块/实体/存储的 NBT 数据
 *
 * 用法:
 * - /data get block <pos> [<path>] [<scale>]
 * - /data get entity <target> [<path>] [<scale>]
 * - /data get storage <id> [<path>] [<scale>]
 * - /data set block <pos> <path> <value>
 * - /data set entity <target> <path> <value>
 * - /data set storage <id> <path> <value>
 * - /data merge block <pos> <nbt>
 * - /data merge entity <target> <nbt>
 * - /data merge storage <id> <nbt>
 * - /data remove block <pos> <path>
 * - /data remove entity <target> <path>
 * - /data remove storage <id> <path>
 *
 * 权限: 2 (游戏管理员)
 */
class DataCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // ========== 具体目标类型的处理函数 ==========

    /**
     * @brief 获取方块实体的 NBT 数据
     */
    static i32 _getBlock(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 获取实体的 NBT 数据
     */
    static i32 _getEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 获取存储的 NBT 数据
     */
    static i32 _getStorage(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置方块实体的 NBT 数据
     */
    static i32 _setBlock(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置实体的 NBT 数据
     */
    static i32 _setEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置存储的 NBT 数据
     */
    static i32 _setStorage(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 合并方块实体的 NBT 数据
     */
    static i32 _mergeBlock(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 合并实体的 NBT 数据
     */
    static i32 _mergeEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 合并存储的 NBT 数据
     */
    static i32 _mergeStorage(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 删除方块实体的 NBT 数据路径
     */
    static i32 _removeBlock(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 删除实体的 NBT 数据路径
     */
    static i32 _removeEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 删除存储的 NBT 数据路径
     */
    static i32 _removeStorage(CommandContext<ServerCommandSource>& context);

    // ========== 辅助函数 ==========

    /**
     * @brief 获取 NBT 标签的单值结果
     * @param tag NBT 标签
     * @return 对于数值返回其值，对于集合返回大小，对于字符串返回长度
     */
    static i32 _getSingleResult(const nbt::tags::tag& tag);

    /**
     * @brief 缩放数值类型标签
     * @param tag NBT 标签
     * @param scale 缩放因子
     * @return 缩放后的整数值
     */
    static i32 _scaleValue(const nbt::tags::tag& tag, double scale);

    /**
     * @brief 发送错误消息
     */
    static void _sendError(ServerCommandSource& source, const std::string& message);
};

} // namespace command
} // namespace mc
