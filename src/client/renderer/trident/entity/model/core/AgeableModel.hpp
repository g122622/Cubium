#pragma once

#include "EntityModel.hpp"
#include <memory>

namespace mc::client::renderer::entity::model {

/**
 * @brief 可成长模型基类
 *
 * 支持幼体和成年两种状态的模型。
 * 幼体通常有更大的头部比例和更小的身体。
 *
 * 参考 MC 1.16.5 AgeableModel
 */
class AgeableModel : public EntityModel {
public:
    AgeableModel();
    ~AgeableModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    // ========== 幼体状态 ==========

    /**
     * @brief 设置幼体状态
     * @param isChild 是否为幼体
     */
    void setChild(bool isChild) { m_isChild = isChild; }

    /**
     * @brief 是否为幼体
     */
    [[nodiscard]] bool isChild() const { return m_isChild; }

    // ========== 缩放参数 ==========

    /**
     * @brief 获取幼体缩放因子
     * @param baseScale 基础缩放
     * @return 幼体缩放后的值
     */
    [[nodiscard]] f64 getChildScale(f64 baseScale) const;

    /**
     * @brief 设置幼体头部缩放比例
     * @param scale 缩放比例（默认2.0，幼体头部更大）
     */
    void setChildHeadScale(f64 scale) { m_childHeadScale = scale; }

    /**
     * @brief 设置幼体身体缩放比例
     * @param scale 缩放比例（默认0.5，幼体身体更小）
     */
    void setChildBodyScale(f64 scale) { m_childBodyScale = scale; }

    /**
     * @brief 设置幼体头部偏移Y
     * @param offset Y偏移（用于调整头部位置）
     */
    void setChildHeadOffsetY(f64 offset) { m_childHeadOffsetY = offset; }

protected:
    bool m_isChild = false;

    // 幼体缩放参数
    f64 m_childHeadScale = 2.0f;     // 幼体头部缩放（更大）
    f64 m_childBodyScale = 0.5f;     // 幼体身体缩放（更小）
    f64 m_childHeadOffsetY = 4.0f;   // 幼体头部Y偏移

    /**
     * @brief 设置幼体模型属性
     *
     * 子类可重写此方法来调整幼体特有的模型参数。
     * 幼体通常：头部更大、身体更小、四肢更短。
     */
    virtual void setupChildModel();
};

} // namespace mc::client::renderer::entity::model
