#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 移动控制器动作类型
 */
enum class MoveAction : u8 {
    Wait,       // 等待
    MoveTo,     // 移动到目标
    Strafe,     // 横向移动
    Jumping     // 跳跃中
};

/**
 * @brief 移动控制器
 *
 * 控制实体的移动行为。
 *
 * 参考 MC 1.16.5 MovementController
 */
class MovementController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit MovementController(MobEntity* mob);

    /**
     * @brief 设置移动目标
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param speed 移动速度倍率
     */
    void setMoveTo(f64 x, f64 y, f64 z, f64 speed);

    /**
     * @brief 设置横向移动
     * @param forward 前进方向（-1到1）
     * @param strafe 横向方向（-1到1）
     */
    void strafe(f32 forward, f32 strafe);

    /**
     * @brief 检查目标位置是否可行走
     * MC 1.16.5 func_234024_b_
     * @param x X坐标
     * @param z Z坐标
     * @return 是否可行走
     */
    [[nodiscard]] bool canWalkAt(f64 x, f64 z) const;

    /**
     * @brief 是否正在更新（移动中）
     */
    [[nodiscard]] bool isUpdating() const { return m_action == MoveAction::MoveTo; }

    /**
     * @brief 获取移动速度倍率
     */
    [[nodiscard]] f64 speed() const { return m_speed; }

    /**
     * @brief 获取目标X坐标
     */
    [[nodiscard]] f64 getX() const { return m_posX; }

    /**
     * @brief 获取目标Y坐标
     */
    [[nodiscard]] f64 getY() const { return m_posY; }

    /**
     * @brief 获取目标Z坐标
     */
    [[nodiscard]] f64 getZ() const { return m_posZ; }

    /**
     * @brief 获取当前动作
     */
    [[nodiscard]] MoveAction action() const { return m_action; }

    /**
     * @brief 刻更新
     *
     * 每tick调用，更新实体移动。
     */
    void tick();

    MobEntity* m_mob;
    f64 m_posX = 0.0;
    f64 m_posY = 0.0;
    f64 m_posZ = 0.0;
    f64 m_speed = 1.0;
    f32 m_moveForward = 0.0f;
    f32 m_moveStrafe = 0.0f;
    MoveAction m_action = MoveAction::Wait;
};

} // namespace entity::ai::controller
} // namespace mc
