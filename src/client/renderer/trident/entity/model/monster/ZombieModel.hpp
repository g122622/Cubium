#pragma once

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸模型
 *
 * 参考 MC 1.16.5 ZombieModel / AbstractZombieModel
 * 僵尸是双足生物，手臂向前伸，攻击时有特殊动画。
 */
class ZombieModel : public model::BipedModel {
public:
    /**
     * @brief 构造函数
     * @param slim 是否使用细长纹理（尸壳/溺尸使用64x32，普通僵尸使用64x64）
     */
    explicit ZombieModel(bool slim = false);
    ~ZombieModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置攻击动画
     * @param swingProgress 攻击进度 (0-1)
     * @param ageInTicks 年龄刻度（用于手臂抖动）
     * @param isAggressive 是否处于攻击状态
     */
    void setAttackAnimation(f64 swingProgress, f64 ageInTicks, bool isAggressive);

    /**
     * @brief 设置纹理尺寸
     * @param useSlimTexture true使用64x32纹理，false使用64x64纹理
     */
    void setTextureDimensions(bool useSlimTexture);

private:
    void setupParts();

    bool m_slim = false;  // 是否使用细长纹理
};

} // namespace mc::client::renderer::entity::model::monster
