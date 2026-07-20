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

#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>
#include <vector>

namespace mc {
class ItemStack;
} // namespace mc

namespace mc::network {

/**
 * @brief 粒子生成数据包 (S->C)
 *
 * 服务端向客户端广播粒子生成事件。
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
 * | data        | bytes[]   | 可选数据（方块/物品/红石/振动）|
 *
 * 可选数据格式：
 * - Block/Breaking/FallingDust: VarInt blockStateId
 * - Item/ItemSlime/ItemSnowball: ItemStack (NBT)
 * - Dust/Redstone: i32 color(ARGB), f32 scale
 * - DustColorTransition: i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
 * - Vibration: VarInt positionSourceTypeId(0=Block,1=Entity), [Block: i64 packedBlockPos | Entity: VarInt entityId, f32
 * yOffset], VarInt arrivalInTicks
 * - Trail: f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
 * - EntityEffect: i32 color(ARGB)
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
    ParticlePacket(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    [[nodiscard]] size_t expectedSize() const noexcept override;

    // ========== Getters ==========

    [[nodiscard]] particle::ParticleTypeId particleType() const noexcept { return m_particleType; }

    [[nodiscard]] f64 x() const noexcept { return m_x; }
    [[nodiscard]] f64 y() const noexcept { return m_y; }
    [[nodiscard]] f64 z() const noexcept { return m_z; }
    [[nodiscard]] Vector3 position() const noexcept
    {
        return Vector3(static_cast<f32>(m_x), static_cast<f32>(m_y), static_cast<f32>(m_z));
    }

    [[nodiscard]] f32 velocityX() const noexcept { return m_velocityX; }
    [[nodiscard]] f32 velocityY() const noexcept { return m_velocityY; }
    [[nodiscard]] f32 velocityZ() const noexcept { return m_velocityZ; }
    [[nodiscard]] Vector3 velocity() const noexcept { return Vector3(m_velocityX, m_velocityY, m_velocityZ); }

    [[nodiscard]] f32 offsetX() const noexcept { return m_offsetX; }
    [[nodiscard]] f32 offsetY() const noexcept { return m_offsetY; }
    [[nodiscard]] f32 offsetZ() const noexcept { return m_offsetZ; }
    [[nodiscard]] Vector3 offset() const noexcept { return Vector3(m_offsetX, m_offsetY, m_offsetZ); }

    [[nodiscard]] u32 count() const noexcept { return m_count; }

    [[nodiscard]] const std::vector<u8>& optionalData() const noexcept { return m_optionalData; }

    // ========== Setters ==========

    void setParticleType(particle::ParticleTypeId type) noexcept { m_particleType = type; }

    void setPosition(f64 x, f64 y, f64 z) noexcept
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    void setPosition(const Vector3& pos) noexcept
    {
        m_x = pos.x;
        m_y = pos.y;
        m_z = pos.z;
    }

    void setVelocity(f32 vx, f32 vy, f32 vz) noexcept
    {
        m_velocityX = vx;
        m_velocityY = vy;
        m_velocityZ = vz;
    }

    void setVelocity(const Vector3& vel) noexcept
    {
        m_velocityX = vel.x;
        m_velocityY = vel.y;
        m_velocityZ = vel.z;
    }

    void setOffset(f32 ox, f32 oy, f32 oz) noexcept
    {
        m_offsetX = ox;
        m_offsetY = oy;
        m_offsetZ = oz;
    }

    void setOffset(const Vector3& off) noexcept
    {
        m_offsetX = off.x;
        m_offsetY = off.y;
        m_offsetZ = off.z;
    }

    void setCount(u32 count) noexcept { m_count = count; }

    void setOptionalData(const std::vector<u8>& data) { m_optionalData = data; }

    void setOptionalData(std::vector<u8>&& data) noexcept { m_optionalData = std::move(data); }

    // ========== 便捷工厂方法 ==========

    /**
     * @brief 创建简单粒子包（无额外数据）
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     */
    static ParticlePacket create(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count);

    /**
     * @brief 创建单个粒子包（无偏移）
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     */
    static ParticlePacket createSingle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity);

    /**
     * @brief 创建振动粒子包（方块目标位置）
     *
     * 振动粒子需要额外数据：目标位置来源和到达 tick 数。
     * 这些数据编码到 optionalData 中，客户端解码后创建 VibrationSignalParticle。
     *
     * 可选数据格式（与 MC Java 1.21.11 VibrationParticleOption.STREAM_CODEC 一致）：
     *   VarInt positionSourceTypeId(0=Block)
     *   i64 packedBlockPos (BlockPos.asLong: X bits 38-63, Z bits 12-37, Y bits 0-11)
     *   VarInt arrivalInTicks
     *
     * @param pos 粒子起始位置（振动源位置）
     * @param targetBlockPos 粒子飞向的目标方块位置（监听器所在方块）
     * @param arrivalInTicks 到达目标的 tick 数
     */
    static ParticlePacket createVibration(const Vector3& pos, const BlockPos& targetBlockPos, i32 arrivalInTicks);

    /**
     * @brief 创建振动粒子包（实体目标位置）
     *
     * 可选数据格式（与 MC Java 1.21.11 VibrationParticleOption.STREAM_CODEC 一致）：
     *   VarInt positionSourceTypeId(1=Entity)
     *   VarInt entityId
     *   f32 yOffset
     *   VarInt arrivalInTicks
     *
     * @param pos 粒子起始位置（振动源位置）
     * @param targetEntityId 粒子飞向的目标实体 ID
     * @param yOffset 实体位置的 Y 轴偏移（如眼睛高度）
     * @param arrivalInTicks 到达目标的 tick 数
     */
    static ParticlePacket createVibration(
        const Vector3& pos, EntityInstanceId targetEntityId, f32 yOffset, i32 arrivalInTicks);

    /**
     * @brief 创建轨迹粒子包（带目标位置、颜色和持续时间）
     *
     * 轨迹粒子需要额外数据：目标位置、颜色和持续时间。
     * 这些数据编码到 optionalData 中，客户端解码后创建 TrailParticle。
     *
     * 可选数据格式：f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
     *
     * @param pos 粒子起始位置
     * @param targetPosition 粒子飞向的目标位置
     * @param color 粒子颜色（ARGB 格式）
     * @param durationInTicks 飞行持续时间（tick 数）
     */
    static ParticlePacket createTrail(
        const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks);

    // ========== 振动粒子数据解码 ==========

    /**
     * @brief 振动粒子目标来源
     *
     * 解码自 VibrationParticleOption 的 PositionSource 部分。
     * 对应 MC Java: PositionSource（BlockPositionSource 或 EntityPositionSource）。
     * 客户端据此决定如何解析目标位置：方块来源直接取方块中心，实体来源需查实体位置。
     */
    struct VibrationTarget {
        /// 来源类型
        enum class Kind : u8 {
            Block = 0,  ///< 方块位置源（对应 positionSourceTypeId=0）
            Entity = 1, ///< 实体位置源（对应 positionSourceTypeId=1）
        };

        Kind kind = Kind::Block;

        // kind == Block 时有效：目标方块位置
        BlockPos blockPos{};

        // kind == Entity 时有效：目标实体 ID 与 Y 轴偏移
        EntityInstanceId entityId = INVALID_ENTITY_ID;
        f32 yOffset = 0.0f;
    };

    /**
     * @brief 检查此粒子包是否为振动粒子
     *
     * 振动粒子包的粒子类型为 ParticleTypeId::Vibration 且含有可选数据。
     *
     * @return 是否为振动粒子
     */
    [[nodiscard]] bool isVibrationParticle() const noexcept;

    /**
     * @brief 解码振动粒子目标来源
     *
     * 仅当 isVibrationParticle() 返回 true 时有效。
     * 从可选数据中解码 PositionSource 类型与数据。
     *
     * @return 目标来源，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<VibrationTarget> decodeVibrationTarget() const;

    /**
     * @brief 解码振动粒子到达时间
     *
     * 仅当 isVibrationParticle() 返回 true 时有效。
     * 从可选数据中解码到达 tick 数。
     *
     * @return 到达 tick 数，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<i32> decodeVibrationArrivalInTicks() const;

    // ========== 轨迹粒子数据解码 ==========

    /**
     * @brief 检查此粒子包是否为轨迹粒子
     *
     * 轨迹粒子包的粒子类型为 ParticleTypeId::Trail 且含有可选数据。
     *
     * @return 是否为轨迹粒子
     */
    [[nodiscard]] bool isTrailParticle() const noexcept;

    /**
     * @brief 解码轨迹粒子目标位置
     *
     * 仅当 isTrailParticle() 返回 true 时有效。
     * 从可选数据中解码目标位置。
     *
     * @return 目标位置，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<Vector3d> decodeTrailTarget() const;

    /**
     * @brief 解码轨迹粒子颜色
     *
     * 仅当 isTrailParticle() 返回 true 时有效。
     * 从可选数据中解码 ARGB 颜色。
     *
     * @return ARGB 颜色，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeTrailColor() const;

    /**
     * @brief 解码轨迹粒子持续时间
     *
     * 仅当 isTrailParticle() 返回 true 时有效。
     * 从可选数据中解码飞行持续时间（tick 数）。
     *
     * @return 持续时间，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<i32> decodeTrailDuration() const;

    // ========== 灰尘粒子数据编解码 ==========

    /**
     * @brief 创建灰尘粒子包（带颜色和缩放）
     *
     * 灰尘粒子需要额外数据：颜色和缩放。
     * 这些数据编码到 optionalData 中，客户端解码后创建 DustParticle。
     *
     * 可选数据格式：i32 color(ARGB), f32 scale
     *
     * @param type 粒子类型（Dust 或 Redstone）
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式）
     * @param scale 缩放因子
     */
    static ParticlePacket createDust(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        u32 color,
        f32 scale);

    /**
     * @brief 创建颜色过渡灰尘粒子包（带起始颜色、目标颜色和缩放）
     *
     * 颜色过渡灰尘粒子需要额外数据：起始颜色、目标颜色和缩放。
     *
     * 可选数据格式：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
     *
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     * @param fromColor 起始颜色（ARGB 格式）
     * @param toColor 目标颜色（ARGB 格式）
     * @param scale 缩放因子
     */
    static ParticlePacket createDustColorTransition(const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        u32 fromColor,
        u32 toColor,
        f32 scale);

    /**
     * @brief 检查此粒子包是否为灰尘粒子（Dust 或 Redstone）
     *
     * 灰尘粒子包的粒子类型为 Dust 或 Redstone 且含有可选数据。
     *
     * @return 是否为灰尘粒子
     */
    [[nodiscard]] bool isDustParticle() const noexcept;

    /**
     * @brief 解码灰尘粒子颜色
     *
     * 仅当 isDustParticle() 返回 true 时有效。
     * 从可选数据中解码 ARGB 颜色。
     *
     * @return ARGB 颜色，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeDustColor() const;

    /**
     * @brief 解码灰尘粒子缩放
     *
     * 仅当 isDustParticle() 返回 true 时有效。
     * 从可选数据中解码缩放因子。
     *
     * @return 缩放因子，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<f32> decodeDustScale() const;

    /**
     * @brief 检查此粒子包是否为颜色过渡灰尘粒子
     *
     * 颜色过渡灰尘粒子包的粒子类型为 DustColorTransition 且含有可选数据。
     *
     * @return 是否为颜色过渡灰尘粒子
     */
    [[nodiscard]] bool isDustColorTransitionParticle() const noexcept;

    /**
     * @brief 解码颜色过渡灰尘粒子起始颜色
     *
     * 仅当 isDustColorTransitionParticle() 返回 true 时有效。
     *
     * @return 起始 ARGB 颜色，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeDustColorTransitionFromColor() const;

    /**
     * @brief 解码颜色过渡灰尘粒子目标颜色
     *
     * 仅当 isDustColorTransitionParticle() 返回 true 时有效。
     *
     * @return 目标 ARGB 颜色，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeDustColorTransitionToColor() const;

    /**
     * @brief 解码颜色过渡灰尘粒子缩放
     *
     * 仅当 isDustColorTransitionParticle() 返回 true 时有效。
     *
     * @return 缩放因子，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<f32> decodeDustColorTransitionScale() const;

    // ========== 实体效果粒子数据编解码 ==========

    /**
     * @brief 创建实体效果粒子包（带颜色）
     *
     * 实体效果粒子需要额外数据：ARGB 颜色。
     * 这些数据编码到 optionalData 中，客户端解码后创建 EntityEffectParticle。
     *
     * 可选数据格式：i32 color(ARGB)
     *
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式）
     */
    static ParticlePacket createEntityEffect(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color);

    /**
     * @brief 检查此粒子包是否为实体效果粒子
     *
     * 实体效果粒子包的粒子类型为 EntityEffect 且含有可选数据。
     *
     * @return 是否为实体效果粒子
     */
    [[nodiscard]] bool isEntityEffectParticle() const noexcept;

    /**
     * @brief 解码实体效果粒子颜色
     *
     * 仅当 isEntityEffectParticle() 返回 true 时有效。
     * 从可选数据中解码 ARGB 颜色。
     *
     * @return ARGB 颜色，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeEntityEffectColor() const;

    // ========== 方块粒子数据编解码 ==========

    /**
     * @brief 创建方块粒子包（带方块状态 ID）
     *
     * 方块粒子（Block/Breaking/FallingDust/BlockMarker/BlockCrumble/DustPillar）
     * 需要额外数据：方块状态 ID，用于客户端选择正确的方块纹理。
     *
     * 可选数据格式：VarInt blockStateId
     *
     * @param type 粒子类型（必须为 requiresBlockState 返回 true 的类型）
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     * @param blockStateId 方块状态 ID（BlockState::stateId()）
     */
    static ParticlePacket createBlock(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        u32 blockStateId);

    /**
     * @brief 检查此粒子包是否为携带方块状态的方块粒子
     *
     * 方块粒子包的粒子类型必须为 requiresBlockState 返回 true 的类型，
     * 且含有可选数据。
     *
     * @return 是否为携带方块状态的方块粒子
     */
    [[nodiscard]] bool isBlockParticle() const noexcept;

    /**
     * @brief 解码方块粒子的方块状态 ID
     *
     * 仅当 isBlockParticle() 返回 true 时有效。
     * 客户端可通过 BlockRegistry::instance().getBlockState(stateId) 解析回 BlockState。
     *
     * @return 方块状态 ID，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<u32> decodeBlockStateId() const;

    // ========== 物品粒子数据编解码 ==========

    /**
     * @brief 创建物品粒子包（带物品堆）
     *
     * 物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball）需要额外数据：ItemStack，
     * 用于客户端选择正确的物品纹理。
     *
     * 可选数据格式：ItemStack 序列化字节流（由 ItemStack::serialize(PacketSerializer&) 产生）。
     * 该字节流自包含（带 itemId、count、damage 等字段），无需额外长度前缀，
     * 因为整个 optionalData 即为该字节流。
     *
     * @param type 粒子类型（必须为 requiresItemData 返回 true 的类型）
     * @param pos 位置
     * @param velocity 速度
     * @param offset 偏移范围
     * @param count 粒子数量
     * @param itemStack 物品堆
     */
    static ParticlePacket createItem(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        const ::mc::ItemStack& itemStack);

    /**
     * @brief 检查此粒子包是否为携带物品堆的物品粒子
     *
     * 物品粒子包的粒子类型必须为 requiresItemData 返回 true 的类型，
     * 且含有可选数据。
     *
     * @return 是否为携带物品堆的物品粒子
     */
    [[nodiscard]] bool isItemParticle() const noexcept;

    /**
     * @brief 解码物品粒子的物品堆
     *
     * 仅当 isItemParticle() 返回 true 时有效。
     * 客户端通过 ItemStack::deserialize(PacketDeserializer&) 解析回 ItemStack。
     *
     * @return 物品堆，解码失败返回 std::nullopt
     */
    [[nodiscard]] std::optional<::mc::ItemStack> decodeItemStack() const;

private:
    particle::ParticleTypeId m_particleType = particle::ParticleTypeId::Invalid;

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
