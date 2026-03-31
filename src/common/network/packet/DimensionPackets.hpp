#pragma once

#include "Packet.hpp"
#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 维度切换包 (服务端 -> 客户端)
 *
 * 当玩家切换维度时发送。客户端应卸载当前维度的所有区块，
 * 重置状态，并准备加载新维度。
 *
 * 参考 MC 1.16.5 SChangeGameStatePacket 或 SRespawnPacket
 */
class ChangeDimensionPacket : public Packet {
public:
    ChangeDimensionPacket();
    ~ChangeDimensionPacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取目标维度ID
     */
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    /**
     * @brief 获取目标位置
     */
    [[nodiscard]] const Vector3d& position() const { return m_position; }
    void setPosition(const Vector3d& pos) { m_position = pos; }

    /**
     * @brief 是否为重生触发的维度切换
     *
     * 如果为 true，客户端应显示重生界面。
     */
    [[nodiscard]] bool respawn() const { return m_respawn; }
    void setRespawn(bool respawn) { m_respawn = respawn; }

private:
    DimensionId m_dimension = 0;
    Vector3d m_position;
    bool m_respawn = false;
};

/**
 * @brief 重生包 (服务端 -> 客户端)
 *
 * 当玩家重生时发送（死亡后重生或从末地返回）。
 * 包含新维度的信息和玩家状态。
 *
 * 参考 MC 1.16.5 SRespawnPacket
 */
class RespawnPacket : public Packet {
public:
    RespawnPacket();
    ~RespawnPacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取重生维度ID
     */
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    /**
     * @brief 获取重生位置
     */
    [[nodiscard]] const Vector3d& position() const { return m_position; }
    void setPosition(const Vector3d& pos) { m_position = pos; }

    /**
     * @brief 获取重生时的偏航角
     */
    [[nodiscard]] f32 yaw() const { return m_yaw; }
    void setYaw(f32 yaw) { m_yaw = yaw; }

    /**
     * @brief 获取重生时的俯仰角
     */
    [[nodiscard]] f32 pitch() const { return m_pitch; }
    void setPitch(f32 pitch) { m_pitch = pitch; }

    /**
     * @brief 获取游戏模式
     */
    [[nodiscard]] GameMode gameMode() const { return m_gameMode; }
    void setGameMode(GameMode mode) { m_gameMode = mode; }

    /**
     * @brief 获取之前的游戏模式
     */
    [[nodiscard]] GameMode previousGameMode() const { return m_previousGameMode; }
    void setPreviousGameMode(GameMode mode) { m_previousGameMode = mode; }

    /**
     * @brief 是否为调试世界
     */
    [[nodiscard]] bool isDebug() const { return m_isDebug; }
    void setDebug(bool debug) { m_isDebug = debug; }

    /**
     * @brief 是否为超平坦世界
     */
    [[nodiscard]] bool isFlat() const { return m_isFlat; }
    void setFlat(bool flat) { m_isFlat = flat; }

    /**
     * @brief 是否复制元数据
     *
     * 如果为 true，客户端应保留某些玩家状态（如经验值）。
     */
    [[nodiscard]] bool copyMetadata() const { return m_copyMetadata; }
    void setCopyMetadata(bool copy) { m_copyMetadata = copy; }

private:
    DimensionId m_dimension = 0;
    Vector3d m_position;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    GameMode m_gameMode = GameMode::Survival;
    GameMode m_previousGameMode = GameMode::NotSet;
    bool m_isDebug = false;
    bool m_isFlat = false;
    bool m_copyMetadata = false;
};

/**
 * @brief 维度信息包 (服务端 -> 客户端)
 *
 * 在玩家登录时发送，告知客户端服务器支持的所有维度。
 * 客户端使用此信息准备维度渲染器。
 */
class DimensionInfoPacket : public Packet {
public:
    /**
     * @brief 单个维度的信息
     */
    struct DimensionInfo {
        DimensionId id = 0;
        String name;
        bool hasSkyLight = true;
        bool hasCeiling = false;
        f32 ambientLight = 0.0f;
    };

    DimensionInfoPacket();
    ~DimensionInfoPacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取所有维度信息
     */
    [[nodiscard]] const std::vector<DimensionInfo>& dimensions() const { return m_dimensions; }

    /**
     * @brief 添加维度信息
     */
    void addDimension(const DimensionInfo& info) { m_dimensions.push_back(info); }

    /**
     * @brief 清空维度信息
     */
    void clear() { m_dimensions.clear(); }

    /**
     * @brief 获取维度数量
     */
    [[nodiscard]] size_t count() const { return m_dimensions.size(); }

private:
    std::vector<DimensionInfo> m_dimensions;
};

/**
 * @brief 确认维度切换包 (客户端 -> 服务端)
 *
 * 客户端完成维度切换后发送，通知服务端可以发送新区块数据。
 */
class ConfirmDimensionChangePacket : public Packet {
public:
    ConfirmDimensionChangePacket();
    ~ConfirmDimensionChangePacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取确认的维度ID
     */
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

private:
    DimensionId m_dimension = 0;
};

} // namespace mc::network
