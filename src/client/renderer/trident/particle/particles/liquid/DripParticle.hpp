#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 液体滴落粒子基类
 *
 * 参考 MC 1.16.5 DripParticle
 *
 * 用于实现水滴、熔岩滴、蜂蜜滴等液体滴落效果。
 *
 * 特性：
 * - 从方块下方悬挂
 * - 缓慢积累变大
 * - 积累满后下落
 * - 落地后消失或变化
 */
class DripParticle : public Particle {
public:
    /**
     * @brief 滴落状态
     */
    enum class DripState {
        Hanging,    ///< 悬挂积累中
        Falling,    ///< 下落中
        Landed      ///< 已落地
    };

    DripParticle(const glm::vec3& pos, const glm::vec3& velocity);

    void tick(ClientWorld* world) override;

    [[nodiscard]] DripState dripState() const { return m_dripState; }
    [[nodiscard]] f64 dripProgress() const { return m_dripProgress; }

protected:
    /**
     * @brief 悬挂更新逻辑
     *
     * 子类可重写以自定义悬挂行为
     */
    virtual void tickHanging(ClientWorld* world);

    /**
     * @brief 下落更新逻辑
     *
     * 子类可重写以自定义下落行为
     */
    virtual void tickFalling(ClientWorld* world);

    /**
     * @brief 落地处理
     *
     * 子类可重写以自定义落地效果
     */
    virtual void onLand(ClientWorld* world);

    DripState m_dripState = DripState::Hanging;
    f64 m_dripProgress = 0.0f;    ///< 悬挂积累进度 (0-1)
    glm::vec3 m_hangPosition;     ///< 悬挂位置
};

} // namespace mc::client::renderer::trident::particle::particles
