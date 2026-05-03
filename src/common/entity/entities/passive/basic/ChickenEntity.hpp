#pragma once

#include "AnimalEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 鸡实体
 *
 * 会下蛋的被动动物，用种子繁殖。
 * 特性：不会受到摔落伤害，有翅膀拍打动画。
 *
 * 参考 MC 1.16.5 ChickenEntity
 */
class ChickenEntity : public AnimalEntity {
public:
    ChickenEntity(LegacyEntityType type, EntityId id);
    ~ChickenEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 下蛋 ==========

    /**
     * @brief 获取下蛋计时器
     * @return 到下次下蛋的时间（tick）
     */
    [[nodiscard]] i32 getEggTimer() const { return m_eggTimer; }

    /**
     * @brief 重置下蛋计时器
     */
    void resetEggTimer();

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 声音 ==========

    /**
     * @brief 获取声音音量
     * MC 1.16.5: 鸡的音量为 1.0
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 1.0f; }

    /**
     * @brief 获取环境音效
     * 参考 MC 1.16.5 ChickenEntity.getAmbientSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * 参考 MC 1.16.5 ChickenEntity.getHurtSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * 参考 MC 1.16.5 ChickenEntity.getDeathSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取脚步声音
     * 参考 MC 1.16.5 ChickenEntity.playStepSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 翅膀动画 ==========

    /**
     * @brief 获取翅膀旋转角度（用于渲染）
     */
    [[nodiscard]] f32 getWingRotation() const { return m_wingRotation; }

    /**
     * @brief 获取上一帧翅膀旋转角度
     */
    [[nodiscard]] f32 getPrevWingRotation() const { return m_prevWingRotation; }

    /**
     * @brief 是否是鸡骑士
     * MC 1.16.5: 鸡骑士模式下不下蛋
     */
    [[nodiscard]] bool isChickenJockey() const { return m_chickenJockey; }

    /**
     * @brief 设置是否是鸡骑士
     */
    void setChickenJockey(bool jockey) { m_chickenJockey = jockey; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 脚步声 ==========

    /**
     * @brief 播放脚步声
     * MC 1.16.5: ChickenEntity.playStepSound() 使用固定的脚步声
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.4f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.7f; }

private:
    i32 m_eggTimer = 0;        // 下蛋计时器

    // 翅膀动画
    f32 m_wingRotation = 0.0f;
    f32 m_prevWingRotation = 0.0f;
    f32 m_wingRotDelta = 1.0f;

    // 鸡骑士标记
    bool m_chickenJockey = false;

    static constexpr i32 EGG_TIME_MIN = 6000;   // 最小下蛋时间（5分钟）
    static constexpr i32 EGG_TIME_MAX = 12000;  // 最大下蛋时间（10分钟）
};

} // namespace mc
