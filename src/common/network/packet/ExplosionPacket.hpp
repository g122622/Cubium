#pragma once

#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../world/block/BlockPos.hpp"
#include <vector>
#include <unordered_map>

namespace mc::network {

/**
 * @brief 爆炸数据包 (S->C)
 *
 * 服务端向客户端广播爆炸事件。
 * 参考 MC 1.16.5 SExplosionPacket
 *
 * 协议格式:
 * | 字段                  | 类型      | 说明                              |
 * |-----------------------|-----------|-----------------------------------|
 * | x                     | f32       | 爆炸位置 X                        |
 * | y                     | f32       | 爆炸位置 Y                        |
 * | z                     | f32       | 爆炸位置 Z                        |
 * | strength              | f32       | 爆炸威力（半径）                  |
 * | affectedBlockCount    | VarInt    | 受影响方块数量                    |
 * | affectedBlocks        | bytes[]   | 方块相对坐标列表（每方块3字节）   |
 * | motionX               | f32       | 玩家击退速度 X                    |
 * | motionY               | f32       | 玩家击退速度 Y                    |
 * | motionZ               | f32       | 玩家击退速度 Z                    |
 *
 * 方块坐标编码：
 * - 每个方块使用3字节存储相对坐标
 * - deltaX = blockX - floor(explosionX)
 * - deltaY = blockY - floor(explosionY)
 * - deltaZ = blockZ - floor(explosionZ)
 * - 使用有符号字节存储（范围 -128 到 127）
 *
 * 使用说明：
 * - 服务端在爆炸计算完成后发送此包
 * - 只发送给爆炸点 64 格范围内的玩家
 * - 每个玩家收到的击退向量可能不同
 */
class ExplosionPacket : public Packet {
public:
    ExplosionPacket();

    /**
     * @brief 从爆炸数据构造数据包
     *
     * @param position 爆炸中心位置
     * @param strength 爆炸威力（半径）
     * @param affectedBlocks 受影响的方块位置列表
     * @param playerKnockback 玩家击退向量（玩家ID -> 击退向量）
     * @param targetPlayerId 目标玩家ID（用于从 playerKnockback 中获取击退向量）
     */
    ExplosionPacket(
        const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback,
        u64 targetPlayerId);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getters ==========

    [[nodiscard]] f32 x() const { return m_x; }
    [[nodiscard]] f32 y() const { return m_y; }
    [[nodiscard]] f32 z() const { return m_z; }
    [[nodiscard]] Vector3 position() const { return Vector3(m_x, m_y, m_z); }

    [[nodiscard]] f32 strength() const { return m_strength; }

    [[nodiscard]] const std::vector<BlockPos>& affectedBlocks() const { return m_affectedBlocks; }

    [[nodiscard]] f32 motionX() const { return m_motionX; }
    [[nodiscard]] f32 motionY() const { return m_motionY; }
    [[nodiscard]] f32 motionZ() const { return m_motionZ; }
    [[nodiscard]] Vector3 motion() const { return Vector3(m_motionX, m_motionY, m_motionZ); }

    // ========== Setters ==========

    void setPosition(f32 x, f32 y, f32 z) {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    void setPosition(const Vector3& pos) {
        m_x = pos.x;
        m_y = pos.y;
        m_z = pos.z;
    }

    void setStrength(f32 strength) {
        m_strength = strength;
    }

    void setAffectedBlocks(const std::vector<BlockPos>& blocks) {
        m_affectedBlocks = blocks;
    }

    void setAffectedBlocks(std::vector<BlockPos>&& blocks) {
        m_affectedBlocks = std::move(blocks);
    }

    void setMotion(f32 mx, f32 my, f32 mz) {
        m_motionX = mx;
        m_motionY = my;
        m_motionZ = mz;
    }

    void setMotion(const Vector3& motion) {
        m_motionX = motion.x;
        m_motionY = motion.y;
        m_motionZ = motion.z;
    }

    /**
     * @brief 从玩家击退映射中设置当前玩家的击退向量
     *
     * @param playerKnockback 玩家ID到击退向量的映射
     * @param playerId 当前玩家ID
     */
    void setKnockbackForPlayer(
        const std::unordered_map<u64, Vector3>& playerKnockback,
        u64 playerId);

private:
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_strength = 0.0f;
    std::vector<BlockPos> m_affectedBlocks;
    f32 m_motionX = 0.0f;
    f32 m_motionY = 0.0f;
    f32 m_motionZ = 0.0f;
};

} // namespace mc::network
