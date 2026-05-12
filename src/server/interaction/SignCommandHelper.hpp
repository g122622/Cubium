#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

// 前向声明
class ServerPlayer;

namespace blockentity {
class SignEntity;
}

namespace server {

/**
 * @brief 告示牌命令执行辅助类
 *
 * 提供服务端告示牌命令执行功能。
 * 由于 SignEntity 位于 mc_common 库中，无法直接访问服务端的命令系统，
 * 因此实际的命令执行逻辑放在此服务端辅助类中。
 *
 * 参考 MC 1.16.5 SignTileEntity.executeCommand()
 */
class SignCommandHelper {
public:
    /**
     * @brief 执行告示牌上的命令
     *
     * 遍历告示牌所有行的文本，查找并执行点击事件中的命令。
     *
     * @param signEntity 告示牌实体
     * @param player 执行命令的服务端玩家
     * @return 如果成功执行了至少一个命令返回 true
     */
    static bool executeSignCommands(
        blockentity::SignEntity& signEntity,
        mc::ServerPlayer& player);

private:
    /**
     * @brief 执行单个命令
     *
     * @param command 命令字符串（可带或不带 '/' 前缀）
     * @param player 执行命令的玩家
     * @param signPos 告示牌位置（用于命令源位置）
     * @return 如果命令执行成功返回 true
     */
    static bool executeCommand(
        const std::string& command,
        mc::ServerPlayer& player,
        const BlockPos& signPos);
};

} // namespace server
} // namespace mc
