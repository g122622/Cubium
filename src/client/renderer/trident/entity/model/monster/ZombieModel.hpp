#pragma once

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸模型
 *
 * 参考 MC 1.16.5 ZombieModel
 * 僵尸是双足生物，手臂向前伸。
 */
class ZombieModel : public model::BipedModel {
public:
    ZombieModel();
    ~ZombieModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
};

} // namespace mc::client::renderer::entity::model::monster
