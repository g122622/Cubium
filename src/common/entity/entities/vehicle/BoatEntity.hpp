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
 * @brief 船的状态
 *
 * MC 1.16.5: 用于确定船在不同介质中的行为
 */
enum class BoatStatus : u8 {
    InWater,           // 在水面上
    UnderWater,        // 在水下（水源方块）
    UnderFlowingWater, // 在水下（流动水）
    OnLand,            // 在陆地上
    InAir              // 在空中
};

/**
 * @brief 船实体
 *
 * 可乘坐的水上交通工具，有多种木材类型变体。
 *
 * 特性：
 * - 浮力：在水面漂浮
 * - 控制：玩家可以控制方向和速度
 * - 乘客：最多承载2名乘客
 * - 损坏：受到足够伤害会掉落物品
 * - 气泡柱：会被气泡柱影响
 *
 * 参考 MC 1.16.5 BoatEntity
 */
class BoatEntity : public Entity {
public:
    /**
     * @brief 船的类型（木材类型）
     * MC 1.16.5: Type enum
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

    // ========== Entity 接口重写 ==========

    void tick() override;

    [[nodiscard]] f32 width() const override { return 1.375f; }
    [[nodiscard]] f32 height() const override { return 0.5625f; }
    [[nodiscard]] f32 eyeHeight() const override { return height(); }

    /**
     * @brief 乘客乘坐高度偏移
     * MC 1.16.5: -0.1D
     */
    [[nodiscard]] f64 getMountedYOffset() const override;

    /**
     * @brief 最大乘客数量
     */
    [[nodiscard]] i32 getMaxPassengers() const override { return MAX_PASSENGERS; }

    /**
     * @brief 更新乘客位置
     */
    void updatePassengerPosition(Entity& passenger) override;

    // ========== 船类型 ==========

    /**
     * @brief 获取船的类型
     */
    [[nodiscard]] Type getBoatType() const { return m_type; }

    /**
     * @brief 设置船的类型
     */
    void setBoatType(Type type) { m_type = type; }

    /**
     * @brief 获取船的状态
     */
    [[nodiscard]] BoatStatus getStatus() const { return m_status; }

    // ========== 桨动画 ==========

    /**
     * @brief 获取桨角度
     * @param side 0=左桨, 1=右桨
     */
    [[nodiscard]] f32 getPaddleState(i32 side) const {
        if (side < 0 || side > 1) return 0.0f;
        return m_paddlePositions[side];
    }

    /**
     * @brief 设置桨状态
     */
    void setPaddleState(bool left, bool right) {
        m_leftPaddle = left;
        m_rightPaddle = right;
    }

    // ========== 控制 ==========

    /**
     * @brief 处理玩家控制输入
     * @param left 左转
     * @param right 右转
     * @param forward 前进
     * @param backward 后退
     */
    void handleInput(bool left, bool right, bool forward, bool backward);

    /**
     * @brief 设置船的朝向
     */
    void setRotation(f32 yaw) {
        Entity::setRotation(yaw, m_pitch);
    }

    // ========== 乘客 ==========

    /**
     * @brief 是否可以添加乘客
     */
    [[nodiscard]] bool canFitPassenger() const {
        return static_cast<i32>(m_passengers.size()) < MAX_PASSENGERS && m_status != BoatStatus::UnderWater;
    }

    // ========== 其他 ==========

    /**
     * @brief 掉落船物品
     */
    void dropItem();

    /**
     * @brief 获取上方水面高度
     * MC 1.16.5: getWaterLevelAbove()
     */
    [[nodiscard]] f32 getWaterLevelAbove();

protected:
    /**
     * @brief 更新运动
     */
    void updateMotion();

    /**
     * @brief 计算浮力
     */
    void floatBoat();

    /**
     * @brief 控制船的移动
     */
    void controlBoat();

    /**
     * @brief 更新插值
     */
    void tickLerp();

    /**
     * @brief 更新状态
     */
    void updateStatus();

    /**
     * @brief 获取水下状态
     * MC 1.16.5: getUnderwaterStatus()
     */
    [[nodiscard]] BoatStatus getUnderwaterStatus();

    /**
     * @brief 检测是否在水中
     * MC 1.16.5: checkInWater()
     */
    [[nodiscard]] bool checkInWater();

    /**
     * @brief 获取地面滑动系数
     * MC 1.16.5: getBoatGlide()
     */
    [[nodiscard]] f32 getBoatGlide();

    /**
     * @brief 更新所有乘客位置
     */
    void updateAllPassengerPositions();

    /**
     * @brief 将船的朝向应用到乘客
     * MC 1.16.5: applyYawToEntity()
     */
    void applyOrientationToEntity(Entity& passenger);

    /**
     * @brief 更新摇晃（气泡柱）
     */
    void updateRocking();

private:
    // 船类型
    Type m_type = Type::OAK;

    // 状态
    BoatStatus m_status = BoatStatus::InWater;
    BoatStatus m_previousStatus = BoatStatus::InWater;

    // 运动状态
    f32 m_speed = 0.0f;
    f32 m_rotationVelocity = 0.0f;

    // 桨状态
    f32 m_paddlePositions[2] = {0.0f, 0.0f};
    bool m_leftPaddle = false;
    bool m_rightPaddle = false;

    // 输入状态
    bool m_leftInputDown = false;
    bool m_rightInputDown = false;
    bool m_forwardInputDown = false;
    bool m_backwardInputDown = false;
    f32 m_deltaRotation = 0.0f;

    // 损伤状态
    i32 m_timeSinceHit = 0;
    i32 m_forwardDirection = 1;
    f32 m_damageTaken = 0.0f;

    // 插值
    i32 m_interpolationSteps = 0;
    f64 m_interpolationX = 0.0;
    f64 m_interpolationY = 0.0;
    f64 m_interpolationZ = 0.0;
    f64 m_interpolationYaw = 0.0;
    f64 m_interpolationPitch = 0.0;

    // 气泡柱
    i32 m_rockingTicks = 0;
    bool m_rocking = false;
    bool m_rockingDownwards = false;
    f32 m_rockingIntensity = 0.0f;
    f32 m_rockingAngle = 0.0f;
    f32 m_prevRockingAngle = 0.0f;

    // 水位
    f64 m_waterLevel = 0.0;
    f32 m_boatGlide = 0.0f;

    // 失控计时器
    i32 m_outOfControlTicks = 0;

    // 落下追踪
    f64 m_lastYd = 0.0;

    // MC 1.16.5 常量
    static constexpr f32 MAX_SPEED = 0.4f;
    static constexpr i32 MAX_PASSENGERS = 2;
};

} // namespace entity
} // namespace mc
