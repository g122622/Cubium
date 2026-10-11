#pragma once

#include "common/core/Types.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include <unordered_map>

namespace mc::server {
class IServer;
class ServerWorld;

/** @brief 每维度导航点门面，管理每个接收玩家的已发送快照。 */
class WaypointManager {
public:
    /** @brief 根据生物属性、可见性及玩家视距增量同步导航点。 */
    void tick(IServer& server, ServerWorld& world);
    /** @brief 重配置前清理接收状态，使下一 tick 重发完整导航点。 */
    void clearReceiver(IServer& server, PlayerId playerId);
    /** @brief 离场前清除玩家接收状态及其作为发送源的导航点。 */
    void removePlayer(IServer& server, PlayerId playerId);

private:
    using SnapshotMap = std::unordered_map<EntityInstanceId, mc::network::ir::play::Waypoint>;
    std::unordered_map<PlayerId, SnapshotMap> m_sent;
};
} // namespace mc::server
