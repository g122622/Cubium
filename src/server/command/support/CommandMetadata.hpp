#pragma once

#include "common/command/CommandNode.hpp"
#include "server/command/ServerCommandSource.hpp"

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
 * @param node 目标命令节点
 * @param metadata 命令元数据
 */
inline void applyMetadata(const std::shared_ptr<LiteralCommandNode<ServerCommandSource>>& node,
    const CommandNode<ServerCommandSource>::Metadata& metadata)
{
    node->setMetadataInfo(metadata);
}

} // namespace mc::command::support
