#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"

namespace mc {

// 前向声明
class Entity;
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 视线控制器
 *
 * 控制实体的头部旋转，使其看向指定位置。
 *
 * 参考 MC 1.16.5 LookController
 */
class LookController {
public:
    virtual ~LookController() = default;

    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit LookController(MobEntity* mob);

    /**
     * @brief 设置看向位置
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    void setLookPosition(f64 x, f64 y, f64 z);

    /**
     * @brief 设置看向位置
     * @param pos 位置向量
     */
    void setLookPosition(const Vector3& pos) {
        setLookPosition(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 设置看向位置（带旋转速度限制）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param deltaYaw 偏航角最大变化速度
     * @param deltaPitch 俯仰角最大变化速度
     */
    void setLookPosition(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch);

    /**
     * @brief 设置看向实体
     *
     * 参考 MC 1.16.5 LookController.setLookPositionWithEntity()
     * @param entity 目标实体
     * @param deltaYaw 偏航角最大变化速度
     * @param deltaPitch 俯仰角最大变化速度
     */
    void setLookPositionWithEntity(const Entity& entity, f32 deltaYaw, f32 deltaPitch);

    /**
     * @brief 是否正在看向某处
     */
    [[nodiscard]] bool isLooking() const { return m_isLooking; }

    /**
     * @brief 获取目标X坐标
     */
    [[nodiscard]] f64 getLookPosX() const { return m_posX; }

    /**
     * @brief 获取目标Y坐标
     */
    [[nodiscard]] f64 getLookPosY() const { return m_posY; }

    /**
     * @brief 获取目标Z坐标
     */
    [[nodiscard]] f64 getLookPosZ() const { return m_posZ; }

    /**
     * @brief 刻更新
     *
     * 每tick调用，更新实体头部旋转。
     */
    void tick();

protected:
    /**
     * @brief 计算目标偏航角
     */
    [[nodiscard]] f32 getTargetYaw() const;

    /**
     * @brief 计算目标俯仰角
     */
    [[nodiscard]] f32 getTargetPitch() const;

    /**
     * @brief 限制角度变化
     * @param from 当前角度
     * @param to 目标角度
     * @param maxDelta 最大变化量
     * @return 限制后的角度
     */
    [[nodiscard]] static f32 clampedRotate(f32 from, f32 to, f32 maxDelta);

    /**
     * @brief 是否应该重置俯仰角
     */
    [[nodiscard]] virtual bool shouldResetPitch() const { return true; }

    MobEntity* m_mob;
    f32 m_deltaLookYaw = 10.0f;
    f32 m_deltaLookPitch = 10.0f;
    bool m_isLooking = false;
    f64 m_posX = 0.0;
    f64 m_posY = 0.0;
    f64 m_posZ = 0.0;
};

} // namespace entity::ai::controller
} // namespace mc
