#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 粒子生成数据包 (S->C)
 *
 * 服务端向客户端广播粒子生成事件。
 * 参考 MC 1.16.5 SSpawnParticlePacket
 *
 * 协议格式:
 * | 字段        | 类型      | 说明                        |
 * |-------------|-----------|----------------------------|
 * | particleId  | VarInt    | 粒子类型ID                  |
 * | x           | f64       | X坐标                       |
 * | y           | f64       | Y坐标                       |
 * | z           | f64       | Z坐标                       |
 * | offsetX     | f32       | X偏移范围                   |
 * | offsetY     | f32       | Y偏移范围                   |
 * | offsetZ     | f32       | Z偏移范围                   |
 * | velocityX   | f32       | X速度                       |
 * | velocityY   | f32       | Y速度                       |
 * | velocityZ   | f32       | Z速度                       |
 * | count       | VarInt    | 粒子数量                    |
 * | data        | bytes[]   | 可选数据（方块/物品/红石）   |
 *
 * 可选数据格式：
 * - Block/Breaking/FallingDust: VarInt blockStateId
 * - Item/ItemSlime/ItemSnowball: ItemStack (NBT)
 * - Redstone/Dust/DustColorTransition: f32 r, f32 g, f32 b, f32 scale
 */
class ParticlePacket : public Packet {
public:
    ParticlePacket()
        : Packet(PacketType::Particle)
    {}

    /**
     * @brief 从参数构造粒子包
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     */
    ParticlePacket(client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getters ==========

    [[nodiscard]] client::renderer::trident::particle::ParticleTypeId particleType() const { return m_particleType; }

    [[nodiscard]] f64 x() const { return m_x; }
    [[nodiscard]] f64 y() const { return m_y; }
    [[nodiscard]] f64 z() const { return m_z; }
    [[nodiscard]] Vector3 position() const
    {
        return Vector3(static_cast<f32>(m_x), static_cast<f32>(m_y), static_cast<f32>(m_z));
    }

    [[nodiscard]] f32 velocityX() const { return m_velocityX; }
    [[nodiscard]] f32 velocityY() const { return m_velocityY; }
    [[nodiscard]] f32 velocityZ() const { return m_velocityZ; }
    [[nodiscard]] Vector3 velocity() const { return Vector3(m_velocityX, m_velocityY, m_velocityZ); }

    [[nodiscard]] f32 offsetX() const { return m_offsetX; }
    [[nodiscard]] f32 offsetY() const { return m_offsetY; }
    [[nodiscard]] f32 offsetZ() const { return m_offsetZ; }
    [[nodiscard]] Vector3 offset() const { return Vector3(m_offsetX, m_offsetY, m_offsetZ); }

    [[nodiscard]] u32 count() const { return m_count; }

    [[nodiscard]] const std::vector<u8>& optionalData() const { return m_optionalData; }

    // ========== Setters ==========

    void setParticleType(client::renderer::trident::particle::ParticleTypeId type) { m_particleType = type; }

    void setPosition(f64 x, f64 y, f64 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    void setPosition(const Vector3& pos)
    {
        m_x = pos.x;
        m_y = pos.y;
        m_z = pos.z;
    }

    void setVelocity(f32 vx, f32 vy, f32 vz)
    {
        m_velocityX = vx;
        m_velocityY = vy;
        m_velocityZ = vz;
    }

    void setVelocity(const Vector3& vel)
    {
        m_velocityX = vel.x;
        m_velocityY = vel.y;
        m_velocityZ = vel.z;
    }

    void setOffset(f32 ox, f32 oy, f32 oz)
    {
        m_offsetX = ox;
        m_offsetY = oy;
        m_offsetZ = oz;
    }

    void setOffset(const Vector3& off)
    {
        m_offsetX = off.x;
        m_offsetY = off.y;
        m_offsetZ = off.z;
    }

    void setCount(u32 count) { m_count = count; }

    void setOptionalData(const std::vector<u8>& data) { m_optionalData = data; }

    void setOptionalData(std::vector<u8>&& data) { m_optionalData = std::move(data); }

    // ========== 便捷工厂方法 ==========

    /**
     * @brief 创建简单粒子包（无额外数据）
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     */
    static ParticlePacket create(client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    /**
     * @brief 创建单个粒子包（无偏移）
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     */
    static ParticlePacket createSingle(
        client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity);

private:
    client::renderer::trident::particle::ParticleTypeId m_particleType =
        client::renderer::trident::particle::ParticleTypeId::Invalid;

    f64 m_x = 0.0;
    f64 m_y = 0.0;
    f64 m_z = 0.0;

    f32 m_velocityX = 0.0f;
    f32 m_velocityY = 0.0f;
    f32 m_velocityZ = 0.0f;

    f32 m_offsetX = 0.0f;
    f32 m_offsetY = 0.0f;
    f32 m_offsetZ = 0.0f;

    u32 m_count = 1;

    std::vector<u8> m_optionalData;
};

} // namespace mc::network
