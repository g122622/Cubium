#pragma once

#include "../../core/Types.hpp"
#include "../../world/block/BlockPos.hpp"
#include "Packet.hpp"
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
 * 协议格式: BlockPos(x, y, z) + angle(f32)
 */
class SpawnPositionPacket : public Packet {
public:
    SpawnPositionPacket();
    explicit SpawnPositionPacket(const BlockPos& pos, f32 angle = 0.0f);
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

    /**
     * @brief 获取出生点偏航角
     *
     * 用于指南针指向计算。MC 1.16.5 新增字段。
     */
    [[nodiscard]] f32 angle() const { return m_angle; }
    void setAngle(f32 angle) { m_angle = angle; }

private:
    BlockPos m_position;
    f32 m_angle = 0.0f; // 出生点偏航角，用于指南针
};

} // namespace mc::network
