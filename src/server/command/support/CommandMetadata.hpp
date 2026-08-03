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

#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::command::support {

/**
 * @brief 命令元数据构造辅助函数。
 *
 * @param description 命令描述
 * @param usage 命令用法
 * @param permissionLevel 命令权限等级
 * @param aliases 命令别名列表
 * @param implemented 命令是否已完整实现
 * @return 适用于命令节点的元数据对象
 */
[[nodiscard]] inline CommandNode<ServerCommandSource>::Metadata makeMetadata(std::string description,
    std::string usage,
    i32 permissionLevel,
    std::vector<std::string> aliases = {},
    bool implemented = true)
{
    CommandNode<ServerCommandSource>::Metadata metadata;
    metadata.description = std::move(description);
    metadata.usage = std::move(usage);
    metadata.permissionLevel = permissionLevel;
    metadata.implemented = implemented;
    metadata.aliases = std::move(aliases);
    return metadata;
}

/**
 * @brief 将命令元数据应用到字面量节点。
 *
 * 此函数将元数据信息应用到指定的命令节点上。
 * 该函数不会抛出异常。
 *
 * @param node 目标命令节点
 * @param metadata 命令元数据
 */
inline void applyMetadata(const std::shared_ptr<LiteralCommandNode<ServerCommandSource>>& node,
    const CommandNode<ServerCommandSource>::Metadata& metadata) noexcept
{
    node->setMetadataInfo(metadata);
}

} // namespace mc::command::support
