#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundTypes.hpp"

#include <glm/glm.hpp>
#include <optional>

namespace mc::sound {

/**
 * @brief 播放声音数据包（服务端->客户端）
 *
 * 在指定位置播放声音。位置使用定点整数传输（乘以8），与MC Java一致。
 * 这样可以节省带宽，同时保持足够的精度。
 *
 * 参考: net.minecraft.network.play.server.SPlaySoundPacket
 *
 * 使用示例:
 * @code
 * // 服务端发送
 * PlaySoundPacket packet(
 *     ResourceLocation("minecraft:block.stone.break"),
 *     SoundCategory::Blocks,
 *     glm::vec3(100.0f, 64.0f, 200.0f),
 *     1.0f,  // volume
 *     1.0f   // pitch
 * );
 * broadcastPacket(packet);
 *
 * // 客户端接收
 * auto soundEvent = packet.getSoundEventId();
 * auto category = packet.getCategory();
 * auto position = packet.getPosition();
 * soundEngine->play(soundEvent, category, position, packet.getVolume(), packet.getPitch());
 * @endcode
 */
class PlaySoundPacket : public network::Packet {
public:
    /**
     * @brief 默认构造函数（用于反序列化）
     */
    PlaySoundPacket();

    /**
     * @brief 构造播放声音包
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置（世界坐标）
     * @param volume 音量倍率（0.0-1.0，可超过1.0用于增大衰减距离）
     * @param pitch 音调倍率（0.5-2.0）
     */
    PlaySoundPacket(const ResourceLocation& soundEventId,
                    SoundCategory category,
                    const glm::vec3& position,
                    f32 volume,
                    f32 pitch);

    // ========================================================================
    // Packet 接口实现
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 属性访问
    // ========================================================================

    /**
     * @brief 获取声音事件ID
     */
    [[nodiscard]] const ResourceLocation& getSoundEventId() const noexcept { return m_soundEventId; }

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] SoundCategory getCategory() const noexcept { return m_category; }

    /**
     * @brief 获取声音位置
     *
     * 位置从定点整数转换回浮点数（除以8.0f）。
     *
     * @return 世界坐标位置
     */
    [[nodiscard]] glm::vec3 getPosition() const noexcept;

    /**
     * @brief 获取原始 X 坐标（定点整数）
     */
    [[nodiscard]] i32 getRawX() const noexcept { return m_x; }

    /**
     * @brief 获取原始 Y 坐标（定点整数）
     */
    [[nodiscard]] i32 getRawY() const noexcept { return m_y; }

    /**
     * @brief 获取原始 Z 坐标（定点整数）
     */
    [[nodiscard]] i32 getRawZ() const noexcept { return m_z; }

    /**
     * @brief 获取音量倍率
     */
    [[nodiscard]] f32 getVolume() const noexcept { return m_volume; }

    /**
     * @brief 获取音调倍率
     */
    [[nodiscard]] f32 getPitch() const noexcept { return m_pitch; }

private:
    ResourceLocation m_soundEventId;
    SoundCategory m_category = SoundCategory::Master;
    i32 m_x = 0;    // 定点整数（实际值 * 8）
    i32 m_y = 0;
    i32 m_z = 0;
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
};

/**
 * @brief 停止声音数据包（服务端->客户端）
 *
 * 停止指定声音事件或类别的所有正在播放的声音。
 * 可以只停止特定声音事件，或停止整个类别的声音。
 *
 * 参考: net.minecraft.network.play.server.SStopSoundPacket
 *
 * 使用示例:
 * @code
 * // 停止特定声音事件
 * StopSoundPacket packet(ResourceLocation("minecraft:music.game"));
 *
 * // 停止整个类别的声音
 * StopSoundPacket packet(std::nullopt, SoundCategory::Music);
 *
 * // 停止特定声音事件和类别
 * StopSoundPacket packet(ResourceLocation("minecraft:music.game"), SoundCategory::Music);
 *
 * // 停止所有声音
 * StopSoundPacket packet();
 * @endcode
 */
class StopSoundPacket : public network::Packet {
public:
    /**
     * @brief 默认构造函数（停止所有声音）
     */
    StopSoundPacket();

    /**
     * @brief 构造停止声音包
     *
     * @param soundEventId 声音事件ID（nullopt 表示不限）
     * @param category 声音类别（nullopt 表示不限）
     *
     * @note 如果两者都为 nullopt，则停止所有声音
     */
    StopSoundPacket(const std::optional<ResourceLocation>& soundEventId,
                    const std::optional<SoundCategory>& category);

    /**
     * @brief 构造停止特定声音事件的包
     */
    explicit StopSoundPacket(const ResourceLocation& soundEventId);

    /**
     * @brief 构造停止特定类别声音的包
     */
    explicit StopSoundPacket(SoundCategory category);

    // ========================================================================
    // Packet 接口实现
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 属性访问
    // ========================================================================

    /**
     * @brief 获取声音事件ID
     *
     * @return 声音事件ID，如果为 nullopt 表示不限声音事件
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getSoundEventId() const noexcept { return m_soundEventId; }

    /**
     * @brief 获取声音类别
     *
     * @return 声音类别，如果为 nullopt 表示不限类别
     */
    [[nodiscard]] const std::optional<SoundCategory>& getCategory() const noexcept { return m_category; }

    /**
     * @brief 检查是否停止所有声音
     */
    [[nodiscard]] bool isStopAll() const noexcept {
        return !m_soundEventId.has_value() && !m_category.has_value();
    }

private:
    std::optional<ResourceLocation> m_soundEventId;
    std::optional<SoundCategory> m_category;
};

/**
 * @brief 播放声音效果数据包（服务端->客户端）
 *
 * 用于播放与实体或方块关联的声音效果。
 * 与 PlaySoundPacket 类似，但通常用于特定事件触发的声音。
 *
 * 参考: net.minecraft.network.play.server.SPlaySoundEffectPacket
 */
class PlaySoundEffectPacket : public network::Packet {
public:
    /**
     * @brief 默认构造函数（用于反序列化）
     */
    PlaySoundEffectPacket();

    /**
     * @brief 构造播放声音效果包
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    PlaySoundEffectPacket(const ResourceLocation& soundEventId,
                          SoundCategory category,
                          const glm::vec3& position,
                          f32 volume,
                          f32 pitch);

    // ========================================================================
    // Packet 接口实现
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 属性访问
    // ========================================================================

    [[nodiscard]] const ResourceLocation& getSoundEventId() const noexcept { return m_soundEventId; }
    [[nodiscard]] SoundCategory getCategory() const noexcept { return m_category; }
    [[nodiscard]] glm::vec3 getPosition() const noexcept;
    [[nodiscard]] f32 getVolume() const noexcept { return m_volume; }
    [[nodiscard]] f32 getPitch() const noexcept { return m_pitch; }

private:
    ResourceLocation m_soundEventId;
    SoundCategory m_category = SoundCategory::Master;
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
};

/**
 * @brief 移动声音数据包（服务端->客户端）
 *
 * 用于播放附加到实体上的移动声音。声音会跟随实体移动，
 * 当实体被移除或声音停止时，声音自动结束。
 *
 * 参考: net.minecraft.network.play.server.SSpawnMovingSoundEffectPacket
 *
 * 使用示例:
 * @code
 * // 服务端发送 - 为实体播放移动声音
 * MovingSoundPacket packet(
 *     ResourceLocation("minecraft:entity.lightning_bolt.thunder"),
 *     SoundCategory::Weather,
 *     entityId,
 *     1.0f,   // volume
 *     1.0f    // pitch
 * );
 * broadcastPacket(packet);
 *
 * // 客户端接收后，创建 TickableSound 附加到实体上
 * @endcode
 */
class MovingSoundPacket : public network::Packet {
public:
    /**
     * @brief 默认构造函数（用于反序列化）
     */
    MovingSoundPacket();

    /**
     * @brief 构造移动声音包
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param entityId 实体ID（声音将跟随此实体）
     * @param volume 音量倍率（0.0-1.0，可超过1.0）
     * @param pitch 音调倍率（0.5-2.0）
     */
    MovingSoundPacket(const ResourceLocation& soundEventId,
                      SoundCategory category,
                      i32 entityId,
                      f32 volume,
                      f32 pitch);

    // ========================================================================
    // Packet 接口实现
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 属性访问
    // ========================================================================

    /**
     * @brief 获取声音事件ID
     */
    [[nodiscard]] const ResourceLocation& getSoundEventId() const noexcept { return m_soundEventId; }

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] SoundCategory getCategory() const noexcept { return m_category; }

    /**
     * @brief 获取实体ID
     *
     * 声音将跟随此实体移动。
     */
    [[nodiscard]] i32 getEntityId() const noexcept { return m_entityId; }

    /**
     * @brief 获取音量倍率
     */
    [[nodiscard]] f32 getVolume() const noexcept { return m_volume; }

    /**
     * @brief 获取音调倍率
     */
    [[nodiscard]] f32 getPitch() const noexcept { return m_pitch; }

private:
    ResourceLocation m_soundEventId;
    SoundCategory m_category = SoundCategory::Master;
    i32 m_entityId = 0;
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
};

} // namespace mc::sound
