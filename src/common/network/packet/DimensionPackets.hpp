#pragma once

#include "Packet.hpp"
#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 重生/维度切换包 (服务端 -> 客户端)
 *
 * 当玩家重生或切换维度时发送。客户端应卸载当前维度的所有区块，
 * 重置状态，并准备加载新维度。
 *
 * 参考 MC 1.16.5 SRespawnPacket:
 * - DimensionType field_240822_a_ (维度类型)
 * - RegistryKey<World> dimensionID (世界名称: minecraft:overworld/nether/the_end)
 * - long hashedSeed (世界种子的 SHA-256 前8字节)
 * - GameType gameType
 * - GameType field_241787_e_ (上一个游戏模式)
 * - boolean field_240823_e_ (isDebug)
 * - boolean field_240824_f_ (isFlat)
 * - boolean field_240825_g_ (copyMetadata - 用于维度切换时保留数据)
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
     * @brief 获取维度类型
     *
     * 维度类型定义了维度的物理特性：
     * - hasSkyLight: 是否有天空光照
     * - hasCeiling: 是否有天花板
     * - ultraWarm: 是否是超热维度（下界）
     * - coordinateScale: 坐标缩放比例
     */
    [[nodiscard]] i32 dimensionType() const { return m_dimensionType; }
    void setDimensionType(i32 type) { m_dimensionType = type; }

    /**
     * @brief 获取维度ID
     *
     * 维度的唯一标识符：
     * - 0: 主世界 (minecraft:overworld)
     * - -1: 下界 (minecraft:the_nether)
     * - 1: 末地 (minecraft:the_end)
     */
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    /**
     * @brief 获取世界种子的哈希值
     *
     * 世界种子 SHA-256 的前8字节，用于客户端验证。
     */
    [[nodiscard]] u64 hashedSeed() const { return m_hashedSeed; }
    void setHashedSeed(u64 seed) { m_hashedSeed = seed; }

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
     * @brief 是否保留玩家数据
     *
     * MC 1.16.5: copyMetadata
     * 如果为 true，客户端应保留某些玩家状态（如经验值）。
     * 维度切换时通常为 true，死亡重生时为 false。
     */
    [[nodiscard]] bool keepData() const { return m_keepData; }
    void setKeepData(bool keep) { m_keepData = keep; }

private:
    i32 m_dimensionType = 0;     // 维度类型（用于渲染设置）
    DimensionId m_dimension = 0;  // 维度ID (0=主世界, -1=下界, 1=末地)
    u64 m_hashedSeed = 0;         // 世界种子哈希
    GameMode m_gameMode = GameMode::Survival;
    GameMode m_previousGameMode = GameMode::NotSet;
    bool m_isDebug = false;
    bool m_isFlat = false;
    bool m_keepData = false;     // 维度切换时保留数据
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
        std::string name;
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
