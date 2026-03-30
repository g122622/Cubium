#pragma once

#include "AnimalEntity.hpp"
#include "common/entity/interfaces/IRideable.hpp"

namespace mc {

// 前向声明
class IWorld;
class Player;

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

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== IRideable 接口实现 ==========

    [[nodiscard]] bool hasSaddle() const override { return m_hasSaddle; }

    void setSaddle(bool saddle) override { m_hasSaddle = saddle; }

    void onPlayerStartRiding(Player* player) override;

    void onPlayerStopRiding(Player* player) override;

    [[nodiscard]] f32 getSteeringSpeed() const override;

    bool boost() override;

    [[nodiscard]] i32 getBoostTime() const override { return m_boostTime; }

    void setBoostTime(i32 time) override { m_boostTime = time; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

    void tick() override;

private:
    bool m_hasSaddle = false;
    i32 m_boostTime = 0;
    f32 m_boostSpeed = 0.0f;

    static constexpr f32 PIG_SPEED = 0.2f;
    static constexpr f32 BOOST_SPEED = 0.3f;
    static constexpr i32 MAX_BOOST_TIME = 140; // 7秒
};

} // namespace mc
