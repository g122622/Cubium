#pragma once

#include "Packet.hpp"
#include "../../core/Types.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 设置乘客列表包
 *
 * 服务端向客户端发送实体的乘客列表。
 * 当实体骑乘/离开载具时发送。
 *
 * 参考 MC 1.16.5 SSetPassengersPacket
 */
class SetPassengersPacket : public Packet {
public:
    SetPassengersPacket();
    explicit SetPassengersPacket(u32 entityId, const std::vector<u32>& passengerIds);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    [[nodiscard]] u32 entityId() const { return m_entityId; }
    [[nodiscard]] const std::vector<u32>& passengerIds() const { return m_passengerIds; }

    void setEntityId(u32 entityId) { m_entityId = entityId; }
    void setPassengerIds(const std::vector<u32>& ids) { m_passengerIds = ids; }
    void addPassengerId(u32 id) { m_passengerIds.push_back(id); }

private:
    u32 m_entityId = 0;
    std::vector<u32> m_passengerIds;
};

} // namespace mc::network
