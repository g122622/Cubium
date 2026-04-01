#pragma once

#include "EntityRenderer.hpp"
#include "model/EntityModel.hpp"
#include "../../../../common/entity/core/LivingEntity.hpp"
#include <memory>

namespace mc::client::renderer {

/**
 * @brief 生物渲染器基类
 *
 * 用于渲染 LivingEntity 的渲染器基类。
 * 提供动画参数计算、模型渲染等功能。
 *
 * 参考 MC 1.16.5 LivingRenderer
 */
template<typename TModel>
class LivingRenderer : public EntityRenderer {
public:
    static_assert(std::is_base_of_v<EntityModel, TModel>,
                  "TModel must derive from EntityModel");

    LivingRenderer() = default;
    ~LivingRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取模型
     */
    [[nodiscard]] TModel& model() { return m_model; }
    [[nodiscard]] const TModel& model() const { return m_model; }

protected:
    TModel m_model;

    /**
     * @brief 设置模型动画参数
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    void setModelAngles(LivingEntity& entity, f64 partialTicks);

    /**
     * @brief 计算步态动画周期
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getLimbSwing(LivingEntity& entity, f64 partialTicks) const;

    /**
     * @brief 计算步态动画强度
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getLimbSwingAmount(LivingEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取头部偏航角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getHeadYaw(LivingEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取头部俯仰角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getHeadPitch(LivingEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取年龄（tick）
     * @param entity 生物实体
     */
    [[nodiscard]] f64 getAgeInTicks(LivingEntity& entity) const;

    /**
     * @brief 处理缩放（幼体）
     * @param entity 生物实体
     * @return 缩放因子
     */
    [[nodiscard]] f64 getScale(LivingEntity& entity) const;
};

// ==================== 模板实现 ====================

template<typename TModel>
void LivingRenderer<TModel>::render(Entity& entity, f64 partialTicks) {
    // 转换为 LivingEntity
    auto& living = static_cast<LivingEntity&>(entity);

    // 设置模型动画参数
    setModelAngles(living, partialTicks);

    // 获取缩放因子
    f64 scale = getScale(living) * (1.0f / 16.0f);

    // 渲染模型
    m_model.render(scale);

    // 渲染阴影
    if (m_shadowSize > 0.0f) {
        renderShadow(entity, partialTicks);
    }

    (void)partialTicks;
}

template<typename TModel>
void LivingRenderer<TModel>::setModelAngles(LivingEntity& entity, f64 partialTicks) {
    f64 limbSwing = getLimbSwing(entity, partialTicks);
    f64 limbSwingAmount = getLimbSwingAmount(entity, partialTicks);
    f64 ageInTicks = getAgeInTicks(entity);
    f64 headYaw = getHeadYaw(entity, partialTicks);
    f64 headPitch = getHeadPitch(entity, partialTicks);
    f64 scale = getScale(entity);

    m_model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
}

template<typename TModel>
f64 LivingRenderer<TModel>::getLimbSwing(LivingEntity& entity, f64 partialTicks) const {
    // 步态动画周期
    f64 prevLimbSwing = entity.prevLimbSwing();
    f64 limbSwing = entity.limbSwing();
    return prevLimbSwing + (limbSwing - prevLimbSwing) * partialTicks;
}

template<typename TModel>
f64 LivingRenderer<TModel>::getLimbSwingAmount(LivingEntity& entity, f64 partialTicks) const {
    // 步态动画强度
    f64 prevAmount = entity.prevLimbSwingAmount();
    f64 amount = entity.limbSwingAmount();
    return prevAmount + (amount - prevAmount) * partialTicks;
}

template<typename TModel>
f64 LivingRenderer<TModel>::getHeadYaw(LivingEntity& entity, f64 partialTicks) const {
    // 头部偏航角（相对于身体）
    f64 bodyYaw = entity.prevRenderYawOffset() + (entity.renderYawOffset() - entity.prevRenderYawOffset()) * partialTicks;
    f64 headYaw = entity.prevRotationYawHead() + (entity.rotationYawHead() - entity.prevRotationYawHead()) * partialTicks;
    f64 diff = headYaw - bodyYaw;

    // 归一化到 -180 到 180
    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;

    return diff;
}

template<typename TModel>
f64 LivingRenderer<TModel>::getHeadPitch(LivingEntity& entity, f64 partialTicks) const {
    // 头部俯仰角
    f64 prevPitch = entity.prevPitch();
    f64 pitch = entity.pitch();
    return prevPitch + (pitch - prevPitch) * partialTicks;
}

template<typename TModel>
f64 LivingRenderer<TModel>::getAgeInTicks(LivingEntity& entity) const {
    // 年龄（用于空闲动画）
    return static_cast<f64>(entity.ticksExisted());
}

template<typename TModel>
f64 LivingRenderer<TModel>::getScale(LivingEntity& entity) const {
    // 幼体缩放 - 检查是否为 AgeableEntity
    // AgeableEntity 实现了 isChild() 方法
    // 这里我们使用动态转换来检查
    // TODO: 当 AgeableEntity 完全实现后启用
    (void)entity;
    return 1.0f;
}

} // namespace mc::client::renderer
