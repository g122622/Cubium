#pragma once

#include "Packet.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../core/Types.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 世界出生点包 (服务端 -> 客户端)
 *
 * 告诉客户端世界出生点的位置。客户端使用此位置来确定指南针指向。
 *
 * 发送时机：
 * - 玩家登录时
 * - 执行 /setworldspawn 命令后
 *
 * 参考 MC 1.16.5 SSpawnPositionPacket
 */
class SpawnPositionPacket : public Packet {
public:
    SpawnPositionPacket();
    explicit SpawnPositionPacket(const BlockPos& pos);
    ~SpawnPositionPacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取出生点位置
     */
    [[nodiscard]] const BlockPos& position() const { return m_position; }
    void setPosition(const BlockPos& pos) { m_position = pos; }

private:
    BlockPos m_position;
};

} // namespace mc::network
