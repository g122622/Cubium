#pragma once

#include "OcelotModel.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猫模型
 *
 * 参考 MC 1.16.5 CatModel
 * 继承自 OcelotModel，添加猫特有的动画（睡觉、伸懒腰等）。
 */
class CatModel : public OcelotModel {
public:
    explicit CatModel(f32 scale = 0.0f);
    ~CatModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 CatModel.setLivingAnimations
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置猫特有动画状态
     * @param lieDownAmount 躺下动画进度 (0-1)
     * @param relaxStateAmount 放松状态动画进度 (0-1)
     * @param sleepPoseAmount 睡眠姿势动画进度 (0-1)
     */
    void setCatAnimState(f32 lieDownAmount, f32 relaxStateAmount, f32 sleepPoseAmount);

    /**
     * @brief 设置是否坐下
     */
    void setSitting(bool sitting) { m_isSitting = sitting; }

private:
    f32 m_lieDownAmount = 0.0f;
    f32 m_relaxStateAmount = 0.0f;
    f32 m_sleepPoseAmount = 0.0f;
    bool m_isSitting = false;
};

} // namespace mc::client::renderer::entity::model::animal
