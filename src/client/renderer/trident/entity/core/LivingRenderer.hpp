#pragma once

#include "EntityRenderer.hpp"
#include "IEntityRenderer.hpp"
#include "AnimationContext.hpp"
#include "../layer/core/LayerRenderer.hpp"
#include "../model/core/EntityModel.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;  // 前向声明
}

namespace mc::client::renderer::entity::core {

/**
 * @brief 生物渲染器基类
 *
 * 用于渲染 LivingEntity 的渲染器基类。
 * 提供动画参数计算、模型渲染、层渲染器管理等功能。
 *
 * 参考 MC 1.16.5 LivingRenderer
 *
 * @tparam TEntity 实体类型（必须继承自 LivingEntity）
 * @tparam TModel 模型类型（必须继承自 EntityModel）
 */
template<typename TEntity, typename TModel>
class LivingRenderer : public EntityRenderer,
                        public IEntityRenderer<TEntity, TModel> {
    static_assert(std::is_base_of_v<::mc::LivingEntity, TEntity>,
                  "TEntity must derive from LivingEntity");
    static_assert(std::is_base_of_v<model::EntityModel, TModel>,
                  "TModel must derive from EntityModel");

public:
    using LayerRendererType = layer::core::LayerRenderer<TEntity>;

    LivingRenderer() = default;
    ~LivingRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    // ========== IEntityRenderer 接口实现 ==========

    [[nodiscard]] TModel& getModel() override { return m_model; }
    [[nodiscard]] const TModel& getModel() const override { return m_model; }

    // 注：getEntityTexture 需要子类实现

    // ========== EntityRenderer 接口实现 ==========

    /**
     * @brief LivingEntity 渲染器支持动画
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

    /**
     * @brief LivingEntity 渲染器支持层渲染
     */
    [[nodiscard]] bool supportsLayers() const override { return true; }

    /**
     * @brief 渲染层（GPU管线路径）
     *
     * 实现 EntityRenderer 接口，调用所有注册的层渲染器。
     */
    void renderLayersPipeline(
        Entity& entity,
        VkCommandBuffer cmd,
        const AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override {
        auto& living = static_cast<TEntity&>(entity);
        renderLayersPipeline(living, cmd, context, pipeline);
    }

    /**
     * @brief 计算动画上下文并设置模型角度
     *
     * 此方法供管线路径使用，在渲染前调用以更新模型状态。
     */
    void computeAnimationContext(
        Entity& entity,
        f64 partialTicks,
        AnimationContext& context,
        std::unique_ptr<model::EntityModel>& model
    ) override;

    // ========== 层渲染器管理 ==========

    /**
     * @brief 添加层渲染器
     * @tparam TLayer 层渲染器类型
     * @tparam TArgs 构造函数参数类型
     * @param args 构造函数参数
     */
    template<typename TLayer, typename... TArgs>
    void addLayer(TArgs&&... args) {
        static_assert(std::is_base_of_v<LayerRendererType, TLayer>,
                      "TLayer must derive from LayerRenderer<TEntity>");
        m_layers.push_back(std::make_unique<TLayer>(std::forward<TArgs>(args)...));
    }

    /**
     * @brief 获取层渲染器数量
     */
    [[nodiscard]] size_t getLayerCount() const { return m_layers.size(); }

    // ========== 动画状态计算（管线路径使用） ==========

    /**
     * @brief 计算动画上下文
     *
     * 计算实体的动画参数，并设置模型的旋转角度。
     * 此方法供管线路径使用，在渲染前调用以更新模型状态。
     *
     * @param entity 生物实体
     * @param partialTicks 部分 tick
     * @param context 输出的动画上下文
     */
    void computeAnimationContext(TEntity& entity, f64 partialTicks, AnimationContext& context) {
        // 计算动画参数
        context.partialTicks = partialTicks;
        context.limbSwing = getLimbSwing(entity, partialTicks);
        context.limbSwingAmount = getLimbSwingAmount(entity, partialTicks);
        context.ageInTicks = getAgeInTicks(entity);
        context.netHeadYaw = getHeadYaw(entity, partialTicks);
        context.headPitch = getHeadPitch(entity, partialTicks);
        context.scale = getScale(entity) * (1.0 / 16.0);

        // 计算状态
        context.isChild = false;
        context.isSitting = false;
        context.isSneaking = false;
        context.isSwimming = false;
        context.isRiding = false;
        context.swingProgress = 0.0f;

        // 检查是否为 AgeableEntity
        if constexpr (std::is_base_of_v<::mc::AgeableEntity, TEntity>) {
            context.isChild = entity.isChild();
        }

        // 计算哈希
        context.computeHash();

        // 设置模型角度
        m_model.setAngles(
            context.limbSwing,
            context.limbSwingAmount,
            context.ageInTicks,
            context.netHeadYaw,
            context.headPitch,
            context.scale
        );
    }

    /**
     * @brief 渲染层（管线路径）
     *
     * 使用管线路径渲染所有层。此方法需要访问管线，
     * 因此需要传入命令缓冲区和管线。
     *
     * @param entity 生物实体
     * @param cmd Vulkan 命令缓冲区
     * @param context 动画上下文
     * @param modelMatrix 模型矩阵
     * @param pipeline 实体管线
     */
    void renderLayersPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) {
        for (auto& layer : m_layers) {
            if (layer && layer->shouldRender(entity)) {
                layer->renderPipeline(entity, cmd, context, pipeline);
            }
        }
    }

protected:
    TModel m_model;
    std::vector<std::unique_ptr<LayerRendererType>> m_layers;

    /**
     * @brief 设置模型动画参数
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    void setModelAngles(TEntity& entity, f64 partialTicks);

    /**
     * @brief 渲染所有层渲染器
     * @param entity 生物实体
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     * @param partialTicks 部分tick
     * @param ageInTicks 年龄tick
     * @param netHeadYaw 头部偏航角
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    void renderLayers(TEntity& entity,
                      f32 limbSwing,
                      f32 limbSwingAmount,
                      f32 partialTicks,
                      f32 ageInTicks,
                      f32 netHeadYaw,
                      f32 headPitch,
                      f32 scale);

    /**
     * @brief 计算步态动画周期
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getLimbSwing(TEntity& entity, f64 partialTicks) const;

    /**
     * @brief 计算步态动画强度
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getLimbSwingAmount(TEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取头部偏航角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getHeadYaw(TEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取头部俯仰角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f64 getHeadPitch(TEntity& entity, f64 partialTicks) const;

    /**
     * @brief 获取年龄（tick）
     * @param entity 生物实体
     */
    [[nodiscard]] f64 getAgeInTicks(TEntity& entity) const;

    /**
     * @brief 处理缩放（幼体）
     * @param entity 生物实体
     * @return 缩放因子
     */
    [[nodiscard]] f64 getScale(TEntity& entity) const;
};

// ==================== 模板实现 ====================

template<typename TEntity, typename TModel>
void LivingRenderer<TEntity, TModel>::render(Entity& entity, f64 partialTicks) {
    // 转换为 TEntity
    auto& living = static_cast<TEntity&>(entity);

    // 设置模型动画参数
    setModelAngles(living, partialTicks);

    // 获取缩放因子
    f64 scale = getScale(living) * (1.0f / 16.0f);

    // 计算动画参数
    f64 limbSwing = getLimbSwing(living, partialTicks);
    f64 limbSwingAmount = getLimbSwingAmount(living, partialTicks);
    f64 ageInTicks = getAgeInTicks(living);
    f64 headYaw = getHeadYaw(living, partialTicks);
    f64 headPitch = getHeadPitch(living, partialTicks);

    // 渲染模型
    m_model.render(scale);

    // 渲染所有层
    renderLayers(living,
                 static_cast<f32>(limbSwing),
                 static_cast<f32>(limbSwingAmount),
                 static_cast<f32>(partialTicks),
                 static_cast<f32>(ageInTicks),
                 static_cast<f32>(headYaw),
                 static_cast<f32>(headPitch),
                 static_cast<f32>(scale));

    // 渲染阴影
    if (m_shadowSize > 0.0f) {
        renderShadow(entity, partialTicks);
    }
}

template<typename TEntity, typename TModel>
void LivingRenderer<TEntity, TModel>::renderLayers(TEntity& entity,
                                                     f32 limbSwing,
                                                     f32 limbSwingAmount,
                                                     f32 partialTicks,
                                                     f32 ageInTicks,
                                                     f32 netHeadYaw,
                                                     f32 headPitch,
                                                     f32 scale) {
    for (auto& layer : m_layers) {
        if (layer && layer->shouldRender(entity)) {
            layer->render(entity, limbSwing, limbSwingAmount, partialTicks,
                          ageInTicks, netHeadYaw, headPitch, scale);
        }
    }
}

template<typename TEntity, typename TModel>
void LivingRenderer<TEntity, TModel>::setModelAngles(TEntity& entity, f64 partialTicks) {
    f64 limbSwing = getLimbSwing(entity, partialTicks);
    f64 limbSwingAmount = getLimbSwingAmount(entity, partialTicks);
    f64 ageInTicks = getAgeInTicks(entity);
    f64 headYaw = getHeadYaw(entity, partialTicks);
    f64 headPitch = getHeadPitch(entity, partialTicks);
    f64 scale = getScale(entity);

    m_model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getLimbSwing(TEntity& entity, f64 partialTicks) const {
    // 步态动画周期
    f64 prevLimbSwing = entity.prevLimbSwing();
    f64 limbSwing = entity.limbSwing();
    return prevLimbSwing + (limbSwing - prevLimbSwing) * partialTicks;
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getLimbSwingAmount(TEntity& entity, f64 partialTicks) const {
    // 步态动画强度
    f64 prevAmount = entity.prevLimbSwingAmount();
    f64 amount = entity.limbSwingAmount();
    return prevAmount + (amount - prevAmount) * partialTicks;
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getHeadYaw(TEntity& entity, f64 partialTicks) const {
    // 头部偏航角（相对于身体）
    f64 bodyYaw = entity.prevRenderYawOffset() + (entity.renderYawOffset() - entity.prevRenderYawOffset()) * partialTicks;
    f64 headYaw = entity.prevRotationYawHead() + (entity.rotationYawHead() - entity.prevRotationYawHead()) * partialTicks;
    f64 diff = headYaw - bodyYaw;

    // 归一化到 -180 到 180
    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;

    return diff;
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getHeadPitch(TEntity& entity, f64 partialTicks) const {
    // 头部俯仰角
    f64 prevPitch = entity.prevPitch();
    f64 pitch = entity.pitch();
    return prevPitch + (pitch - prevPitch) * partialTicks;
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getAgeInTicks(TEntity& entity) const {
    // 年龄（用于空闲动画）
    return static_cast<f64>(entity.ticksExisted());
}

template<typename TEntity, typename TModel>
f64 LivingRenderer<TEntity, TModel>::getScale(TEntity& entity) const {
    // 幼体缩放 - 检查是否为 AgeableEntity
    // AgeableEntity 实现了 isChild() 方法
    // 使用动态转换来检查，避免模板约束问题

    // 尝试转换为 AgeableEntity 指针
    // 如果转换成功且为幼体，返回幼体缩放因子
    if constexpr (std::is_base_of_v<::mc::AgeableEntity, TEntity>) {
        if (entity.isChild()) {
            return 0.5f;  // 幼体缩放为成体的一半
        }
    }

    return 1.0f;
}

template<typename TEntity, typename TModel>
void LivingRenderer<TEntity, TModel>::computeAnimationContext(
    Entity& entity,
    f64 partialTicks,
    AnimationContext& context,
    std::unique_ptr<model::EntityModel>& model
) {
    auto& living = static_cast<TEntity&>(entity);

    // 计算动画参数
    context.partialTicks = partialTicks;
    context.limbSwing = getLimbSwing(living, partialTicks);
    context.limbSwingAmount = getLimbSwingAmount(living, partialTicks);
    context.ageInTicks = getAgeInTicks(living);
    context.netHeadYaw = getHeadYaw(living, partialTicks);
    context.headPitch = getHeadPitch(living, partialTicks);
    context.scale = getScale(living) * (1.0 / 16.0);

    // 计算状态
    context.isChild = false;
    context.isSitting = false;
    context.isSneaking = false;
    context.isSwimming = false;
    context.isRiding = false;
    context.swingProgress = 0.0f;

    // 检查是否为 AgeableEntity
    if constexpr (std::is_base_of_v<::mc::AgeableEntity, TEntity>) {
        context.isChild = entity.isChild();
    }

    // 计算哈希
    context.computeHash();

    // 设置模型角度
    m_model.setAngles(
        context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale
    );

    // 将模型指针传递出去（用于网格生成）
    // 注意：这里我们使用裸指针转换，因为 m_model 是成员变量
    // 调用者不应该持有这个 unique_ptr，只是用于类型擦除
    model.reset();  // 清空输入的 unique_ptr
    // 调用者需要知道 m_model 的生命周期由 LivingRenderer 管理
    // 这里我们不创建新的 unique_ptr，因为 m_model 是成员变量
    // 调用者应该使用返回的 AnimationContext 和直接访问 getModel()
}

} // namespace mc::client::renderer::entity::core
