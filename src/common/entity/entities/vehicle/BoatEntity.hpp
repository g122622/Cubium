/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#pragma once

#include "../../core/Entity.hpp"
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemEntity;
class BlockState;
class DamageSource;

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
    enum class Type : u8 { OAK = 0, SPRUCE = 1, BIRCH = 2, JUNGLE = 3, ACACIA = 4, DARK_OAK = 5 };

    explicit BoatEntity(Type type = Type::OAK);
    ~BoatEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 处理伤害
     * MC 1.16.5: BoatEntity.attackEntityFrom()
     * @note 船不继承LivingEntity，所以不重写hurt方法
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 检查是否可以被碰撞
     * MC 1.16.5: canBeCollidedWith()
     */
    [[nodiscard]] bool canBeCollidedWith() const override { return isAlive(); }

    /**
     * @brief 检查是否可以被推动
     * MC 1.16.5: canBePushed()
     */
    [[nodiscard]] bool canBePushed() const { return true; }

protected:
    /**
     * @brief 注册数据参数
     * MC 1.16.5: registerData()
     */
    void registerData() override;

public:
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

    // ========== 伤害和掉落 ==========

    /**
     * @brief 处理摔落伤害
     * MC 1.16.5: updateFallState()
     */
    void updateFallState(f64 y, bool onGround);

    /**
     * @brief 掉落船物品
     * MC 1.16.5: entityDropItem(this.getItemBoat())
     */
    void dropItem();

    /**
     * @brief 掉落船物品（带伤害倍率）
     * MC 1.16.5: 当超过伤害阈值时调用
     */
    void dropItemWithDamage();

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

    // ========== 伤害状态 ==========

    /**
     * @brief 获取上次受击时间
     * MC 1.16.5: getTimeSinceHit()
     */
    [[nodiscard]] i32 getTimeSinceHit() const { return m_timeSinceHit; }

    /**
     * @brief 设置上次受击时间
     */
    void setTimeSinceHit(i32 time) { m_timeSinceHit = time; }

    /**
     * @brief 获取前进方向
     * MC 1.16.5: getForwardDirection()
     */
    [[nodiscard]] i32 getForwardDirection() const { return m_forwardDirection; }

    /**
     * @brief 设置前进方向
     */
    void setForwardDirection(i32 direction) { m_forwardDirection = direction; }

    /**
     * @brief 获取累积伤害
     * MC 1.16.5: getDamageTaken()
     */
    [[nodiscard]] f32 getDamageTaken() const { return m_damageTaken; }

    /**
     * @brief 设置累积伤害
     */
    void setDamageTaken(f32 damage) { m_damageTaken = damage; }

    /**
     * @brief 获取摇晃tick数
     * MC 1.16.5: getRockingTicks()
     */
    [[nodiscard]] i32 getRockingTicks() const { return m_rockingTicks; }

    /**
     * @brief 设置摇晃tick数
     */
    void setRockingTicks(i32 ticks) { m_rockingTicks = ticks; }

    // ========== 桨动画 ==========

    /**
     * @brief 获取桨角度
     * @param side 0=左桨, 1=右桨
     */
    [[nodiscard]] f32 getPaddleState(i32 side) const
    {
        if (side < 0 || side > 1) return 0.0f;
        return m_paddlePositions[side];
    }

    /**
     * @brief 设置桨状态
     */
    void setPaddleState(bool left, bool right)
    {
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
    void setRotation(f32 yaw) { Entity::setRotation(yaw, m_pitch); }

    // ========== 乘客 ==========

    /**
     * @brief 是否可以添加乘客
     */
    [[nodiscard]] bool canFitPassenger() const override
    {
        return static_cast<i32>(m_passengers.size()) < MAX_PASSENGERS && m_status != BoatStatus::UnderWater;
    }

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
    void applyOrientationToEntity(Entity& passenger) override;

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
