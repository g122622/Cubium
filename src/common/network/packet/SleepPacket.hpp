#pragma once

#include "../../world/block/BlockPos.hpp"
#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include <optional>

namespace mc::network {

/**
 * @brief 睡眠状态同步包 (S->C)
 *
 * 服务器发送给客户端，同步玩家的睡眠状态。
 * 当玩家进入睡眠时，发送带有床位置的包。
 * 当玩家离开睡眠时，发送不带床位置的包。
 *
 * 参考 MC 1.16.5 SUseBedPacket
 */
class SleepPacket : public Packet {
public:
    SleepPacket()
        : Packet(PacketType::Sleep)
    {}

    /**
     * @brief 构造进入睡眠的包
     * @param entityId 实体ID
     * @param bedPos 床头位置
     */
    SleepPacket(u32 entityId, const BlockPos& bedPos);

    /**
     * @brief 构造离开睡眠的包
     * @param entityId 实体ID
     */
    static SleepPacket createWakeUp(u32 entityId);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // Getters
    [[nodiscard]] u32 entityId() const { return m_entityId; }
    [[nodiscard]] const std::optional<BlockPos>& bedPosition() const { return m_bedPos; }
    [[nodiscard]] bool isSleeping() const { return m_bedPos.has_value(); }

    // Setters
    void setEntityId(u32 id) { m_entityId = id; }
    void setBedPosition(const BlockPos& pos) { m_bedPos = pos; }
    void clearBedPosition() { m_bedPos = std::nullopt; }

private:
    u32 m_entityId = 0;
    std::optional<BlockPos> m_bedPos;
};

} // namespace mc::network
