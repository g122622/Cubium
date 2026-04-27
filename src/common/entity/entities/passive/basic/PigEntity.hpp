#pragma once

#include "AnimalEntity.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class Player;
class DamageSource;

/**
 * @brief 猪实体
 *
 * 最基础的被动动物，可被骑乘（使用鞍）。
 * 实现 IRideable 接口以支持骑乘功能。
 *
 * 参考 MC 1.16.5 PigEntity
 */
class PigEntity : public AnimalEntity, public entity::IRideable {
public:
    PigEntity(LegacyEntityType type, EntityId id);
    ~PigEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * 参考 MC 1.16.5 PigEntity.getAmbientSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * 参考 MC 1.16.5 PigEntity.getHurtSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * 参考 MC 1.16.5 PigEntity.getDeathSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== IRideable 接口实现 ==========

    [[nodiscard]] bool hasSaddle() const override { return m_hasSaddle; }

    void setSaddle(bool saddle) override { m_hasSaddle = saddle; }

    void onPlayerStartRiding(Player* player) override;

    void onPlayerStopRiding(Player* player) override;

    /**
     * @brief 获取骑乘速度
     * 参考 MC 1.16.5 PigEntity.getMountedSpeed()
     * @return 基础速度 * 0.225F
     */
    [[nodiscard]] f32 getSteeringSpeed() const override;

    bool boost() override;

    [[nodiscard]] i32 getBoostTime() const override { return m_boostTime; }

    void setBoostTime(i32 time) override { m_boostTime = time; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

    void tick() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.9f; }

    /**
     * @brief 获取眼睛高度
     * 参考 MC 1.16.5 PigEntity.getStandingEyeHeight()
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f * height(); }

private:
    bool m_hasSaddle = false;
    i32 m_boostTime = 0;
    f32 m_boostSpeed = 0.0f;

    // MC 1.16.5 常量
    static constexpr f32 PIG_SPEED = 0.25f;       // 基础移动速度
    static constexpr f32 MOUNTED_SPEED_MULT = 0.225f;  // 骑乘速度乘数
    static constexpr f32 BOOST_SPEED = 0.3f;      // 加速额外速度
    static constexpr i32 MAX_BOOST_TIME = 140;    // 最大加速时间 (7秒)
};

} // namespace mc
