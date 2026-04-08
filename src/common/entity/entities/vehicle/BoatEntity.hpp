#pragma once

#include "../../core/Entity.hpp"
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class BlockState;

namespace entity {

/**
 * @brief 船实体基类
 *
 * 可乘坐的水上交通工具。
 * 有多种木材类型变体。
 *
 * 参考 MC 1.16.5 BoatEntity
 */
class BoatEntity : public Entity {
public:
    /**
     * @brief 船的类型（木材类型）
     */
    enum class Type : u8 {
        OAK = 0,
        SPRUCE = 1,
        BIRCH = 2,
        JUNGLE = 3,
        ACACIA = 4,
        DARK_OAK = 5
    };

    explicit BoatEntity(Type type = Type::OAK);
    ~BoatEntity() override = default;

    // Entity overrides
    void tick() override;

    // Boat-specific methods
    [[nodiscard]] bool isInWater() const override { return m_inWater; }
    [[nodiscard]] f32 getBoatSpeed() const { return m_speed; }
    void setBoatSpeed(f32 speed) { m_speed = speed; }

    /**
     * @brief 设置船的朝向
     * @param yaw Yaw角度
     */
    void setRotation(f32 yaw);

    /**
     * @brief 处理玩家控制输入
     * @param forward 前进输入
     * @param backward 后退输入
     * @param left 左转输入
     * @param right 右转输入
     */
    void handleInput(f32 forward, f32 backward, f32 left, f32 right);

    /**
     * @brief 掉落船物品
     */
    void dropItem();

protected:
    void updateMotion();
    void floatBoat();
    void controlBoat();

private:
    Type m_type;
    f32 m_speed = 0.0f;
    f32 m_rotationVelocity = 0.0f;
    f32 m_forwardInput = 0.0f;
    f32 m_turnInput = 0.0f;
    bool m_inWater = false;
    bool m_onLand = false;
    i32 m_interpolationSteps = 0;
    f64 m_interpolationX = 0.0;
    f64 m_interpolationZ = 0.0;
    static constexpr f32 MAX_SPEED = 0.4f;
    static constexpr f32 WATER_SPEED_MULT = 0.04f;
    static constexpr f32 LAND_SPEED_MULT = 0.2f;
};

} // namespace entity
} // namespace mc
