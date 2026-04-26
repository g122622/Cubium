#pragma once

#include "EntityModel.hpp"
#include <memory>
#include <vector>

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
    /**
     * @brief 默认构造函数
     */
    AgeableModel();

    /**
     * @brief 带参数的构造函数
     * @param isChildHeadScaled 是否缩放头部
     * @param childHeadOffsetY 头部 Y 偏移
     * @param childHeadOffsetZ 头部 Z 偏移
     */
    AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ);

    /**
     * @brief 完整参数的构造函数
     * @param isChildHeadScaled 是否缩放头部
     * @param childHeadOffsetY 头部 Y 偏移
     * @param childHeadOffsetZ 头部 Z 偏移
     * @param childHeadScale 头部缩放
     * @param childBodyScale 身体缩放
     * @param childBodyOffsetY 身体 Y 偏移
     */
    AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ,
                 f32 childHeadScale, f32 childBodyScale, f32 childBodyOffsetY);

    ~AgeableModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 EntityModel.setLivingAnimations
     * 用于在每帧设置模型状态（位置、状态变量）
     */
    virtual void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick);

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

protected:
    /**
     * @brief 获取头部部件
     * @return 头部模型部件列表
     */
    virtual std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const {
        return {};
    }

    /**
     * @brief 获取身体部件
     * @return 身体模型部件列表
     */
    virtual std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const {
        return m_parts;
    }

    // ========== 幼体参数 ==========
    bool m_isChildHeadScaled = false;
    f32 m_childHeadOffsetY = 5.0f;   // 默认值来自 Java
    f32 m_childHeadOffsetZ = 2.0f;   // 默认值来自 Java
    f32 m_childHeadScale = 2.0f;     // 头部缩放
    f32 m_childBodyScale = 2.0f;     // 身体缩放
    f32 m_childBodyOffsetY = 24.0f;  // 身体 Y 偏移

    bool m_isChild = false;
};

} // namespace mc::client::renderer::entity::model
