#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "AbstractFishEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 河豚实体
 *
 * 有毒的海洋鱼类。
 *
 * 特性：
 * - 膨胀：玩家靠近时会膨胀
 * - 中毒：接触会导致中毒
 * - 掉落：河豚、骨头
 *
 * 音效：
 * - ENTITY_PUFFER_FISH_AMBIENT: 水中环境音
 * - ENTITY_PUFFER_FISH_BLOW_UP: 膨胀音效
 * - ENTITY_PUFFER_FISH_BLOW_OUT: 收缩音效
 * - ENTITY_PUFFER_FISH_DEATH: 死亡音效
 * - ENTITY_PUFFER_FISH_FLOP: 陆地扑腾音效
 * - ENTITY_PUFFER_FISH_HURT: 受伤音效
 * - ENTITY_PUFFER_FISH_STING: 刺击音效
 *
 * 参考 MC 1.16.5 PufferfishEntity
 */
class PufferfishEntity : public AbstractFishEntity {
public:
    /**
     * @brief 河豚膨胀状态
     */
    enum class PuffState : u8 {
        Deflated = 0,   // 未膨胀
        SemiPuffed = 1, // 半膨胀
        FullyPuffed = 2 // 完全膨胀
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    PufferfishEntity(LegacyEntityType type, EntityId id);
    ~PufferfishEntity() override = default;

    // 禁止拷贝
    PufferfishEntity(const PufferfishEntity&) = delete;
    PufferfishEntity& operator=(const PufferfishEntity&) = delete;

    // 允许移动
    PufferfishEntity(PufferfishEntity&&) = default;
    PufferfishEntity& operator=(PufferfishEntity&&) = default;

    /**
     * @brief 创建河豚实体
     * @param world 世界实例
     * @return 新的河豚实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 膨胀状态 ==========

    /**
     * @brief 获取膨胀状态
     */
    [[nodiscard]] PuffState getPuffState() const { return m_puffState; }

    /**
     * @brief 设置膨胀状态
     *
     * 当状态变化时会自动播放膨胀/收缩音效。
     */
    void setPuffState(PuffState state);

    /**
     * @brief 获取膨胀尺寸
     * 根据膨胀状态返回不同的碰撞箱大小
     */
    [[nodiscard]] f32 getPuffSize() const;

    /**
     * @brief 是否完全膨胀
     */
    [[nodiscard]] bool isFullyPuffed() const { return m_puffState == PuffState::FullyPuffed; }

    // ========== 中毒 ==========

    /**
     * @brief 是否会使接触者中毒
     */
    [[nodiscard]] bool canPoison() const { return m_puffState != PuffState::Deflated; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.15f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 在水中和陆地播放不同的音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    PuffState m_puffState = PuffState::Deflated;
    i32 m_puffTimer = 0;
    i32 m_deflateTimer = 0;

    // MC 1.16.5: 收缩延迟常量
    static constexpr i32 PUFF_DURATION = 60;            // 膨胀持续时间（ticks）
    static constexpr i32 DEFLATE_SEMI_TO_DEFLATE = 100; // 半膨胀到未膨胀的延迟
    static constexpr i32 DEFLATE_FULL_TO_SEMI = 60;     // 完全膨胀到半膨胀的延迟
};

} // namespace mc
