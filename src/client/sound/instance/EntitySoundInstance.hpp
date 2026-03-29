#pragma once

#include "client/sound/instance/SoundInstance.hpp"

namespace mc {

// 前向声明
class Entity;

namespace client::sound {

/**
 * @brief 实体跟随声音实例
 *
 * 声音位置自动跟随实体移动。
 * 当实体被移除或变为静音时自动停止。
 *
 * 参考: net.minecraft.client.audio.EntityTickableSound
 *
 * 使用示例:
 * @code
 * // 播放矿车声音
 * auto sound = std::make_unique<EntitySoundInstance>(
 *     ResourceLocation("minecraft:entity.minecart.inside"),
 *     SoundCategory::Blocks,
 *     minecartEntity,
 *     1.0f,  // volume
 *     1.0f   // pitch
 * );
 * sound->setLooping(true);
 * engine.play(std::move(sound));
 *
 * // 播放生物受伤声音
 * auto sound = std::make_unique<EntitySoundInstance>(
 *     ResourceLocation("minecraft:entity.cow.hurt"),
 *     SoundCategory::Neutral,
 *     cowEntity
 * );
 * engine.play(std::move(sound));
 * @endcode
 */
class EntitySoundInstance : public TickableSound {
public:
    /**
     * @brief 构造实体跟随声音
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param entity 实体引用（必须是有效引用）
     * @param volume 音量（默认1.0）
     * @param pitch 音调（默认1.0）
     */
    EntitySoundInstance(
        const ResourceLocation& soundEventId,
        SoundCategory category,
        Entity& entity,
        f32 volume = 1.0f,
        f32 pitch = 1.0f
    );

    /**
     * @brief 析构函数
     */
    ~EntitySoundInstance() override = default;

    // 禁止拷贝
    EntitySoundInstance(const EntitySoundInstance&) = delete;
    EntitySoundInstance& operator=(const EntitySoundInstance&) = delete;

    // 允许移动
    EntitySoundInstance(EntitySoundInstance&&) noexcept = default;
    EntitySoundInstance& operator=(EntitySoundInstance&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 更新声音位置以跟随实体。
     * 如果实体被移除或静音，标记声音为完成。
     */
    void tick() override;

    // ========================================================================
    // EntitySoundInstance 特有方法
    // ========================================================================

    /**
     * @brief 获取关联的实体
     */
    [[nodiscard]] Entity& entity() const { return m_entity; }

    /**
     * @brief 检查实体是否仍然有效（未被移除且未静音）
     */
    [[nodiscard]] bool isEntityValid() const;

private:
    Entity& m_entity;
};

} // namespace mc::client::sound
} // namespace mc
